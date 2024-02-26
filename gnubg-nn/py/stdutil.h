// -*- C++ -*-
#if !defined( STDUTIL_H )
#define STDUTIL_H

#include <assert.h>
#include <string.h>

/// Compares @arg{s1} and @arg{s2}.

inline bool
streq(const char* s1, const char* s2)
{
  {                                                       assert( s1 && s2 ); }
  return strcmp(s1, s2) == 0;
}

/// Compares first @arg{n} characters of @arg{s1} and @arg{s2}.

inline bool
strneq(const char* s1, const char* s2, int n)
{
  {                                                       assert( s1 && s2 ); }
  return strncmp(s1, s2, n) == 0;
}

/// Compares @arg{s1} and @arg{s2}, ignoring case.

inline bool
strcaseeq(const char* s1, const char* s2)
{
  {                                                       assert( s1 && s2 ); }
  return (strcasecmp(s1, s2) == 0);
}

/// Compares first @arg{n} characters of @arg{s1} and @arg{s2}, ignoring case.

inline bool
strncaseeq(const char* s1, const char* s2, int n)
{
  {                                                       assert( s1 && s2 ); }
  return (strncasecmp(s1, s2, n) == 0);
}

#endif
