/*
*  Copyright (C) 2007 Sukanta Bose, Jolien Creighton, John Whelan
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with with program; see the file COPYING. If not, write to the
*  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
*  MA  02111-1307  USA
*/

/*********************** <lalVerbatim file="SimulateSBTestCV">
Author: Sukanta Bose (Adapted from a non-LAL code written by Bruce Allen)
$Id$
********************************* </lalVerbatim> */

/********************************************************** <lalLaTeX>
\subsection{Program \texttt{SimulateSBTest.c}}
\label{inject:ss:SimulateSBTest.c}

A program to test \texttt{LALSSSimStochBGTimeSeries()}.

\subsubsection*{Usage}

\begin{verbatim}
./SimulateSBTest [options]
Options:
  -h             print usage message
  -q             quiet: run silently
  -v             verbose: print extra information
  -d level       set lalDebugLevel to level
  -s siteID1     calculate simulated SB signal for site siteID1
  -t siteID2       with site siteID2
  -f f0          set start frequency to f0
  -e deltaT      set temporal spacing to deltaT
  -l length      set number of points in time series to length
  -a alpha       the exponent in freq power law for Omega
  -r fRef        reference freq needed to compute Omega
  -o omegaRef    value of Omega at reference frequency
  -p filename    print simulated SB signal for site1 to file filename
  -q filename    print simulated SB signal for site2 to file filename

\end{verbatim}

\subsubsection*{Description}

This program tests the function {\tt LALSSSimStochBGTimeSeries()\/}, which
calculates the signal from a stochastic background in the outputs of a pair of
gravitational wave detectors.

It first tests that the correct error codes
(\textit{cf.}\ Sec.~\ref{inject:s:SimulateSB.h})
     are generated for the following error conditions:
\begin{itemize}
\item null pointer to parameter structure
\item null pointer to output series
\item null pointer to data member of output series
\item null pointer to data member of data member of output series
\end{itemize}

It then verifies that the correct time series are generated for
valid input data.

To test the function {\tt LALSSSimStochBGTimeSeries()\/}, this test program
generates
detector response function, $R_I (f)$ ($I=1$, 2) for each of the detectors in
the pair. To do so, it first computes the noise amplitude spectrum $s(f)$
(assumed identical) for each detector. Taking the one-sided detector output
to be white Gaussian noise (in the absence of a signal), with a chosen
root-mean-squared value, RMS, it then computes the detector response function,
$R(f) = {\rm RMS}/(\sqrt(f_{\rm Nyquist}) s(f))$.

The outputs of this code are two time-domain data samples, namely,
WHITENED-SB1 and WHITENED-SB2, corresponding to the SB signals in detector
at site 1 and in detector at site 2, respectively.

As an additional test, this code prints out the mean-squared theoretical value
of the whitened output of the detector at site 1 and the  mean-squared value
of the data samples generated by the function  {\tt LALSSSimStochBG()\/}
for the same detector. The ratio of these two outputs, which is also printed,
should ideally be equal to 1.

\subsubsection*{Uses}

\begin{verbatim}
lalDebugLevel
getopt()
LALSCreateVector()
LALOverlapReductionFunction()
LALStochasticOmegaGW()
LALSimulateSB()
LALSPrintTimeSeries()
LALSDestroyVector()
LALCheckMemoryLeaks()
\end{verbatim}

******************************************************* </lalLaTeX> */

#include <math.h>
#include <string.h>
#include <stdio.h>

#include <lal/LALStdlib.h>
#include <lal/LALConstants.h>
#include <lal/LALStatusMacros.h>
#include <lal/StochasticCrossCorrelation.h>
#include <lal/AVFactories.h>
#include <lal/RealFFT.h>
#include <lal/ComplexFFT.h>
#include <lal/PrintFTSeries.h>
#include <lal/Units.h>
#include <lal/PrintVector.h>
#include <lal/Random.h>
#include <lal/SimulateSB.h>
#include "CheckStatus.h"
#include <lal/DetectorSite.h>
/**/

NRCSID(SIMULATESBTESTC, "$Id$");

