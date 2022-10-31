#include <cstdlib>
#include <iostream>

#include <fnmatch.h>
#include <stdio.h>
#include "shellexp.h"

using namespace std;

/* this was a C++ wrapper around C shellexpt() as
   a attempt to be more performant

   maybe the whole thing can now be ripped out
   and replace by a simplified "Non-recursive gitignore-style glob Matching"
   algorithm as seen here.

   https://www.codeproject.com/Articles/5163931/Fast-String-Matching-with-Wildcards-Globs-and-Giti

   We do not want to support the "[...]" regexp, only
   the usual '?', '*' and the special '**'.

   For now cruft/cruft-ng expect that a glob pattern from the ruleset
   never ends with '/', so to declare a whole subdirecoty structure
   one has to list:
   - /var/spool/foo
   - /var/spool/foo/ **
   There's even a check in Makefile to ensure this rule is followed.

   This rule could be reversed, so that 'xxxxx/' is a dir, and 'xxxxx' is a file.
   This would make the .postrm snippets generated by 'dh-cruft'
   more accurate:
   - 'rm -f' for files
   - 'rm -rf' for dirs


   Morevover, the only case were '**' is not at the end of the file
   is Emacs, which I don't personally use,
   but looks like support for it could be added to python.cc

   tchet@brix ~/git/cruft-ng $ grep '\*\*' rules/ * | grep -v '\*\*$'
   rules/xemacs21:/usr/share/xemacs21/site-lisp/ ** / *.elc

   So support for '**' could be dropped altogether.
   (and this is only possible now because cruft-common is gone)
*/


/* error: ‘bool myglob(const std::string&, const std::string&)’
   was declared ‘extern’ and later ‘static’
*/
bool myglob(const string& file, const string& glob )
{
	bool debug = getenv("DEBUG_GLOB") != nullptr;

	if (file==glob) return true;
	auto filesize=file.size();
	auto globsize=glob.size();
		if ( glob.find("**")==globsize-2
		  and filesize >= globsize-2
		  and file.compare(0, globsize - 2, glob) == 0) {
		if (debug) cerr << "match ** " << file << " # " << glob << '\n';
		return true;
	}  else if ( glob.find('*')==globsize-1
		  and filesize >= globsize-1
		  and file.find('/',globsize-1)==string::npos
		  and file.compare(0, globsize - 1, glob) == 0) {
		if (debug) cerr << "match * " << file << " # " << glob << '\n';
		return true;
	} else if ( fnmatch(glob.c_str(),file.c_str(),FNM_PATHNAME)==0 ) {
		if (debug) cerr << "fnmatch " << file << " # " << glob << '\n';
		return true;
	} else {
		// fallback to shellexp.c
		bool result=shellexp(file.c_str(),glob.c_str());
		if (result and debug) {
			cerr << "shellexp.c " << file << " # " << glob << '\n';
		}
		return result;
	}
}

/* this is the original function from original "cruft" project */

/* 0 on no match, non-zero on match */
int shellexp(const char* string_, const char* pattern ) {
    /*  printf( "...matching( \"%s\", \"%s\" )\n", string, pattern ); */

    switch( pattern[0] ) {
    case '\0':
	return string_[0] == '\0';
    case '?':
	switch( string_[0] ) {
	case '\0':
	case '/':
	    return false;
	default:
	    return shellexp( string_+1, pattern+1 );
	}
    case '/':
	if ( pattern[1] == '*' && pattern[2] == '*' ) {
	    const char* pch = string_;
	    if ( pattern[3] != '/' && pattern[3] != '\0' ) {
		fprintf( stderr, "Bad expression.\n" );
		return -1;
	    }
	    if ( *pch != '/' ) return false;
	    if ( pattern[3] == '\0' ) return true;
	    while ( *pch != '\0' ) {
		if ( *pch == '/' ) {
		    int ret = shellexp( pch, pattern + 3 );
		    if ( ret == true || ret == -1 )
		    	return ret;
		}
		pch++;
	    }
	    return false;
	} else if ( string_[0] == '/' ) {
		return shellexp( string_+1, pattern+1 );
	} else
		return false;
    case '*':
	if ( string_[0] == '/' ) return shellexp( string_, pattern+1 );
	{
		int ret = shellexp( string_, pattern+1 );
		if (ret == false)
			return string_[0] != '\0' ? shellexp( string_ + 1, pattern ) : false;
		else
			return ret;
	}
    case '[':
	if ( string_[0] == '\0' ) return false;
	{
	    int nott = false;
	    int okay = false;
	    pattern++;
	    if ( pattern[0] == '!' || pattern[0] == '^' ) {
		nott = true;
		pattern++;
	    }

	    if ( pattern[0] == ']' ) {
		if ( string_[0] == ']' ) { okay = true; }
		pattern++;
	    }

	    while( pattern[0] != ']' && pattern[0] != '\0' ) {
		if ( pattern[0] == string_[0] ) {
		    okay = true;
		} else if ( pattern[1] == '-' && pattern[2] != ']' ) {
		    if ( pattern[0] <= string_[0] && string_[0] <= pattern[2] ) {
			okay = true;
		    }
		    pattern += 2;
		}
		pattern++;
	    }

	    if ( pattern[0] == '\0' ) {
		fprintf( stderr, "Bad shell expression\n" );
		return -1;
	    }

	    if (! (nott ? !okay : okay))
		return false;
	    else
		return shellexp( string_ + 1, pattern + 1 );
	}
    default:
	if (pattern[0] != string_[0])
		return false;
	else
		return shellexp( string_ + 1, pattern + 1 );
    }
}
