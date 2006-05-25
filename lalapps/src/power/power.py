# $Id$
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51
# Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#
# =============================================================================
#
#                                   Preamble
#
# =============================================================================
#

"""
Classes needed for the excess power analysis pipeline.
"""

__author__ = "Duncan Brown <duncan@gravity.phys.uwm.edu>, Kipp Cannon <kipp@gravity.phys.uwm.edu>"
__date__ = "$Date$"[7:-2]
__version__ = "$Revision$"[11:-2]

import math
import os
import sys
import time

from glue import segments
from glue.lal import CacheEntry
from glue import pipeline
from pylal import packing


#
# =============================================================================
#
#                                   Helpers
#
# =============================================================================
#

def get_universe(config_parser):
	return config_parser.get("condor", "universe")

def get_executable(config_parser, name):
	return config_parser.get("condor", name)

def get_out_dir(config_parser):
	return config_parser.get("pipeline", "out_dir")

def get_cache_dir(config_parser):
	return config_parser.get("pipeline", "cache_dir")

def make_dag_directories(config_parser):
	os.mkdir(get_cache_dir(config_parser))
	os.mkdir(get_out_dir(config_parser))


#
# =============================================================================
#
#                            DAG Node and Job Class
#
# =============================================================================
#

class BurstInjJob(pipeline.CondorDAGJob):
	"""
	A lalapps_binj job used by the power pipeline. The static options
	are read from the [lalapps_binj] section in the ini file. The
	stdout and stderr from the job are directed to the logs directory.
	The job runs in the universe specified in the ini file.  The path
	to the executable is determined from the ini file.
	"""
	def __init__(self, config_parser):
		"""
		config_parser = ConfigParser object
		"""
		pipeline.CondorDAGJob.__init__(self, get_universe(config_parser), get_executable(config_parser, "lalapps_binj"))

		# do this many injections between flow and fhigh inclusively
		self.injection_bands = config_parser.getint("pipeline", "injection_bands")

		self.add_ini_opts(config_parser, "lalapps_binj")

		self.set_stdout_file(os.path.join(get_out_dir(config_parser), "binj-$(macrochannelname)-$(macrogpsstarttime)-$(macrogpsendtime)-$(cluster)-$(process).out"))
		self.set_stderr_file(os.path.join(get_out_dir(config_parser), "binj-$(macrochannelname)-$(macrogpsstarttime)-$(macrogpsendtime)-$(cluster)-$(process).err"))
		self.set_sub_file("lalapps_binj.sub")


class BurstInjNode(pipeline.CondorDAGNode):
	def __init__(self, job):
		pipeline.CondorDAGNode.__init__(self, job)
		self.__usertag = None

	def set_user_tag(self, tag):
		self.__usertag = tag
		self.add_var_opt("user-tag", self.__usertag)

	def get_user_tag(self):
		return self.__usertag

	def set_start(self, start):
		self.add_var_opt("gps-start-time", start)

	def set_end(self, end):
		self.add_var_opt("gps-end-time", end)

	def get_start(self):
		return self.get_opts().get("macrogpsstarttime", None)

	def get_end(self):
		return self.get_opts().get("macrogpsendtime", None)

	def get_output_files(self):
		"""
		Returns the list of output file names.  This must be kept
		synchronized with the name of the output file in binj.c.
		Note in particular the calculation of the "start" and
		"duration" parts of the name.
		"""
		if not self.get_start() or not self.get_end():
			raise ValueError, "start time or end time has not been set"

		if self.__usertag:
			filename = "HL-INJECTIONS_%s-%d-%d.xml" % (self.__usertag, int(self.get_start()), int(self.get_end() - self.get_start()))
		else:
			filename = "HL-INJECTIONS-%d-%d.xml" % (int(self.get_start()), int(self.get_end() - self.get_start()))
		return [filename]

	def get_output(self):
		# FIXME: use get_output_files() instead
		return self.get_output_files()[0]