#define SIMULATESBTESTC_LENGTH    8192
#define SIMULATESBTESTC_SEED      123
#define SIMULATESBTESTC_RATE      128.0
#define SIMULATESBTESTC_F0        0.0
#define SIMULATESBTESTC_ALPHA     0.0
#define SIMULATESBTESTC_FREF      100.0
#define SIMULATESBTESTC_OMEGAREF  1.e-5
#define SIMULATESBTESTC_RMS       1024
#define SIMULATESBTESTC_DETECTORONE lalCachedDetectors[site0]
#define SIMULATESBTESTC_DETECTORTWO lalCachedDetectors[site1]
#define SIMULATESBTESTC_TRUE     1
#define SIMULATESBTESTC_FALSE    0
#define SIMULATESBTESTC_BAR      0

/* These values do not necessarily represent any physical bar*/
REAL8     SIMULATESBTESTC_BARLONGRAD  =  4.691815; /*in radians*/
REAL8     SIMULATESBTESTC_BARLATRAD   =  0.426079; /*in radians*/
REAL4     SIMULATESBTESTC_BARALT      = -6.574;    /*in meters*/
REAL4     SIMULATESBTESTC_BARXALTRAD  =  0.0;
REAL4     SIMULATESBTESTC_BARXAZIRAD  =  0.0;
REAL4     SIMULATESBTESTC_BARYALTRAD  =  0.0;
REAL4     SIMULATESBTESTC_BARYAZIRAD  =  0.0;
REAL8     SIMULATESBTESTC_BARLOCX     = -113258.848;
REAL8     SIMULATESBTESTC_BARLOCY     =  5504077.706;
REAL8     SIMULATESBTESTC_BARLOCZ     =  3209892.343;

extern char *optarg;
extern int   optind;

/* int lalDebugLevel = LALMSGLVL3; */
int lalDebugLevel = LALNDEBUG;
BOOLEAN optVerbose    = SIMULATESBTESTC_FALSE;
REAL8 optDeltaT       = -1;
UINT4 optLength       = 0;
REAL8 optF0           = 0.0;
UINT4 optDetector1    = LALNumCachedDetectors;
UINT4 optDetector2    = LALNumCachedDetectors;
REAL4 optAlpha        = 0.0;
REAL4 optFRef         = 100.0;
REAL4 optOmegaRef     = 1.e-5;

CHAR  optFile[LALNameLength] = "";

#if 0
static void
Usage (const char *program, int exitflag);

static void
ParseOptions (int argc, char *argv[]);
#endif

/***************************** <lalErrTable file="SimulateSBTestCE"> */
#define SIMULATESBTESTC_ENOM 0
#define SIMULATESBTESTC_EARG 1
#define SIMULATESBTESTC_ECHK 2
#define SIMULATESBTESTC_EFLS 3
#define SIMULATESBTESTC_EUSE 4
#define SIMULATESBTESTC_MSGENOM "Nominal exit"
#define SIMULATESBTESTC_MSGEARG "Error parsing command-line arguments"
#define SIMULATESBTESTC_MSGECHK "Error checking failed to catch bad data"
#define SIMULATESBTESTC_MSGEFLS "Incorrect answer for valid data"
#define SIMULATESBTESTC_MSGEUSE "Bad user-entered data"
/***************************** </lalErrTable> */


/* LIGO-1 power spectrum; returns strain/rHz */
static float s_of_f(float freq) {
  int i;
  double slope,y;

  /* number of different log-slope piecewise linear bits in power spectrum */
  enum constants {NBREAKS=7};

  /* structure containing frequency/S_h pairs in increasing frequency order */
  typedef struct pairstag {
    REAL4 freq;
    REAL4 S_h;
  } pairstype;

  /* data that defines a piecewise power-law response function */
  pairstype pairs[NBREAKS+1]={{0.001, 1.0},
			      {0.006, 1.0},
			      {18.80, 2.205e-18},
			      {33.60, 2.990e-22},
			      {157.0, 1.800e-23},
			      {450.0, 5.500e-23},
			      {512.0, 1.0},
			      {1.0e6, 1.0}};

  /* if to the left of the first point */
  if (freq<pairs[0].freq)
    /* return first point */
    freq=pairs[0].freq;
  /* if to the right of the last point */
  else if (freq>pairs[NBREAKS].freq)
    /* return last point */
    freq=pairs[NBREAKS].freq;

  /* hunt for correct pair of points in array */
  i=0;
  while (pairs[i].freq<freq) i++;

  /* find logarithmic slope between that pair of points */
  slope=log(pairs[i].S_h/pairs[i-1].S_h)/
    log(pairs[i].freq/pairs[i-1].freq);

  /* and do power law interpolation between them */
  y=pairs[i-1].S_h*pow(freq/pairs[i-1].freq,slope);

  /* finally return the computed value of strain/rHz */
  return y;
}

