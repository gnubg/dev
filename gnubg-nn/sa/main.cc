/*@top GNU rollout program

@set prog @code{sagnubg}

@section Usage
@example
@value{prog} [flags] [cmd-file] [output-file]
@end

Process @file{cmd-file} and write results to @file{output-file}.
@file{cmd-file} and @file{output-file} default to stdin and stdout
respectively.

@section Command Line Flags

@multitable @code
@c @caption  command line flags

@header flag		full name	description

@item   -w	--weights-dir=@var{FILE} @tab
Explicitly specify name of NN's files. Default is to use the environment
variable @code{GNUBGWEIGHTS} if it exists, otherwise use
@file{gnubg.weights} in the same directory as the executable.

@item @tab --moves2p-limit=@var{N} @tab
Maximum number of moves to keep after 0-ply pruning. Default is 20.

@item @tab --rollout-limit=@var{N} @tab
Maximum number of moves to rollout. Default is 5.

@item @tab --rollout-games=@var{N} @tab
Number of games per rollout. Default is 1296.

@item @tab --cube-away=@var{N} @tab
Use (N,N) away for cube decisions. Default is 7.

@item @tab --include-ply0=1/0 @tab
Force inclusion of top 0-ply move. Default is 1.

@item @tab --eval-plies=@var{N} @tab
Ply used in evaluation (@samp{e} command).

@item @tab --n-osr=@var{N} @tab
Number of games for OSR evaluation (@samp{O} command).

@item @tab --no-shortcuts @tab
Disable usage of small net pruning.

@item @tab --resume @tab
Resume an interrupted run. Both @file{cmd-file} and @file{output-file} must be
given.

@item -v	--verbose @tab Print status report to stderr during run.

@item -h	--help
@end

@section Command File Format

Command file is a simple ASCII file. Each command occupies one line, where the
first character specifies command type.

@table @code
@item # @dots{}
Comment. Echoed to output.

@item s @var{SETTINGS}
Change run settings in the middle.
@example
s nRollOutGames 12960
@end

@item r @var{N}
Set randomizer seed to @var{N} for next @samp{m} or @samp{o} command.
Echoed to output. 

@item m @var{position} @var{@eqn{dice_0}} @var{@eqn{dice_1}}
Rollout top 2-ply moves. Output is

@example
m @var{position} @var{@eqn{dice_0}} @var{@eqn{dice_1}} @var{@eqn{move_1}}@
@var{@eqn{eqt}} @var{@eqn{move_2}} @var{@eqn{dif_2}} @var{@eqn{move_3}}@
@var{@eqn{dif_3}} @dots{}
@end

@var{eqt} is the (money) equity of the best move, and @eqn{dif_i} if the
equity difference of @eqn{move_i} from best.

@item c @var{position}

Perform Cube-full Rollout of @var{position} with centered cube and 2 cube
follwing a Double/Take. Output is

@example
c @var{position} @eqn{P_nd P_dt} @var{action}
@end

@eqn{P_nd} and @eqn{P_dt} are match win%, in the range 0 to 100. @var{action}
is one of @samp{TG},@samp{D/T},@samp{D/D} or @samp{ND}.

@item o @var{position}
Rollout position. Output is
@example
o position @eqn{p_0 p_1 p_2 p_3 p_4}
@end
Where the @eqn{p_i}'s are the GNUbg net evaluation probabilities.

@item e @var{position}
Evaluate @var{position} using the net.

Output is,
@example
e position @eqn{p_0 p_1 p_2 p_3 p_4}
@end

@item O @var{position}
Evaluate race @var{position} using the OSR method.

Output is,
@example
O position @eqn{p_0 p_1 p_2 p_3 p_4}
@end

@item b @var{position} @var{@eqn{dice_0}} @var{@eqn{dice_1}}
Find evaluation best move.

Output is,
@example
b position @eqn{dice_0} @eqn{dice_1} move @eqn{p_0 p_1 p_2 p_3 p_4}
@end

@var{move} is the position after the move, @var{@eqn{p_i}} are the position
evaluation.

@end table

Note that output has the same format as the input, i.e. can serve as input for
another run.

The positions are encoded as 20 character in the range of @samp{A} to @samp{F}.
Each letter stands for 4 bits (A == 0x0, F == 0xf), and concatenated they form
the 80 bit GNUbg position ID.

*/
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>
#include <iostream>
#include <fstream>

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 1
#define GCC3
#endif

