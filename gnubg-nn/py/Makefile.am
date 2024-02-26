## $Id$
 
AM_CPPFLAGS=-I$(srcdir)/../analyze \
         -I$(srcdir)/../gnubg \
	 @PYTHON_CSPEC@

pygnubg_SOURCES = ../analyze/analyze.h ../analyze/bgdefs.h ../analyze/bm.h ../analyze/defs.h ../analyze/equities.h ../analyze/misc.h ../analyze/player.h ../gnubg/br.h ../gnubg/eval.h ../gnubg/inputs.h ../gnubg/mt19937int.h ../gnubg/osr.h ../gnubg/positionid.h pygnubg.h pynets.h pytrainer.h raceinfo.h stdutil.h pygnubg.cc pynets.cc pytrainer.cc raceinfo.cc
pygnubg_LDADD =@PYTHON_LSPEC@ \
	       -L$(srcdir)/../analyze \
	       -L$(srcdir)/../gnubg \
	       -L$(srcdir)/../gnubg/lib \
	       ../analyze/libanalyze.la \
	       ../gnubg/libgnubg.la \
	       ../gnubg/lib/libneuralnet.la
bin_PROGRAMS = pygnubg
#SYSLIBS= -ldl -lpthread -lutil
AM_CFLAGS = $(SSE_CFLAGS)
AM_CXXFLAGS = $(SSE_CFLAGS)

EXTRA_DIST = bgutil.py bgutil1.py
