 /* -------------------------------------------------------------------------- **
 * Microsoft Network Services for Unix, AKA., Andrew Tridgell's SAMBA.
 *
 * This module Copyright (C) 1990-1998 Karl Auer
 *
 * Rewritten almost completely by Christopher R. Hertel
 * at the University of Minnesota, September, 1997.
 * This module Copyright (C) 1997-1998 by the University of Minnesota
 * -------------------------------------------------------------------------- **
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * -------------------------------------------------------------------------- **
 * Module name: params
 *
 * -------------------------------------------------------------------------- **
 *
 *  This module performs lexical analysis and initial parsing of a
 *  Windows-like parameter file.  It recognizes and handles four token
 *  types:  section-name, parameter-name, parameter-value, and
 *  end-of-file.  Comments and line continuation are handled
 *  internally.
 *
 *  The entry point to the module is function pm_process().  This
 *  function opens the source file, calls the Parse() function to parse
 *  the input, and then closes the file when either the EOF is reached
 *  or a fatal error is encountered.
 *
 *  A sample parameter file might look like this:
 *
 *  [section one]
 *  parameter one = value string
 *  parameter two = another value
 *  [section two]
 *  new parameter = some value or t'other
 *
 *  The parameter file is divided into sections by section headers:
 *  section names enclosed in square brackets (eg. [section one]).
 *  Each section contains parameter lines, each of which consist of a
 *  parameter name and value delimited by an equal sign.  Roughly, the
 *  syntax is:
 *
 *    <file>            :==  { <section> } EOF
 *
 *    <section>         :==  <section header> { <parameter line> }
 *
 *    <section header>  :==  '[' NAME ']'
 *
 *    <parameter line>  :==  NAME '=' VALUE '\n'
 *
 *  Blank lines and comment lines are ignored.  Comment lines are lines
 *  beginning with either a semicolon (';') or a pound sign ('#').
 *
 *  All whitespace in section names and parameter names is compressed
 *  to single spaces.  Leading and trailing whitespace is stipped from
 *  both names and values.
 *
 *  Only the first equals sign in a parameter line is significant.
 *  Parameter values may contain equals signs, square brackets and
 *  semicolons.  Internal whitespace is retained in parameter values,
 *  with the exception of the '\r' character, which is stripped for
 *  historic reasons.  Parameter names may not start with a left square
 *  bracket, an equal sign, a pound sign, or a semicolon, because these
 *  are used to identify other tokens.
 *
 * -------------------------------------------------------------------------- **
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "param.h"

/* we can't use FILE* due to the 256 fd limit - use this cheap hack
   instead */
typedef struct {
	char *buf;		/* buffer to store file content */
	size_t buflen;	/* buffer size */
	
	char *bufr;		/* buffer to read a line from buf */
	size_t bufrlen;	/* buffer size */
	
	char *p;		/* pointer to file content */
	size_t size;	/* file size */
} myFILE;


static int mygetc(myFILE *f)
{
	if (f->p >= f->buf+f->size) return EOF;
        /* be sure to return chars >127 as positive values */
	return (int)( *(f->p++) & 0x00FF );
}

static void myfile_close(myFILE *f)
{
	;
}

/* -------------------------------------------------------------------------- **
 * Functions...
 */

static int EatWhitespace( myFILE *InFile )
  /* ------------------------------------------------------------------------ **
   * Scan past whitespace (see ctype(3C)) and return the first non-whitespace
   * character, or newline, or EOF.
   *
   *  Input:  InFile  - Input source.
   *
   *  Output: The next non-whitespace character in the input stream.
   *
   *  Notes:  Because the config files use a line-oriented grammar, we
   *          explicitly exclude the newline character from the list of
   *          whitespace characters.
   *        - Note that both EOF (-1) and the nul character ('\0') are
   *          considered end-of-file markers.
   *
   * ------------------------------------------------------------------------ **
   */
{
  int c;

  for( c = mygetc( InFile ); isspace( c ) && ('\n' != c); c = mygetc( InFile ) )
    ;
  return( c );
} /* EatWhitespace */