int main( void ){
  static LALStatus status;

  /* This stores the parameters of functions used */
  StochasticOmegaGWParameters        parametersOmega;

  /* This stores the structures of SimulateSB */
  SSSimStochBGParams                 SBParams;
  SSSimStochBGInput                  SBInput;
  SSSimStochBGOutput                 SBOutput;
  REAL4TimeSeries                    whitenedSSimStochBG1;
  REAL4TimeSeries                    whitenedSSimStochBG2;

  REAL4FrequencySeries               omegaGW;
  COMPLEX8FrequencySeries            wFilter1;
  COMPLEX8FrequencySeries            wFilter2;



  /* vector to store response functions of a pair of detectors  */
  COMPLEX8Vector                    *response[2]={NULL,NULL};

  /* The detectors */
  int                                site0 =0;
  int                                site1= 1;

  /* Counters & output */
  INT4                               i;
  REAL8                              totnorm;
  REAL8                              totnorm2;

  /* various times, frequencies, sample rates */
  UINT4                              length = SIMULATESBTESTC_LENGTH;
  UINT4                              freqlen=length/2+1;

  REAL4                              fnyquist=0.5*SIMULATESBTESTC_RATE;
  REAL4                              deltaF;
  INT4                               code;

  LALFrDetector                      barFrame;


  /*
   *
   * Define valid parameters
   *
   */


  /* Create vectors */
  omegaGW.data = NULL;
  LALSCreateVector(&status, &(omegaGW.data), freqlen);

  whitenedSSimStochBG1.data = NULL;
  LALSCreateVector(&status, &(whitenedSSimStochBG1.data), length);
  whitenedSSimStochBG2.data = NULL;
  LALSCreateVector(&status, &(whitenedSSimStochBG2.data), length);

  for (i=0;i<2;i++)
    {
      LALCCreateVector(&status, &response[i],freqlen);
    }

  if (SIMULATESBTESTC_BAR) {
    barFrame.vertexLongitudeRadians = SIMULATESBTESTC_BARLONGRAD;
    barFrame.vertexLatitudeRadians = SIMULATESBTESTC_BARLATRAD;
    barFrame.vertexElevation = SIMULATESBTESTC_BARALT;
    barFrame.xArmAltitudeRadians = SIMULATESBTESTC_BARXALTRAD;
    barFrame.xArmAzimuthRadians  = SIMULATESBTESTC_BARXAZIRAD;
    barFrame.yArmAltitudeRadians  = SIMULATESBTESTC_BARYALTRAD;
    barFrame.yArmAzimuthRadians = SIMULATESBTESTC_BARYAZIRAD;
  }

  /* define SimulateSBParams */
  SBParams.length         = length;
  SBParams.deltaT         = 1/SIMULATESBTESTC_RATE;
  SBParams.seed           = SIMULATESBTESTC_SEED;
  SBParams.SSimStochBGTimeSeries1Unit = lalADCCountUnit;
  SBParams.SSimStochBGTimeSeries2Unit = lalADCCountUnit;

  if (SIMULATESBTESTC_BAR) {
    LALCreateDetector(&status, &(SBParams.detectorOne), &barFrame, LALDETECTORTYPE_CYLBAR);
  }
  else {
    SBParams.detectorOne    = SIMULATESBTESTC_DETECTORONE;
  }
  SBParams.detectorTwo    = SIMULATESBTESTC_DETECTORTWO;


  deltaF = 1/(SBParams.deltaT*SBParams.length);

  /* find omegaGW, and print it */
  parametersOmega.length   = SBParams.length/2 + 1;
  parametersOmega.f0       = SIMULATESBTESTC_F0;
  parametersOmega.deltaF   = deltaF;
  parametersOmega.alpha    = SIMULATESBTESTC_ALPHA;
  parametersOmega.fRef     = SIMULATESBTESTC_FREF;
  parametersOmega.omegaRef = SIMULATESBTESTC_OMEGAREF;
  LALStochasticOmegaGW(&status, &omegaGW, &parametersOmega);

  for (i=1;(UINT4)i<freqlen;i++){
    /* response fn */
    float freq = i/(SBParams.deltaT*SBParams.length);
    float factor=SIMULATESBTESTC_RMS/(sqrt(fnyquist)*s_of_f(freq));
    response[0]->data[i].re = factor;
    response[0]->data[i].im = 0.0;
    response[1]->data[i].re = factor;
    response[1]->data[i].im = 0.0;
  }

  response[0]->data[0].re = 0.0;
  response[0]->data[0].im = 0.0;
  response[1]->data[0].re = 0.0;
  response[1]->data[0].im = 0.0;

  wFilter1.epoch.gpsSeconds = 0;
  wFilter1.deltaF = deltaF;
  wFilter1.f0 = SIMULATESBTESTC_F0;
  wFilter1.data = response[0];

  wFilter2.epoch.gpsSeconds = 0;
  wFilter2.deltaF = deltaF;
  wFilter2.f0 = SIMULATESBTESTC_F0;
  wFilter2.data = response[1];

  /* define SSSimStochBGInput */
  SBInput.omegaGW                  = &omegaGW;
  SBInput.whiteningFilter1         = &wFilter1;
  SBInput.whiteningFilter2         = &wFilter2;

  /*
   *
   * TEST INVALID DATA HERE
   *
   *
   */

#ifndef LAL_NDEBUG
  if ( ! lalNoDebug )
    {
      /* test behavior for null pointer to real time series for output */
      LALSSSimStochBGTimeSeries(&status, NULL, &SBInput, &SBParams);
      if ( ( code = CheckStatus(&status, SIMULATESBH_ENULLP,
			      SIMULATESBH_MSGENULLP,SIMULATESBTESTC_ECHK,
			      SIMULATESBTESTC_MSGECHK) ) )
	{
	  return code;
	}
      printf("  PASS: null pointer to output series results in error: \n"
		      "\"%s\"\n", SIMULATESBH_MSGENULLP);

      /* test behavior for null pointer to input structure */
      SBOutput.SSimStochBG1 = &whitenedSSimStochBG1;
      SBOutput.SSimStochBG2 = &whitenedSSimStochBG2;
      LALSSSimStochBGTimeSeries(&status, &SBOutput, NULL, &SBParams);
      if ( ( code = CheckStatus(&status, SIMULATESBH_ENULLP,
			      SIMULATESBH_MSGENULLP,SIMULATESBTESTC_ECHK,
			      SIMULATESBTESTC_MSGECHK) ) )
	{
	  return code;
	}
      printf("  PASS: null pointer to input structure results in error: \n"
		      "\"%s\"\n", SIMULATESBH_MSGENULLP);
    }

#endif /* LAL_NDEBUG */

  /*
   *
   * TEST VALID DATA HERE
   *
   *
   */

  SBOutput.SSimStochBG1 = &whitenedSSimStochBG1;
  SBOutput.SSimStochBG2 = &whitenedSSimStochBG2;

  /* generate whitened simulated SB data */
  LALSSSimStochBGTimeSeries(&status, &SBOutput, &SBInput, &SBParams);
  if ( ( code = CheckStatus(&status, 0, "",
			  SIMULATESBTESTC_EFLS, SIMULATESBTESTC_MSGEFLS) ) )
    {
      return code;
    }

  /* Mean square */
  totnorm2=0.0;
  for (i=0;(UINT4)i<length;i++)
    totnorm2+=((whitenedSSimStochBG1.data->data[i])*(whitenedSSimStochBG1.data->data[i]));
  totnorm2/=length;
  printf("Mean square of whitened output is: %e\n",totnorm2);

  if (!SIMULATESBTESTC_BAR){
    /* check normalizations */
    totnorm=0.0;
    for (i=1;(UINT4)i<freqlen;i++){
      REAL8 freq=i*SIMULATESBTESTC_RATE/SIMULATESBTESTC_LENGTH;
      REAL8 resp=SIMULATESBTESTC_RMS/(sqrt(fnyquist)*s_of_f(freq));
      totnorm+=resp*resp*(omegaGW.data->data[i])/(freq*freq*freq);
    }
    totnorm*=0.3*LAL_H0FAC_SI*LAL_H0FAC_SI*SIMULATESBTESTC_RATE/(LAL_PI*LAL_PI*SIMULATESBTESTC_LENGTH);
    printf("Mean square of whitened output should be: %e.  Ratio is %e\n",totnorm,totnorm/totnorm2);
  }

  LALSPrintTimeSeries(&whitenedSSimStochBG1,"WHITENED-SB1");
  LALSPrintTimeSeries(&whitenedSSimStochBG2,"WHITENED-SB2");

  /* clean up, and exit */
  LALSDestroyVector(&status, &(omegaGW.data));
  for (i=0;i<2;i++){
    LALCDestroyVector(&status, &response[i]);
  }
  /* clean up valid data */
  LALSDestroyVector(&status, &(whitenedSSimStochBG1.data));
  if ( ( code = CheckStatus(&status, 0 , "", SIMULATESBTESTC_EFLS,
			  SIMULATESBTESTC_MSGEFLS) ) )
    {
      return code;
    }
  LALSDestroyVector(&status, &(whitenedSSimStochBG2.data));
  if ( ( code = CheckStatus(&status, 0 , "", SIMULATESBTESTC_EFLS,
			  SIMULATESBTESTC_MSGEFLS) ) )
    {
      return code;
    }
  LALCheckMemoryLeaks();

  printf("Output files WHITENED-SB1 and WHITENED-SB2 generated for valid data; PASS: all tests\n");


  return SIMULATESBTESTC_ENOM;
}