#if defined(GCC3)
#include <sstream>
#else
#include <strstream>
#endif

#include <algorithm>
#include <string>

#include "GetOpt.h"
#include <analyze.h>
#include <bms.h>
#include <bm.h>
#include <equities.h>
#include <osr.h>

#if defined(WIN32)
// on windows
#define random rand
#define srandom srand
#endif

using std::endl;
using std::cerr;
using std::cin;
using std::cout;

using std::string;

using std::min;
using std::sort;

using std::istream;
using std::ostream;
using std::ifstream;
using std::ofstream;

namespace {
const char* const version = "1.93";

bool
validBoard(const char s[21])
{
   if( strlen(s) != 20 ) {
    return false;
  }
  
  for(uint i = 0; i < 20; ++i) {
    if( ! ('A' <= s[i] && s[i] < ('A' + 16)) ) {
      return false;
    }
  }
  
  return true;
}
  
unsigned char*
auchFromSring(const char s[20])
{
  static unsigned char auch[10];

  for(uint i = 0; i < 10; ++i) {
    auch[i] = ((s[2*i+0] - 'A') << 4) +  (s[2*i+1] - 'A');
  }

  return auch;
}

const char*
posFromAuch(const unsigned char* const auch)
{
  static char p[21];

  for(uint i = 0; i < 10; ++i) {
    p[2*i+0] = 'A' + ((auch[i] >> 4) & 0xf);
    p[2*i+1] = 'A' + (auch[i] & 0xf);
  }
  p[20] = 0;

  return p;
}


void
fortify(movelist& ml)
{
  move* m = new move [ml.cMoves];
  memcpy(m, ml.amMoves, ml.cMoves * sizeof(move));
  ml.amMoves = m;
}

}

class SortScore {
public:
  int	operator ()(move const& i1, move const& i2) const;
};


inline int
SortScore::operator ()(move const& i1, move const& i2) const 
{
  return i1.rScore > i2.rScore;
}


static uint const N_M2P = 257;
static uint const N_RL = 258;
static uint const N_RG = 259;
static uint const N_CA = 260;

static uint const N_I0 = 261;
static uint const N_RS = 262;

static uint const N_EP = 263;
static uint const N_SC = 264;
static uint const N_OG = 265;
static uint const N_OSRO = 266;

static const GetOptLongOption
longOpt[] =
{
  { "weights-dir",	GetOptLongOption::required_argument,	0, 'w' } ,

  { "moves2p-limit",    GetOptLongOption::required_argument,    0,  N_M2P } ,
  { "rollout-limit",    GetOptLongOption::required_argument,    0,  N_RL } ,
  { "rollout-games",    GetOptLongOption::required_argument,    0,  N_RG } ,
  { "cube-away",        GetOptLongOption::required_argument,    0,  N_CA } ,

  { "include-ply0",     GetOptLongOption::required_argument,    0,  N_I0 } ,
  { "eval-plies",       GetOptLongOption::required_argument,    0,  N_EP } ,
  { "no-shortcuts",     GetOptLongOption::no_argument,          0,  N_SC },
  { "n-osr",            GetOptLongOption::required_argument,    0,  N_OG },
  { "osr-in-roll",      GetOptLongOption::required_argument,    0,  N_OSRO },
  { "resume",           GetOptLongOption::no_argument,          0,  N_RS } ,
  
  { "verbose",		GetOptLongOption::optional_argument,	0, 'v' } , 
  { "help",		GetOptLongOption::no_argument,	        0, 'h' } , 
 
  {0, GetOptLongOption::no_argument , 0, 0}
};

static void
usage(const char* const p)
{
  cerr << "usage: " << p << " [flags] [cmd-file] [output-file]" << endl
       << endl
       << "Flags: " << endl
       << "  -w DIR,--weights-dir=DIR  directory of 'gnubg.weights'" << endl
       << "  --moves2p-limit=N         Max number of moves for initial "
          "2ply pruning." << endl
       << "  --rollout-limit=N         Max number of moves to rollout." << endl
       << "  --rollout-games=N         Number of games per rollout." << endl
       << "  --cube-away=N             Use (N,N) away for cube decisions."
       << "  --eval-plies=N            Evaluate using N-ply."
       << endl
       << "  --resume                  Resume an interrupted session."
          " (both 'cmd-file' and 'output-file' must be given)."
       << endl
       << "  -v,--verbose=N            Verbosity level. Print progress report"
          " to stderr." << endl
       << endl
       << " If 'output-file' is not given, write to stdout." << endl
       << " If 'cmd-file' is not given, reads from stdin." << endl;
}

