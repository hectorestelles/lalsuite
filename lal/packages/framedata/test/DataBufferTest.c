/*----------------------------------------------------------------------- 
 * 
 * File Name: DataBufferTest.c
 *
 * Author: Creighton, J. D. E.
 * 
 * Revision: $Id$
 * 
 *-----------------------------------------------------------------------
 */

#ifndef _STDIO_H
#include <stdio.h>
#ifndef _STDIO_H
#define _STDIO_H
#endif
#endif

#ifndef _STRING_H
#include <string.h>
#ifndef _STRING_H
#define _STRING_H
#endif
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#ifndef _STDLIB_H
#define _STDLIB_H
#endif
#endif

#ifndef _MATH_H
#include <math.h>
#ifndef _MATH_H
#define _MATH_H
#endif
#endif

#ifndef _LALSTDLIB_H
#include "LALStdlib.h"
#ifndef _LALSTDLIB_H
#define _LALSTDLIB_H
#endif
#endif

#ifndef _AVFACTORIES_H
#include "AVFactories.h"
#ifndef _AVFACTORIES_H
#define _AVFACTORIES_H
#endif
#endif

#ifndef _DATABUFFER_H
#include "DataBuffer.h"
#ifndef _DATABUFFER_H
#define _DATABUFFER_H
#endif
#endif

#define _CODES(x) #x
#define CODES(x) _CODES(x)

NRCSID (MAIN, "$Id$");

extern char *optarg;
extern int   optind;

int   debuglevel = 1;
int   verbose    = 0;
int   output     = 0;
char *framePath  = NULL;

static void
Usage (const char *program, int exitflag);

static void
ParseOptions (int argc, char *argv[]);

static void
TestStatus (Status *status, const char *expectedCodes, int exitCode);

static void
ClearStatus (Status *status);

int
main (int argc, char *argv[])
{
  const INT4 numPoints = 65536;
  const INT4 numSpec   = 8;
  const INT4 numSegs   = 10;

  static Status            status;
  DataBuffer              *buffer = NULL;
  DataBufferPar            bufferPar;
  DataSegment              dataout;
  INT2TimeSeries           data;
  REAL4FrequencySeries     spec;
  COMPLEX8FrequencySeries  resp;
  INT4                     seg;

  ParseOptions (argc, argv);

  if (!framePath)
  {
    framePath = getenv ("LAL_FRAME_PATH");
    if (!framePath)
    {
      fprintf (stderr, "error: environment LAL_FRAME_PATH undefined\n");
      return 77;
    }
  }

  data.data = NULL;
  spec.data = NULL;
  resp.data = NULL;

  I2CreateVector (&status, &data.data, numPoints);
  TestStatus (&status, "0", 1);

  CreateVector (&status, &spec.data, numPoints/2 + 1);
  TestStatus (&status, "0", 1);

  CCreateVector (&status, &resp.data, numPoints/2 + 1);
  TestStatus (&status, "0", 1);

  dataout.data = &data;
  dataout.spec = &spec;
  dataout.resp = &resp;

  bufferPar.numSpec    = numSpec;
  bufferPar.numPoints  = numPoints;
  bufferPar.windowType = Welch;
  bufferPar.plan       = NULL;
  bufferPar.framePath  = framePath;
  EstimateFwdRealFFTPlan (&status, &bufferPar.plan, numPoints);
  TestStatus (&status, "0", 1);

  CreateDataBuffer (&status, &buffer, &bufferPar);
  TestStatus (&status, "-1 0", 1);
  ClearStatus (&status);

  for (seg = 0; seg < numSegs; ++seg)
  {
    fprintf (stderr, "Segment %2d", seg);

    GetData (&status, &dataout, 3*numPoints/4, buffer);
    TestStatus (&status, "-1 0", 1);
    ClearStatus (&status);

    if (dataout.endOfData)
    {
      fprintf (stderr, "... end of data\n");
      goto exit;
    }

    /* print data vector */
    if (output)
    {
      FILE *fp;
      CHAR  fname[64];
      INT4  i;

      sprintf (fname, "Segment.%03d", seg);
      fp = fopen (fname, "w");

      for (i = 0; i < dataout.data->data->length; ++i)
        fprintf (fp, "%u\t%d\n", i, dataout.data->data->data[i]);

      fclose (fp);
    }

    /* print spectrum */
    if (output)
    {
      FILE *fp;
      CHAR  fname[64];
      INT4  i;

      sprintf (fname, "Spectrum.%03d", seg);
      fp = fopen (fname, "w");

      for (i = 0; i < dataout.spec->data->length; ++i)
        fprintf (fp, "%u\t%e\n", i, dataout.spec->data->data[i]);

      fclose (fp);
    }

    /* print calibration info */
    if (output)
    {
      FILE *fp;
      CHAR  fname[64];
      INT4  i;

      sprintf (fname, "Response.%03d", seg);
      fp = fopen (fname, "w");

      for (i = 0; i < dataout.resp->data->length; ++i)
      {
        REAL8 re  = dataout.resp->data->data[i].re;
        REAL8 im  = dataout.resp->data->data[i].im;
        REAL8 mod = sqrt (re*re + im*im);
        REAL8 arg = atan2 (im, re);
        fprintf (fp, "%u\t%e\t%e\n", i, mod, arg);
      }

      fclose (fp);
    }

    fprintf (stderr, "\n");

  }

exit:

  DestroyRealFFTPlan (&status, &bufferPar.plan);
  TestStatus (&status, "0", 1);
  DestroyDataBuffer (&status, &buffer);
  TestStatus (&status, "0", 1);
  I2DestroyVector (&status, &data.data);
  TestStatus (&status, "0", 1);
  DestroyVector (&status, &spec.data);
  TestStatus (&status, "0", 1);
  CDestroyVector (&status, &resp.data);
  TestStatus (&status, "0", 1);

  LALCheckMemoryLeaks ();
  return 0;
}