class PowerJob(pipeline.CondorDAGJob, pipeline.AnalysisJob):
	"""
	A lalapps_power job used by the power pipeline. The static options
	are read from the [lalapps_power] and [lalapps_power_<inst>]
	sections in the ini file. The stdout and stderr from the job are
	directed to the logs directory.  The job runs in the universe
	specified in the ini file. The path to the executable is determined
	from the ini file.
	"""
	def __init__(self, config_parser):
		"""
		config_parser = ConfigParser object
		"""
		pipeline.CondorDAGJob.__init__(self, get_universe(config_parser), get_executable(config_parser, "lalapps_power"))
		pipeline.AnalysisJob.__init__(self, config_parser)

		self.add_ini_opts(config_parser, "lalapps_power")

		self.set_stdout_file(os.path.join(get_out_dir(config_parser), "power-$(macrochannelname)-$(macrogpsstarttime)-$(macrogpsendtime)-$(cluster)-$(process).out"))
		self.set_stderr_file(os.path.join(get_out_dir(config_parser), "power-$(macrochannelname)-$(macrogpsstarttime)-$(macrogpsendtime)-$(cluster)-$(process).err"))
		self.set_sub_file("lalapps_power.sub")


class PowerNode(pipeline.AnalysisNode):
	def __init__(self, job):
		pipeline.CondorDAGNode.__init__(self, job)
		pipeline.AnalysisNode.__init__(self)
		self.__usertag = None

	def set_ifo(self, instrument):
		"""
		Load additional options from the per-instrument section in
		the config file.
		"""
		pipeline.AnalysisNode.set_ifo(self, instrument)
		for optvalue in self.job()._AnalysisJob__cp.items("lalapps_power_%s" % instrument):
			self.add_var_arg("--%s %s" % optvalue)

	def set_user_tag(self, tag):
		self.__usertag = tag
		self.add_var_opt("user-tag", self.__usertag)

	def get_user_tag(self):
		return self.__usertag

	def get_output_files(self):
		"""
		Returns the file name of output from the power code. This
		must be kept synchronized with the name of the output file
		in power.c.  Note in particular the calculation of the
		"start" and "duration" parts of the name.
		"""
		if None in [self.get_start(), self.get_end(), self.get_ifo(), self.__usertag]:
			raise ValueError, "start time, end time, ifo, or user tag has not been set"
		filename = "%s-POWER_%s-%d-%d.xml" % (self.get_ifo(), self.__usertag, int(self.get_start()), int(self.get_end()) - int(self.get_start()))
		return [filename]

	def get_output(self):
		raise NotImplementedError

	def set_mdccache(self, file):
		"""
		Set the LAL frame cache to to use. The frame cache is
		passed to the job with the --frame-cache argument.  @param
		file: calibration file to use.
		"""
		self.add_var_opt("mdc-cache", file)
		self.add_input_file(file)

	def set_burstinj(self, file):
		"""
		Set the LAL frame cache to to use. The frame cache is
		passed to the job with the --frame-cache argument.  @param
		file: calibration file to use.
		"""
		self.add_var_opt("burstinjection-file", file)
		self.add_input_file(file)

	def set_inspinj(self, file):
		"""
		Set the LAL frame cache to to use. The frame cache is
		passed to the job with the --frame-cache argument.  @param
		file: calibration file to use.
		"""
		self.add_var_opt("inspiralinjection-file", file)
		self.add_input_file(file)

	def set_siminj(self, file):
		"""
		Set the LAL frame cache to to use. The frame cache is
		passed to the job with the --frame-cache argument.  @param
		file: calibration file to use.
		"""
		self.add_var_opt("siminjection-file", file)
		self.add_input_file(file)


class BucutJob(pipeline.CondorDAGJob):
	def __init__(self, config_parser):
		pipeline.CondorDAGJob.__init__(self, "vanilla", get_executable(config_parser, "ligolw_bucut"))
		self.set_sub_file("ligolw_bucut.sub")
		self.set_stdout_file(os.path.join(get_out_dir(config_parser), "ligolw_bucut-$(cluster)-$(process).out"))
		self.set_stderr_file(os.path.join(get_out_dir(config_parser), "ligolw_bucut-$(cluster)-$(process).err"))
		self.add_condor_cmd("getenv", "True")
		self.add_ini_opts(config_parser, "ligolw_bucut")