int
main(int argc, char* argv[])
{
  const char* wFile = "";
  uint moves2plyLimit = 20;
  uint rolloutLimit = 5;
  uint nRollOutGames = 1296;
  uint cubeAway = 7;
  uint evalPlies = 2;
  bool shortCuts = true;
  uint osrGames = 1296;
  int  osrInRoll = 1;
  
  bool include0Ply = true;
  bool resume = false;
  
  int verbose = 0;
  
  GetOpt opt(argc, argv, 0, longOpt);

  int optionChar;
  
  while( (optionChar = opt()) != EOF ) {
    switch( optionChar ) {
      case 'w':
      {
	wFile = opt.optarg;
	break;
      }
      case 'v':
      {
	verbose = opt.optarg ? atoi(opt.optarg) : 1;
	break;
      }
      case N_M2P:
      {
	moves2plyLimit = atoi(opt.optarg);
	break;
      }
      case N_RL:
      {
	rolloutLimit = atoi(opt.optarg);
	break;
      }
      case N_RG:
      {
	nRollOutGames = atoi(opt.optarg);
	break;
      }
      case N_CA:
      {
	cubeAway = atoi(opt.optarg);

	if( ! (1 < cubeAway && cubeAway <= 25) ) {
	  exit(1);
	}
	break;
      }
      case N_I0:
      {
	include0Ply = atoi(opt.optarg);
	
	break;
      }
      case N_RS:
      {
	resume = true;
	break;
      }
      case N_EP:
      {
	int i = atoi(opt.optarg);

	if( i < 0 ) {
	  cerr << endl << "negative plies" << endl;
	  exit(1);
	}
	evalPlies = (unsigned int) i;
	break;
      }
      case N_OG:
      {
	int i = atoi(opt.optarg);

	if( i <= 0 ) {
	  cerr << endl << "non positive osrGames" << endl;
	  exit(1);
	}
	osrGames = (unsigned int) i;
	break;
      }
      case N_OSRO:
      {
	osrInRoll = atoi(opt.optarg);

	if( !( osrInRoll == 0 || osrInRoll == 1) ) {
	  cerr << endl << "osrInRoll must be either 0 or 1" << endl;
	  exit(1);
	}
	break;
      }
      case N_SC:
      {
	shortCuts = false;
	break;
      }
      case 'h':
      default:
      {
	usage(argv[0]);
	// error message already given
	exit(1);
      }
    }
  }

  const char* const weightsVersion = Analyze::init(wFile, shortCuts);
  
  if( ! weightsVersion ) {
    cerr << endl << "failed to initalize GNU bg" << endl;
    
    exit(1);
  }

  setPlyBounds(evalPlies, moves2plyLimit, 0, 0.0);
  
  srandom(time(0));

  useSSE(1);
  
#if defined( WIN32 )
#undef srandom
#endif
  
  const char* inpFile = 0;
  const char* outFile = 0;

  istream* in = &cin;
  ostream* out = &cout;
  
  if( opt.optind < argc ) {
    inpFile = argv[opt.optind];
    in = new ifstream(inpFile);

    if( ! in->good() ) {
      cerr << "failed to open '" << inpFile << "'" << endl;
      exit(1);
    }
  }
  
  if( opt.optind + 1 < argc ) {
    outFile = argv[opt.optind + 1];
    if( resume ) {
#if defined(GCC3)
      typedef std::ios_base b;
#else
      typedef ios b;
#endif
      
      out = new ofstream(outFile, b::out|b::app);
    } else {
      out = new ofstream(outFile);
    }
    
    if( ! out->good() ) {
      cerr << "failed to open '" << outFile << "'" << endl;
      exit(1);
    }
  }

  Analyze analyzer;

  Analyze::R1 ad(nRollOutGames);
  
  if( resume ) {
    if( !inpFile || !outFile ) {
      cerr << " --resume requires both input & output files" << endl;
      exit(1);
    }

    string line;
    string lastLine;
    {
      ifstream o(outFile);
    
      while( ! o.eof() ) {

	getline(o, line);

	if( ! o.eof() && ! o.good() ) {
	  perror("read failed");
	  exit(1);
	}
    
	if( o.fail() ) {
	  break;
	}

	char const c = line[0];
	switch( c ) {
	  case 's':
	  {
#if defined(GCC3)
	    std::istringstream sline(line);
#else
	    istrstream sline(line.c_str());
#endif
	    char opr;
	    sline >> opr;

	    // shamefull replication
	    string option;
	    string value;

	    // *out << "s ";
	
	    while( 1 ) {
	      sline >> option >> value;
	      if( sline.fail() ) {
		break;
	      }

	      if( option == "version" ) {
		// ignore
		continue;
	      }
	      if( option == "weights" ) {
		// ignore
		continue;
	      }
	  
	      if( option == "moves2plyLimit" ) {
		moves2plyLimit = atoi(value.c_str());
	      } else if( option == "rolloutLimit" ) {
		rolloutLimit = atoi(value.c_str());
	      } else if( option == "nRollOutGames" ) {
		nRollOutGames = atoi(value.c_str());
		ad.nRolloutGames = nRollOutGames;
	      } else if( option == "cubeAway" ) {
		cubeAway = atoi(value.c_str());
	      } else if( option == "include0Ply" ) {
		include0Ply = atoi(value.c_str());
	      } else if( option == "evalPlies" ) {
		evalPlies = atoi(value.c_str());
	      } else if( option == "shortCuts" ) {
		shortCuts = atoi(value.c_str());
	    
		setShortCuts(shortCuts);
	      } else if( option == "osrGames" ) {
		osrGames = atoi(value.c_str());
	      } else if( option == "osrInRoll" ) {
		osrInRoll = atoi(value.c_str());
	      } else {
		cerr << "Unknown option " << option << endl;
		exit(1);
	      }
	      // *out << option << " "  << value;
	    }

	    // set to new values if changed
	    setPlyBounds(evalPlies, moves2plyLimit, 0, 0.0);
	
	    //*out << endl;
	
	    break;
	  }
	  case 'm':
	  case 'c':
	  case 'o':
	  case 'b':
	  case 'e':
	  case 'O':
	  {
	    lastLine = line;
	    break;
	  }
	  default: break;
	}
      }
    }
    
    if( lastLine.length() ) {
      uint const s = lastLine.length();
      
      while( ! (*in).eof() ) {

	getline(*in, line);

	if( ! (*in).eof() && ! (*in).good() ) {
	  perror("read failed (input)");
	  exit(1);
	}
    
	if( (*in).fail() ) {
	  cerr << "Resume failed:"
	       << "Failed to find supposed last line in input file.";
	  exit(1);
	}

	if( line.length() <= s &&
	    lastLine.substr(0, line.length()) == line ) {
	  *out << "#Resumed" << endl;
	  break;
	}
      }
    }
  }
  
  // Output full setup

  *out << "s"
       << " version "        << version
       << " weights "        << weightsVersion
       << " moves2plyLimit " << moves2plyLimit
       << " rolloutLimit "   << rolloutLimit
       << " nRollOutGames "  << nRollOutGames
       << " cubeAway "       << cubeAway
       << " include0Ply "    << include0Ply
       << " evalPlies "      << evalPlies
       << " shortCuts "      << shortCuts
       << " osrGames "       << osrGames
       << " osrInRoll "      << osrInRoll
       << endl;
  

  // no need for cubeless rollout of position
  ad.nPlies = 0;
  ad.rollOutProbs = false;
  
  // 
  Analyze::useOSRinRollouts = osrInRoll;
  
  char b[21];
  uint d[2];
  int board[2][25];
  int btmp[2][25];
  float p[5];
  char opr;
  
  string line;
  long rseed = 0;
  
  while( ! (*in).eof() ) {
    b[0] = 0;
    d[0] = d[1] = 0;

    getline(*in, line);

    if( ! (*in).eof() && ! (*in).good() ) {
      perror("read failed");
      exit(1);
    }
    
    if( (*in).fail() ) {
      break;
    }

#if defined(GCC3)
    std::istringstream sline(line);
#else
    istrstream sline(line.c_str());
#endif
    
    sline >> opr;

    if( sline.fail() ) {
      // empty line, continue
      continue;
    }

    switch( opr ) {
      case 's':
      {
	string option;
	string value;

	*out << "s ";
	
	while( 1 ) {
	  sline >> option >> value;
	  if( sline.fail() ) {
	    break;
	  }

	  if( option == "version" ) {
	    // ignore
	    continue;
	  }
	  if( option == "weights" ) {
	    // ignore
	    continue;
	  }
	  
	  if( option == "moves2plyLimit" ) {
	    moves2plyLimit = atoi(value.c_str());
	  } else if( option == "rolloutLimit" ) {
	    rolloutLimit = atoi(value.c_str());
	  } else if( option == "nRollOutGames" ) {
	    nRollOutGames = atoi(value.c_str());
	    ad.nRolloutGames = nRollOutGames;
	  } else if( option == "cubeAway" ) {
	    cubeAway = atoi(value.c_str());
	  } else if( option == "include0Ply" ) {
	    include0Ply = atoi(value.c_str());
	  } else if( option == "evalPlies" ) {
	    evalPlies = atoi(value.c_str());
	  } else if( option == "shortCuts" ) {
	    shortCuts = atoi(value.c_str());
	    
	    setShortCuts(shortCuts);
	  } else if( option == "osrGames" ) {
	    osrGames = atoi(value.c_str());
          } else if( option == "osrInRoll" ) {
            osrInRoll = atoi(value.c_str());
	  } else {
	    cerr << "Unknown option " << option << endl;
	    exit(1);
	  }
	  *out << option << " "  << value << " ";
	}

	// set to new values if changed
	setPlyBounds(evalPlies, moves2plyLimit, 0, 0.0);
	
	*out << endl;
	
	break;
      }
      case 'r':
      {
	sline >> rseed;

	if( sline.fail() ) {
	  cerr << "Illegal line '" << line << "'" << endl;
	  exit(1);
	}

	Analyze::srandom(rseed);

	*out << line << endl;
	
	break;
      }
      case 'm':
      {
	sline >> b >> d[0] >> d[1];
	
	if( sline.fail() ) {
	  cerr << "Illegal line '" << line << "'" << endl;
	  exit(1);
	}
	
	if( ! (validBoard(b)  && ((1 <= d[0] && d[0] <= 6)
				  && (1 <= d[1] && d[1] <= 6))) ) {
	  cerr << "Illegal line '" << b << "' " << d[0] << ' ' << d[1] << endl;
	  exit(1);
	}

	if( rseed == 0 ) {
	  rseed = random();
	  *out << "r " << rseed << endl;
	}

	movelist ml;
	float maxDiff = 2.0;

	PositionFromKey(board, auchFromSring(b));

	Analyze::RolloutEndsAt target = analyzer.rolloutTarget(board);
	
	findBestMoves(ml, 0, d[0], d[1], board, 0, false, moves2plyLimit, 5.0);

	fortify(ml);

	for(uint k = 0; k < ml.cMoves; ++k) {
	  move& mv = ml.amMoves[k];

	  PositionFromKey(btmp, mv.auch);
	  SwapSides(btmp);
	  // change auch to op side from now on
	  PositionKey(btmp, mv.auch);

	  if( verbose ) {
	    cerr << "Evaluating 2ply " << posFromAuch(mv.auch);
	  }
	  	  
	  EvaluatePosition(btmp, p, 2, 0, 0, 0, 0, 0);

	  mv.rScore = -Equities::money(p);
	  
	  if( verbose ) {
	    cerr << " - " << mv.rScore << endl;
	  }
	}

	uint const offset = include0Ply ? 1 : 0;
	
	// keep 0ply move (if required) always in by excluding it from sort
	sort(ml.amMoves + offset,
	     ml.amMoves + (ml.cMoves-offset), SortScore());

	if( include0Ply && rolloutLimit < ml.cMoves &&
	    ml.amMoves[rolloutLimit].rScore >= ml.amMoves[0].rScore ) {
	  ml.cMoves = rolloutLimit + 1;
	} else {
	  ml.cMoves = min(ml.cMoves, int(rolloutLimit));
	}
	
	for(uint k = 0; k < ml.cMoves; ++k) {
	  move& mv = ml.amMoves[k];

	  PositionFromKey(btmp, mv.auch);

	  if( verbose ) {
	    cerr << "Rollout (" << nRollOutGames << ") "
		 << posFromAuch(mv.auch);
	  }

	  Analyze::srandom(rseed);
          analyzer.rollout(btmp, false, p, 0, 0, 512, nRollOutGames, k, target);

	  mv.rScore = -Equities::money(p);

	  if( verbose ) {
	    cerr << " - " << mv.rScore << endl;
	  }

	  *out << "#R " << posFromAuch(mv.auch);
	  for(uint k = 0; k < 5; ++k) {
	    *out << ' ' << p[k];
	  }
	  *out << endl;
	  
	}
    
	sort(ml.amMoves, ml.amMoves + ml.cMoves, SortScore());

	// Output args + moves

	*out << "m " << b << ' ' << d[0] << ' ' << d[1];
	for(uint k = 0; k < ml.cMoves; ++k) {
	  move& mv = ml.amMoves[k];

	  PositionFromKey(btmp, mv.auch);
	  
	  *out << ' ' << posFromAuch(mv.auch) << ' '
	       << (k == 0 ? mv.rScore : (ml.amMoves[0].rScore - mv.rScore));
	}
	*out << endl;
	
	delete [] ml.amMoves;

	// reset seed
	rseed = 0;
	
	break;
      }
      case 'e':
      case 'O':
      {
	sline >> b;
	
	if( sline.fail() || !validBoard(b) ) {
	  cerr << "Illegal line '" << line << "'" << endl;
	  exit(1);
	}

	PositionFromKey(board, auchFromSring(b));

	float p[5];

	if( opr == 'e' ) {
	  EvaluatePosition(board, p, evalPlies, 0, 0, 0, 0, 0);
	} else {
	  raceProbs(board, p, osrGames);
	}
	  
	*out << opr << " " << b;
	for(uint k = 0; k < 5; ++k) {
	  *out << ' ' << p[k];
	}
	*out << endl;
	
	break;
      }
      case 'b':
      {
	sline >> b >> d[0] >> d[1];
	
	if( sline.fail() ) {
	  cerr << "Illegal line '" << line << "'" << endl;
	  exit(1);
	}
	
	if( ! (validBoard(b)  && ((1 <= d[0] && d[0] <= 6)
				  && (1 <= d[1] && d[1] <= 6))) ) {
	  cerr << "Illegal line '" << b << "' " << d[0] << ' ' << d[1] << endl;
	  exit(1);
	}

	PositionFromKey(board, auchFromSring(b));
	
	int const m = findBestMove(0, d[0], d[1], board, false, evalPlies);

	SwapSides(board);
	
	unsigned char auch[10];
	PositionKey(board, auch);

	float p[5];
	EvaluatePosition(board, p, evalPlies, 0, 0, 0, 0, 0);

	*out << opr << " " << b << " " << d[0] << " " << d[1] << " "
	     << posFromAuch(auch);
	
	for(uint k = 0; k < 5; ++k) {
	  *out << ' ' << p[k];
	}
	*out << endl;
	
	break;
      }
      case 'c':
      case 'o':
      {
	sline >> b;
	
	if( sline.fail() || !validBoard(b) ) {
	  cerr << "Illegal line '" << line << "'" << endl;
	  exit(1);
	}
	
	if( rseed == 0 ) {
	  rseed = random();
	  *out << "r " << rseed << endl;
	}

	Analyze::srandom(rseed);

	PositionFromKey(board, auchFromSring(b));

	if( opr == 'c' ) {
	  if( verbose ) {
	    cerr << "Cube Rollout (" << nRollOutGames << ") " << b << endl;
	  }
	
	  analyzer.setScore(cubeAway, cubeAway);

	  analyzer.analyze(ad, board, false, 0, 0);

	  analyzer.setScore(0, 0);

	  *out << opr << " " << b
	       << " " << 100 * ad.matchProbNoDouble
	       << " " << 100 * ad.matchProbDoubleTake
	       << " "
	       << (ad.tooGood ? "TG" :
		   (ad.actionDouble ? (ad.actionTake ? "D/T" : "D/D") : "ND"))
	       << endl;
	
	} else {
	  if( verbose ) {
	    cerr << "Rollout (" << nRollOutGames << ") " << b << endl;
	  }

	  //analyzer.rollout(board, false, p, 0, 1024, nRollOutGames, 0);
	  analyzer.rollout(board, false, p, 0, 0, 1024, nRollOutGames, 0);
	  
	  *out << opr << " " << b;
	  for(uint k = 0; k < 5; ++k) {
	    *out << ' ' << p[k];
	  }
	  *out << endl;
	}
	
	rseed = 0;
	
	break;
      }
      case '#':
      default:
      {
	// echo comment or any other lines
	*out << line << endl;
	break;
      }
    }
  }

  if( out != &cout ) {
    delete out;
  }

  if( in != &cin ) {
    delete in;
  }
  
  return 0;
}
