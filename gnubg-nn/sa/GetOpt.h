// This may look like C code, but it is really -*- C++ -*-
/* Getopt for GNU. 
   Copyright (C) 1987, 1989, 1992 Free Software Foundation, Inc.
   (Modified by Douglas C. Schmidt for use with GNU G++.)

This file is part of the GNU C++ Library.  This library is free
software; you can redistribute it and/or modify it under the terms of
the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.  This library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/


/* This version of `getopt' appears to the caller like standard Unix `getopt'
   but it behaves differently for the user, since it allows the user
   to intersperse the options with the other arguments.

   As `getopt' works, it permutes the elements of `argv' so that,
   when it is done, all the options precede everything else.  Thus
   all application programs are extended to handle flexible argument order.

   Setting the environment variable _POSIX_OPTION_ORDER disables permutation.
   Then the behavior is completely standard.

   GNU application programs can use a third alternative mode in which
   they can distinguish the relative order of options and other arguments.  */

#ifndef GetOpt_h
#ifdef __GNUG__
#pragma interface
#endif
#define GetOpt_h 1

/// Describe the long-named options requested by the application.
//
//  The LONG_OPTIONS argument to @ref{GetOpt::GetOpt} is a vector
//  of GetOptLongOption terminated by an element containing a name which is
//  zero.
//
//  To have a long-named option do something other than set an @code{int} to
//  a compiled-in constant, such as set a value from @ref{GetOpt::optarg},
//  set the option's @ref{flag} field to zero and its @ref{val} field to a
//  nonzero value (the equivalent single-letter option character, if there is
//  one).  

struct GetOptLongOption {
  /// Long option name.
  //
  const char* name;

  ///
  enum Arg {
    /// If the option does not take an argument,
    no_argument,
    
    /// If the option requires an argument,    
    required_argument,

    /// If the option takes an optional argument.    
    optional_argument,	
  };

  ///
  Arg has_arg;

  ///
  //  If not 0, it points to a variable that is set
  //  to the value given in the field @ref{val} when the option is found, but
  //  left unchanged if the option is not found.

  int* flag;

  /// For long options that have a zero @ref{flag}, GetOpt
  /// returns the contents of the @arg{val} field.
  //
  int  val;
};

///
class GetOpt {
private:
  /// The next char to be scanned in the option-element
  /// in which the last option character we returned was found.
  //
  //  This allows us to pick up the scan where we left off.
  //
  //  If this is zero, or a null string, it means resume the scan
  //  by advancing to the next ARGV-element.
  
  static char *nextchar;
  
  
  /// Describe how to deal with options that follow non-option ARGV-elements.
  //
  //  The special argument @samp{--} forces an end of option-scanning
  //  regardless of the value of @ref{ordering}.
  //
  //  In the case of @ref{RETURN_IN_ORDER}, only
  //  @samp{--} can cause GetOpt to return EOF with @ref{optind} != ARGC.
  
   enum OrderingEnum {
     ///  Don't recognize them as options.
     //   Stop option processing when the first non-option is seen.
     //   This is what Unix does.

     REQUIRE_ORDER,

     
     ///  The default.
     //   We permute the contents of @code{argv} as we scan, so that
     //   eventually all the options are at the end.
     //
     //   This allows options to be given in any order, even with programs
     //   that were not written to expect this.
     //
     PERMUTE,

     
     /// Available to programs that were written
     /// to expect options and other ARGV-elements in any order and that care
     /// about the ordering of the two.
     //
     //  We describe each non-option ARGV-element as if it were the argument
     //  of an option with character code zero.
     //
     //  Using @samp{-} as the first character of the list of option characters
     //  requests this mode of operation.
     
     RETURN_IN_ORDER
   };

  ///
  OrderingEnum ordering;

  //@ Handle permutation of arguments.
  //
  //  Describe the part of ARGV that contains non-options that have
  //  been skipped.
  //
  //  @arg{first_nonopt} is the index in ARGV of the first of them.
  //
  //  @arg{last_nonopt} is the index after the last of them.

  ///
  static int first_nonopt;
  ///
  static int last_nonopt;
  
  void exchange(char** argv);
public:
  /// For communication from @samp{getopt} to the caller.
  //  When @samp{getopt} finds an option that takes an argument,
  //  the argument value is returned here.
  //
  //  Also, when @ref{ordering} is @ref{RETURN_IN_ORDER},
  //  each non-option ARGV-element is returned here.
  //
  
  char* optarg;
  
  /// Index in ARGV of the next element to be scanned.
  //  This is used for communication to and from the caller
  //  and for communication between successive calls to @samp{getopt}.
  //  On entry to @samp{getopt}, zero means this is the first call; initialize.
  //  
  //  When @samp{getopt} returns EOF, this is the index of the first of the
  //  non-option elements that the caller should itself scan.
  //  
  //  Otherwise, @arg{optind} communicates from one call to the next
  //  how much of ARGV has been scanned so far.
  //
  int optind;

  /// Callers store zero here to inhibit the error message
  /// for unrecognized options.  
  //
  
  int opterr;

    
  ///
  GetOpt(int                      argc,
	 char**                   argv,
	 const char*              optstring,
	 const GetOptLongOption*  lOptions = 0,
	 int*                     lind = 0,
	 int                      l_only = 0);

  ~GetOpt();
  
  /// Return next option
  //
  int operator () (void);

private:
  ///
  int    		nargc;
  ///
  char**		nargv;
  ///
  const char*		noptstring;
  ///
  const GetOptLongOption* const 
  			longOptions;
  ///
  int			long_only;
  ///
  int*			longind;
  
  /// options string created internally by constructor flag.
  //
  bool		ownsnoptstring;
};

#endif