class BucutNode(pipeline.CondorDAGNode):
	def add_file_arg(self, filename):
		pipeline.CondorDAGNode.add_file_arg(self, filename)
		self.add_output_file(filename)

	def get_output(self):
		raise NotImplementedError


class BuclusterJob(pipeline.CondorDAGJob):
	def __init__(self, config_parser):
		pipeline.CondorDAGJob.__init__(self, "vanilla", get_executable(config_parser, "ligolw_bucluster"))
		self.set_sub_file("ligolw_bucluster.sub")
		self.set_stdout_file(os.path.join(get_out_dir(config_parser), "ligolw_bucluster-$(cluster)-$(process).out"))
		self.set_stderr_file(os.path.join(get_out_dir(config_parser), "ligolw_bucluster-$(cluster)-$(process).err"))
		self.add_condor_cmd("getenv", "True")
		self.add_ini_opts(config_parser, "ligolw_bucluster")


class BuclusterNode(pipeline.CondorDAGNode):
	def add_file_arg(self, filename):
		pipeline.CondorDAGNode.add_file_arg(self, filename)
		self.add_output_file(filename)

	def get_output(self):
		raise NotImplementedError


class BinjfindJob(pipeline.CondorDAGJob):
	def __init__(self, config_parser):
		pipeline.CondorDAGJob.__init__(self, "vanilla", get_executable(config_parser, "ligolw_binjfind"))
		self.set_sub_file("ligolw_binjfind.sub")
		self.set_stdout_file(os.path.join(get_out_dir(config_parser), "ligolw_binjfind-$(cluster)-$(process).out"))
		self.set_stderr_file(os.path.join(get_out_dir(config_parser), "ligolw_binjfind-$(cluster)-$(process).err"))
		self.add_condor_cmd("getenv", "True")
		self.add_ini_opts(config_parser, "ligolw_binjfind")


class BinjfindNode(pipeline.CondorDAGNode):
	def add_file_arg(self, filename):
		pipeline.CondorDAGNode.add_file_arg(self, filename)
		self.add_output_file(filename)

	def get_output(self):
		raise NotImplementedError


class TisiJob(pipeline.CondorDAGJob):
	def __init__(self, config_parser):
		pipeline.CondorDAGJob.__init__(self, "vanilla", get_executable(config_parser, "ligolw_tisi"))
		self.set_sub_file("ligolw_tisi.sub")
		self.set_stdout_file(os.path.join(get_out_dir(config_parser), "ligolw_tisi-$(cluster)-$(process).out"))
		self.set_stderr_file(os.path.join(get_out_dir(config_parser), "ligolw_tisi-$(cluster)-$(process).err"))
		self.add_condor_cmd("getenv", "True")
		self.add_ini_opts(config_parser, "ligolw_tisi")


class TisiNode(pipeline.CondorDAGNode):
	def add_file_arg(self, filename):
		pipeline.CondorDAGNode.add_file_arg(self, filename)
		self.add_output_file(filename)

	def get_output(self):
		raise NotImplementedError


class BurcaJob(pipeline.CondorDAGJob):
	def __init__(self, config_parser):
		pipeline.CondorDAGJob.__init__(self, "vanilla", get_executable(config_parser, "ligolw_burca"))
		self.set_sub_file("ligolw_burca.sub")
		self.set_stdout_file(os.path.join(get_out_dir(config_parser), "ligolw_burca-$(cluster)-$(process).out"))
		self.set_stderr_file(os.path.join(get_out_dir(config_parser), "ligolw_burca-$(cluster)-$(process).err"))
		self.add_condor_cmd("getenv", "True")
		self.add_ini_opts(config_parser, "ligolw_burca")