static int EatComment( myFILE *InFile )
  /* ------------------------------------------------------------------------ **
   * Scan to the end of a comment.
   *
   *  Input:  InFile  - Input source.
   *
   *  Output: The character that marks the end of the comment.  Normally,
   *          this will be a newline, but it *might* be an EOF.
   *
   *  Notes:  Because the config files use a line-oriented grammar, we
   *          explicitly exclude the newline character from the list of
   *          whitespace characters.
   *        - Note that both EOF (-1) and the nul character ('\0') are
   *          considered end-of-file markers.
   *
   * ------------------------------------------------------------------------ **
   */
{
  int c;

  for( c = mygetc( InFile ); ('\n'!=c) && (EOF!=c) && (c>0); c = mygetc( InFile ) )
    ;
  return( c );
} /* EatComment */

/*****************************************************************************
 * Scan backards within a string to discover if the last non-whitespace
 * character is a line-continuation character ('\\').
 *
 *  Input:  line  - A pointer to a buffer containing the string to be
 *                  scanned.
 *          pos   - This is taken to be the offset of the end of the
 *                  string.  This position is *not* scanned.
 *
 *  Output: The offset of the '\\' character if it was found, or -1 to
 *          indicate that it was not.
 *
 *****************************************************************************/

static int Continuation(char *line, int pos )
{
	pos--;
	while( (pos >= 0) && isspace((int)line[pos]))
		pos--;

	return (((pos >= 0) && ('\\' == line[pos])) ? pos : -1 );
}


static int Section( myFILE *InFile, 
	int (*sfunc)(const char *) )
  /* ------------------------------------------------------------------------ **
   * Scan a section name, and pass the name to function sfunc().
   *
   *  Input:  InFile  - Input source.
   *          sfunc   - Pointer to the function to be called if the section
   *                    name is successfully read.
   *
   *  Output: 0 if the section name was read and 0 was returned from
   *          <sfunc>.  -1 if <sfunc> failed or if a lexical error was
   *          encountered.
   *
   * ------------------------------------------------------------------------ **
   */
{
  int   c;
  int   i;
  int   end;


  i = 0;      /* <i> is the offset of the next free byte in bufr[] and  */
  end = 0;    /* <end> is the current "end of string" offset.  In most  */
              /* cases these will be the same, but if the last          */
              /* character written to bufr[] is a space, then <end>     */
              /* will be one less than <i>.                             */

  c = EatWhitespace( InFile );    /* We've already got the '['.  Scan */
                                  /* past initial white space.        */

  while( (EOF != c) && (c > 0) )
    {

    /* Check that the buffer is big enough for the next character. */
    if( i > (InFile->bufrlen - 2) )
    {
		return -1;
	}

    /* Handle a single character. */
    switch( c )
      {
      case ']':                       /* Found the closing bracket.         */
        InFile->bufr[end] = '\0';
        if( 0 == end )                  /* Don't allow an empty name.       */
          {
          return( -1 );
          }

        if( -1 == sfunc(InFile->bufr) )            /* Got a valid name.  Deal with it. */
          return( -1 );

        (void)EatComment( InFile );     /* Finish off the line.             */
        return( 0 );

      case '\n':                      /* Got newline before closing ']'.    */
        i = Continuation( InFile->bufr, i );    /* Check for line continuation.     */
        if( i < 0 )
          {
          InFile->bufr[end] = '\0';
          return( -1 );
          }
        end = ( (i > 0) && (' ' == InFile->bufr[i - 1]) ) ? (i - 1) : (i);
        c = mygetc( InFile );             /* Continue with next line.         */
        break;

      default:                        /* All else are a valid name chars.   */
        if( isspace( c ) )              /* One space per whitespace region. */
          {
          InFile->bufr[end] = ' ';
          i = end + 1;
          c = EatWhitespace( InFile );
          }
        else                            /* All others copy verbatim.        */
          {
          InFile->bufr[i++] = c;
          end = i;
          c = mygetc( InFile );
          }
      }
    }

  /* We arrive here if we've met the EOF before the closing bracket. */
  return( -1 );
} /* Section */