#if 0
/*
 * Usage ()
 *
 * Prints a usage message for program program and exits with code exitcode.
 *
 */
static void
Usage (const char *program, int exitcode)
{
  INT4 i;

  fprintf (stderr, "Usage: %s [options]\n", program);
  fprintf (stderr, "Options:\n");
  fprintf (stderr, "  -h             print this message\n");
  fprintf (stderr, "  -q             quiet: run silently\n");
  fprintf (stderr, "  -v             verbose: print extra information\n");
  fprintf (stderr, "  -d level       set lalDebugLevel to level\n");
  fprintf (stderr, "  -s siteID1     calculate simulate SB for site siteID1\n");
  fprintf (stderr, "  -t siteID2       and site siteID2\n");
  for (i=0; i<LALNumCachedDetectors; ++i)
    {
      fprintf (stderr, "                   %d = %s\n",
	       i, lalCachedDetectors[i].frDetector.name);
    }
  fprintf (stderr, "  -f f0          set start frequency to f0\n");
  fprintf (stderr, "  -e deltaT      set temporal spacing to deltaT\n");
  fprintf (stderr, "  -l length      set number of points in time series to length\n");
  fprintf (stderr, "  -a alpha       set the exponent in freq power law for Omega to alpha\n");
  fprintf (stderr, "  -r fRef        set reference freq needed to compute Omegato fRef\n");
  fprintf (stderr, "  -o omegaRef    set value of Omega at reference frequency to fRef\n");
  fprintf (stderr, "  -p filename    print simulated SB signal for site1 to file filename\n");
  fprintf (stderr, "  -q filename    print simulated SB signal for site2 to file filename\n");
  exit (exitcode);
}