class BurcaNode(pipeline.CondorDAGNode):
	def add_file_arg(self, filename):
		pipeline.CondorDAGNode.add_file_arg(self, filename)
		self.add_output_file(filename)

	def get_output(self):
		raise NotImplementedError


#
# =============================================================================
#
#                                DAG Job Types
#
# =============================================================================
#

# This is *SUCH* a hack I don't know where to begin.  Please, shoot me.

datafindjob = None
binjjob = None
powerjob = None
lladdjob = None
tisijob = None
binjfindjob = None
bucutjob = None
buclusterjob = None
burcajob = None

def init_job_types(config_parser, types = ["datafind", "binj", "power", "lladd", "tisi", "binjfind", "bucluster", "bucut"]):
	"""
	Construct definitions of the submit files.
	"""
	global datafindjob, binjjob, powerjob, lladdjob, tisijob, binjfindjob, buclusterjob, llb2mjob, bucutjob

	# LSCdataFind
	if "datafind" in types:
		datafindjob = pipeline.LSCDataFindJob(get_cache_dir(config_parser), get_out_dir(config_parser), config_parser)

	# lalapps_binj
	if "binj" in types:
		binjjob = BurstInjJob(config_parser)

	# lalapps_power
	if "power" in types:
		powerjob = PowerJob(config_parser)

	# ligolw_add
	if "lladd" in types:
		lladdjob = pipeline.LigolwAddJob(get_out_dir(config_parser), config_parser)
		lladdjob.cache_dir = get_cache_dir(config_parser)

	# ligolw_tisi
	if "tisi" in types:
		tisijob = TisiJob(config_parser)

	# ligolw_binjfind
	if "binjfind" in types:
		binjfindjob = BinjfindJob(config_parser)

	# ligolw_bucut
	if "bucut" in types:
		bucutjob = BucutJob(config_parser)

	# ligolw_bucluster
	if "bucluster" in types:
		buclusterjob = BuclusterJob(config_parser)

	# ligolw_burca
	if "burca" in types:
		burcajob = BurcaJob(config_parser)


#
# =============================================================================
#
#                                 Segmentation
#
# =============================================================================
#

def split_segment(powerjob, segment, psds_per_job):
	"""
	Split the data segment into correctly-overlaping segments.  We try
	to have the numbers of PSDs in each segment be equal to
	psds_per_job, but with a short segment at the end if needed.
	"""
	psd_length = float(powerjob.get_opts()["psd-average-points"]) / float(powerjob.get_opts()["resample-rate"])
	window_length = float(powerjob.get_opts()["window-length"]) / float(powerjob.get_opts()["resample-rate"])
	window_shift = float(powerjob.get_opts()["window-shift"]) / float(powerjob.get_opts()["resample-rate"])
	filter_corruption = float(powerjob.get_opts()["filter-corruption"]) / float(powerjob.get_opts()["resample-rate"])

	psd_overlap = window_length - window_shift

	joblength = psds_per_job * (psd_length - psd_overlap) + psd_overlap + 2 * filter_corruption
	joboverlap = 2 * filter_corruption + psd_overlap

	segs = segments.segmentlist()
	t = segment[0]
	while t + joblength <= segment[1]:
		segs.append(segments.segment(t, t + joblength) & segment)
		t += joblength - joboverlap

	extra_psds = int((float(segment[1] - t) - 2 * filter_corruption - psd_overlap) / (psd_length - psd_overlap))
	if extra_psds:
		segs.append(segments.segment(segment[0], segment[0] + extra_psds * (psd_length - psd_overlap) + psd_overlap + 2 * filter_corruption))
	return segs


def segment_ok(segment, psds_per_job):
	"""
	Return True if the segment can be analyzed using lalapps_power.
	"""
	# This is slow, but guaranteed to be using the same algorithm as
	# split_segment()
	return bool(split_segment(powerjob, segment, psds_per_job))


#
# =============================================================================
#
#                            Single Node Fragments
#
# =============================================================================
#