static int Parameter( myFILE *InFile,
	int (*pfunc)(const char *, const char *),
	int c )
  /* ------------------------------------------------------------------------ **
   * Scan a parameter name and value, and pass these two fields to pfunc().
   *
   *  Input:  InFile  - The input source.
   *          pfunc   - A pointer to the function that will be called to
   *                    process the parameter, once it has been scanned.
   *          c       - The first character of the parameter name, which
   *                    would have been read by Parse().  Unlike a comment
   *                    line or a section header, there is no lead-in
   *                    character that can be discarded.
   *
   *  Output: 0 if the parameter name and value were scanned and processed
   *          successfully, else -1.
   *
   *  Notes:  This function is in two parts.  The first loop scans the
   *          parameter name.  Internal whitespace is compressed, and an
   *          equal sign (=) terminates the token.  Leading and trailing
   *          whitespace is discarded.  The second loop scans the parameter
   *          value.  When both have been successfully identified, they are
   *          passed to pfunc() for processing.
   *
   * ------------------------------------------------------------------------ **
   */
{
  int   i       = 0;    /* Position within bufr. */
  int   end     = 0;    /* bufr[end] is current end-of-string. */
  int   vstart  = 0;    /* Starting position of the parameter value. */


  /* Read the parameter name. */
  while( 0 == vstart )  /* Loop until we've found the start of the value. */
    {

    if( i > (InFile->bufrlen - 2) )       /* Ensure there's space for next char.    */
      {     
        return( -1 );
      }

    switch( c )
      {
      case '=':                 /* Equal sign marks end of param name. */
        if( 0 == end )              /* Don't allow an empty name.      */
          {
          return( -1 );
          }
        InFile->bufr[end++] = '\0';         /* Mark end of string & advance.   */
        i       = end;              /* New string starts here.         */
        vstart  = end;              /* New string is parameter value.  */
        InFile->bufr[i] = '\0';             /* New string is nul, for now.     */
        break;

      case '\n':                /* Find continuation char, else error. */
        i = Continuation( InFile->bufr, i );
        if( i < 0 )
          {
          InFile->bufr[end] = '\0';
          return( 0 );
          }
        end = ( (i > 0) && (' ' == InFile->bufr[i - 1]) ) ? (i - 1) : (i);
        c = mygetc( InFile );       /* Read past eoln.                   */
        break;

      case '\0':                /* Shouldn't have EOF within param name. */
      case EOF:
        InFile->bufr[i] = '\0';
        return( -1 );

      default:
        if( isspace( c ) )     /* One ' ' per whitespace region.       */
          {
          InFile->bufr[end] = ' ';
          i = end + 1;
          c = EatWhitespace( InFile );
          }
        else                   /* All others verbatim.                 */
          {
          InFile->bufr[i++] = c;
          end = i;
          c = mygetc( InFile );
          }
      }
    }

  /* Now parse the value. */
  c = EatWhitespace( InFile );  /* Again, trim leading whitespace. */
  while( (EOF !=c) && (c > 0) )
    {

    if( i > (InFile->bufrlen - 2) )       /* Make sure there's enough room. */
      {    
        //return( -1 );

		/* if there isn't enough room for value, cut the left value */
		
		char last=0;/* last non-space-char of a line */

		InFile->bufr[i] = 0;
		/* eat tailing blanks */
		for( end = i-1; (end >= 0) && isspace((int)InFile->bufr[end]); end-- )
			;
		last = (end >= 0 ? InFile->bufr[end] : 0);
		
		/* drop the left ant continuation line */
		do{
			do{
				c = mygetc(InFile);
				if( !isspace(c) )
				{
					last = c;
				}
			}while( (EOF !=c) && (c > 0) && (c != '\n') );			
		}while( ('\n' == c) && ('\\' == last) );

		return( pfunc( InFile->bufr, &InFile->bufr[vstart] ) );		
      }

    switch( c )
      {
      case '\r':              /* Explicitly remove '\r' because the older */
        c = mygetc( InFile );   /* version called fgets_slash() which also  */
        break;                /* removes them.                            */

      case '\n':              /* Marks end of value unless there's a '\'. */
        i = Continuation( InFile->bufr, i );
        if( i < 0 )
          c = 0;
        else
          {
          for( end = i; (end >= 0) && isspace((int)InFile->bufr[end]); end-- )
            ;
          c = mygetc( InFile );
          }
        break;

      default:               /* All others verbatim.  Note that spaces do */
        InFile->bufr[i++] = c;       /* not advance <end>.  This allows trimming  */
        if( !isspace( c ) )  /* of whitespace at the end of the line.     */
          end = i;
        c = mygetc( InFile );
        break;
      }
    }
  InFile->bufr[end] = '\0';          /* End of value. */

  return( pfunc( InFile->bufr, &InFile->bufr[vstart] ) );   /* Pass name & value to pfunc().  */
} /* Parameter */

