## $Id$
 
AM_CPPFLAGS=-I$(srcdir)/../analyze \
         -I$(srcdir)/../gnubg \
	 @PYTHON_CSPEC@

sagnubg_SOURCES = GetOpt.h GetOpt.h ../analyze/analyze.h ../analyze/bms.h ../analyze/bm.h ../analyze/equities.h ../gnubg/osr.h main.cc GetOpt.cc

sagnubg_LDADD =@PYTHON_LSPEC@ \
	       -L$(srcdir)/../analyze \
	       -L$(srcdir)/../gnubg \
	       -L$(srcdir)/../gnubg/lib \
	       ../analyze/libanalyze.la \
	       ../gnubg/libgnubg.la \
	       ../gnubg/lib/libneuralnet.la
bin_PROGRAMS = sagnubg
AM_CFLAGS = $(SSE_CFLAGS)
AM_CXXFLAGS = $(SSE_CFLAGS)