def make_datafind_fragment(dag, instrument, seg):
	datafind_pad = 512

	node = pipeline.LSCDataFindNode(datafindjob)
	node.set_name("LSCdataFind-%s-%s-%s" % (instrument, int(seg[0]), int(seg.duration())))
	node.set_start(seg[0] - datafind_pad)
	node.set_end(seg[1] + 1)
	node.set_observatory(instrument[0])
	node.set_post_script("/archive/home/kipp/local/bin/LSCdataFindcheck --dagman-return $RETURN --gps-start-time %s --gps-end-time %s %s" % (str(seg[0]), str(seg[1]), node.get_output()))
	dag.add_node(node)

	return node


def make_tisi_fragment(dag, tag):
	node = TisiNode(tisijob)
	node.set_name("ligolw_tisi-%s" % tag)
	node.add_file_arg("tisi_%s.xml" % tag)
	node.add_macro("macrocomment", tag)
	dag.add_node(node)

	return node


def make_lladd_fragment(dag, parents, instrument, seg, tag):
	cache_name = os.path.join(lladdjob.cache_dir, "lladd-%s-%s-%s-%s.cache" % (instrument, tag, int(seg[0]), int(seg.duration())))
	cache = packing.LALCache()

	node = pipeline.LigolwAddNode(lladdjob)
	node.set_name("lladd-%s-%s-%s-%s" % (instrument, tag, int(seg[0]), int(seg.duration())))
	for parent in parents:
		node.add_parent(parent)
		for filename in parent.get_output_files():
			cache.add_new("ANY", "EMPTY", seg, filename)
	print >>file(cache_name, "w"), str(cache)
	node.add_var_opt("input-cache", cache_name)
	dag.add_node(node)

	return node


def make_power_fragment(dag, parents, instrument, seg, tag, framecache, injargs = {}):
	node = PowerNode(powerjob)
	node.set_name("lalapps_power-%s-%s-%s-%s" % (instrument, tag, int(seg[0]), int(seg.duration())))
	map(node.add_parent, parents)
	node.set_cache(framecache)
	node.set_ifo(instrument)
	node.set_start(seg[0])
	node.set_end(seg[1])
	node.set_user_tag(tag)
	for arg, value in injargs.iteritems():
		# this is a hack, but I can't be bothered
		node.add_var_arg("--%s %s" % (arg, value))
	dag.add_node(node)

	return node


def make_bucut_fragment(dag, parents, instrument, seg, tag):
	node = BucutNode(bucutjob)
	node.set_name("ligolw_bucut-%s-%s-%s-%s" % (instrument, tag, int(seg[0]), int(seg.duration())))
	for parent in parents:
		node.add_parent(parent)
		map(node.add_file_arg, parent.get_output_files())
	node.add_macro("macrocomment", tag)
	dag.add_node(node)

	return node


def make_binj_fragment(dag, seg, tag, offset, flow, fhigh):
	# determine frequency ratio from number of injections across band
	# (take a bit off to allow fhigh to be picked up despite round-off
	# errors)
	fratio = 0.9999999 * (fhigh / flow) ** (1.0 / binjjob.injection_bands)

	# one injection every time-step / pi seconds
	period = binjjob.injection_bands * float(binjjob.get_opts()["time-step"]) / math.pi

	# adjust start time to be commensurate with injection period
	start = seg[0] - seg[0] % period + period * offset

	node = BurstInjNode(binjjob)
	node.set_start(start)
	node.set_end(seg[1])
	node.set_name("lalapps_binj-%d-%d" % (int(start), int(flow)))
	node.set_user_tag(tag)
	node.add_macro("macroflow", flow)
	node.add_macro("macrofhigh", fhigh)
	node.add_macro("macrofratio", fratio)
	node.add_macro("macroseed", int(time.time() + start))
	dag.add_node(node)

	return node