static int Parse( myFILE *InFile,
                   int (*sfunc)(const char *),
                   int (*pfunc)(const char *, const char *)
				   )
  /* ------------------------------------------------------------------------ **
   * Scan & parse the input.
   *
   *  Input:  InFile  - Input source.
   *          sfunc   - Function to be called when a section name is scanned.
   *                    See Section().
   *          pfunc   - Function to be called when a parameter is scanned.
   *                    See Parameter().
   *
   *  Output: 0 if the file was successfully scanned, else -1.
   *
   *  Notes:  The input can be viewed in terms of 'lines'.  There are four
   *          types of lines:
   *            Blank      - May contain whitespace, otherwise empty.
   *            Comment    - First non-whitespace character is a ';' or '#'.
   *                         The remainder of the line is ignored.
   *            Section    - First non-whitespace character is a '['.
   *            Parameter  - The default case.
   * 
   * ------------------------------------------------------------------------ **
   */
{
  int    c;

  c = EatWhitespace( InFile );
  while( (EOF != c) && (c > 0) )
    {
    switch( c )
      {
      case '\n':                        /* Blank line. */
        c = EatWhitespace( InFile );
        break;

      case ';':                         /* Comment line. */
      case '#':
        c = EatComment( InFile );
        break;

      case '[':                         /* Section Header. */
        if( -1 == Section( InFile, sfunc ) )
          return( -1 );
        c = EatWhitespace( InFile );
        break;

      case '\\':                        /* Bogus backslash. */
        c = EatWhitespace( InFile );
        break;

      default:                          /* Parameter line. */
        if( -1 == Parameter( InFile, pfunc, c ) )
          return( -1 );
        c = EatWhitespace( InFile );
        break;
      }
    }
  return( 0 );
} /* Parse */

/**
load a file into memory from a fd.
**/
static char *fd_load(int fd, myFILE* InFile)
{
	struct stat sbuf;
	char *p = InFile->buf;
	size_t size;

	if (fstat(fd, &sbuf) != 0) return NULL;

	size = (sbuf.st_size > InFile->buflen - 1 ) ?
		(InFile->buflen - 1):(sbuf.st_size);

	if (read(fd, p, size) != size) {		
		return NULL;
	}
	p[size] = 0;

	InFile->size = size;

	return p;
}

/**
load a file into memory
**/
static char *file_load(const char *fname, myFILE *InFile)
{
	int fd;
	char *p;

	if (!fname || !*fname) return NULL;

	fd = open(fname,O_RDONLY);
	if (-1 == fd) return NULL;
	p = fd_load(fd, InFile);

	close(fd);

	return p;
}

static int OpenConfFile( const char *FileName, myFILE *InFile)
  /* ------------------------------------------------------------------------ **
   * Open a configuration file.
   *
   *  Input:  FileName  - The pathname of the config file to be opened.
   *
   *  Output: A pointer of type (char **) to the lines of the file
   *
   * ------------------------------------------------------------------------ **
   */
{

  if( NULL == file_load(FileName, InFile) )
    {
		return -1;
    }
  InFile->p = InFile->buf;
  return 0;
} /* OpenConfFile */

int load_param(int (*sfunc)(const char *),
                   int (*pfunc)(const char *, const char *),
                   const char* confile)
  /* ------------------------------------------------------------------------ **
   * Process the named parameter file.
   *
   *  Input:  sfunc     - A pointer to a function that will be called when
   *                      a section name is discovered.
   *          pfunc     - A pointer to a function that will be called when
   *                      a parameter name and value are discovered.
   *
   *  Output: 0 if the file was successfully parsed, else -1.
   *
   * ------------------------------------------------------------------------ **
   */
{
  int   result;
  myFILE File;
  char *conf;
  char *line;

  if( (NULL == sfunc) || (NULL == pfunc) || (NULL == confile) )
  	return -1;

  conf = (char *)malloc(CONF_FILE_LEN);
  if(!conf)
  {
  	return -1;
  }
  
  line = (char *)malloc(CONF_LINE_LEN);
  if(!line)
  {
  	free(conf);
  	return -1;
  }

  /* init File , with static buffer */
  File.buf = conf;
  File.buflen = CONF_FILE_LEN;
  File.bufr = line;
  File.bufrlen = CONF_LINE_LEN;
  File.p = NULL;
  File.size = 0;
  	
  result = OpenConfFile( confile , &File);          /* Open the config file. */
  if( -1 == result ){
  	free(conf);
  	free(line);
    return( -1 );
  }

  result = Parse( &File, sfunc, pfunc );   /* (recursive call), then just */
                                              /* use it.                     */
  myfile_close(&File);

  free(conf);
  free(line);
  
  return( result );                             /* Generic success. */
} /* pm_process */