/*
 * TestStatus ()
 *
 * Routine to check that the status code status->statusCode agrees with one of
 * the codes specified in the space-delimited string ignored; if not,
 * exit to the system with code exitcode.
 *
 */
static void
TestStatus (Status *status, const char *ignored, int exitcode)
{
  char  str[64];
  char *tok;

  if (verbose)
  {
    REPORTSTATUS (status);
  }

  if (strncpy (str, ignored, sizeof (str)))
  {
    if ((tok = strtok (str, " ")))
    {
      do
      {
        if (status->statusCode == atoi (tok))
        {
          return;
        }
      }
      while ((tok = strtok (NULL, " ")));
    }
    else
    {
      if (status->statusCode == atoi (tok))
      {
        return;
      }
    }
  }

  fprintf (stderr, "\nExiting to system with code %d\n", exitcode);
  exit (exitcode);
}


/*
 *
 * ClearStatus ()
 *
 * Recursively applies DETATCHSTATUSPTR() to status structure to destroy
 * linked list of statuses.
 *
 */
void
ClearStatus (Status *status)
{
  if (status->statusPtr)
  {
    ClearStatus      (status->statusPtr);
    DETATCHSTATUSPTR (status);
  }
}


/*
 * Usage ()
 *
 * Prints a usage message for program program and exits with code exitcode.
 *
 */
static void
Usage (const char *program, int exitcode)
{
  fprintf (stderr, "Usage: %s [options]\n", program);
  fprintf (stderr, "Options:\n");
  fprintf (stderr, "  -h         print this message\n");
  fprintf (stderr, "  -q         quiet: run silently\n");
  fprintf (stderr, "  -v         verbose: print extra information\n");
  fprintf (stderr, "  -d level   set debuglevel to level\n");
  fprintf (stderr, "  -o         output framedata to files\n");
  fprintf (stderr, "  -f dir     set frame data path to dir\n");
  fprintf (stderr, "             "
                   "(otherwise use path in environment LAL_FRAME_PATH)\n");
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

    c = getopt (argc, argv, "hqvd:""of:");
    if (c == -1)
    {
      break;
    }

    switch (c)
    {
      case 'f': /* sets frame path */
        framePath = optarg;
        break;

      case 'o': /* sets flag to write output files */
        output = 1;
        break;

      case 'd': /* set debug level */
        debuglevel = atoi (optarg);
        break;

      case 'v': /* verbose */
        ++verbose;
        break;

      case 'q': /* quiet: run silently (ignore error messages) */
        freopen ("/dev/null", "w", stderr);
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