def make_bucluster_fragment(dag, parents, instrument, seg, tag):
	node = BuclusterNode(buclusterjob)
	node.set_name("ligolw_bucluster-%s-%s-%s-%s" % (instrument, tag, int(seg[0]), int(seg.duration())))
	for parent in parents:
		node.add_parent(parent)
		map(node.add_file_arg, parent.get_output_files())
	node.add_macro("macrocomment", tag)
	dag.add_node(node)

	return node


def make_binjfind_fragment(dag, parents, instrument, seg, tag):
	cluster = make_bucluster_fragment(dag, parents, instrument, seg, tag)

	binjfind = BinjfindNode(binjfindjob)
	binjfind.set_name("ligolw_binjfind-%s-%s-%s-%s" % (instrument, tag, int(seg[0]), int(seg.duration())))
	binjfind.add_parent(cluster)
	map(binjfind.add_file_arg, cluster.get_output_files())
	binjfind.add_macro("macrocomment", tag)
	dag.add_node(binjfind)

	return binjfind


#
# =============================================================================
#
#        DAG Fragment Combining Multiple lalapps_power With ligolw_add
#
# =============================================================================
#

def make_multipower_fragment(dag, powerparents, lladdparents, framecache, seglist, instrument, tag, injargs = {}):
	segment = seglist.extent()
	powernodes = [make_power_fragment(dag, powerparents, instrument, seg, tag, framecache, injargs) for seg in seglist]
	lladdnode = make_lladd_fragment(dag, powernodes + lladdparents, instrument, segment, "POWER_%s" % tag)
	lladdnode.set_output("%s-POWER_%s-%s-%s.xml" % (instrument, tag, int(segment[0]), int(segment.duration())))
	return lladdnode


#
# =============================================================================
#
#             Analyze A Segment Using Multiple lalapps_power Jobs
#
# =============================================================================
#

def make_power_segment_fragment(dag, datafindnode, segment, instrument, psds_per_job, tag):
	"""
	Construct a DAG fragment for an entire segment, splitting the
	segment into multiple power jobs.
	"""
	seglist = split_segment(powerjob, segment, psds_per_job)
	print >>sys.stderr, "Segment split: " + str(seglist)

	lladdnode = make_multipower_fragment(dag, [datafindnode], [], datafindnode.get_output(), seglist, instrument, tag)

	return lladdnode


#
# =============================================================================
#
#         DAG Fragment Combining Multiple lalapps_binj With ligolw_add
#
# =============================================================================
#

def make_multibinj_fragment(dag, seg, tag):
	flow = float(powerjob.get_opts()["low-freq-cutoff"])
	fhigh = flow + float(powerjob.get_opts()["bandwidth"])

	binjnodes = [make_binj_fragment(dag, seg, tag, 0.0, flow, fhigh)]

	node = make_lladd_fragment(dag, binjnodes, "ANY", seg, tag)
	node.set_output("HL-%s-%d-%d.xml" % (tag, int(seg[0]), int(seg.duration())))

	return node


#
# =============================================================================
#
#     Analyze A Segment Using Multiple lalapps_power Jobs With Injections
#
# =============================================================================
#

def make_injection_segment_fragment(dag, datafindnode, segment, instrument, psds_per_job, tag, binjfrag = None, tisifrag = None):
	seglist = split_segment(powerjob, segment, psds_per_job)
	print >>sys.stderr, "Injections split: " + str(seglist)

	if not binjfrag:
		binjfrag = make_multibinj_fragment(dag, seglist.extent(), "INJECTIONS_%s" % tag)

	if not tisifrag:
		tisifrag = make_tisi_fragment(dag, "INJECTIONS_%s" % tag)

	lladdnode = make_multipower_fragment(dag, [datafindnode, binjfrag], [binjfrag, tisifrag], datafindnode.get_output(), seglist, instrument, "INJECTIONS_%s" % tag, injargs = {"burstinjection-file": binjfrag.get_output()})

	bucutnode = make_bucut_fragment(dag, [lladdnode], instrument, segment, "INJECTIONS_%s" % tag)

	return bucutnode