/*
 * ParseOptions ()
 *
 * Parses the argc - 1 option strings in argv[].
 *
 */
static void
ParseOptions (int argc, char *argv[])
{
  while (1)
    {
      int c = -1;

      c = getopt (argc, argv, "hqvd:s:t:f:e:l:a:r:o:p:q");
      if (c == -1)
	{
	  break;
	}

      switch (c)
	{
	case 'p': /* specify output file */
	  strncpy (optFile, optarg, LALNameLength);
	  break;

	case 'l': /* specify number of points in length series */
	  optLength = atoi (optarg);
	  break;

	case 'e': /* specify temporal resolution */
	  optDeltaT = atof (optarg);
	  break;

	case 'f': /* specify start frequency */
	  optF0 = atof (optarg);
	  break;

	case 's': /* specify detector #1 */
	  optDetector1 = atoi (optarg);

	case 't': /* specify detector #2 */
	  optDetector2 = atoi (optarg);

	case 'a': /* specify alpha */
	  optAlpha = atof (optarg);
	  break;

	case 'r': /* specify reference frequency */
	  optFRef = atof (optarg);
	  break;

	case 'o': /* specify value of omega at reference frequency */
	  optOmegaRef = atof (optarg);
	  break;

	case 'd': /* set debug level */
	  lalDebugLevel = atoi (optarg);
	  break;

	case 'v': /* optVerbose */
	  optVerbose = SIMULATESBTESTC_TRUE;
	  break;

	case 'q': /* quiet: run silently (ignore error messages) */
	  freopen ("/dev/null", "w", stderr);
	  freopen ("/dev/null", "w", stdout);
	  break;

	case 'h':
	  Usage (argv[0], 0);
	  break;

	default:
	  Usage (argv[0], 1);
	}

    }

  if (optind < argc)
    {
      Usage (argv[0], 1);
    }

  return;
}
#endif
