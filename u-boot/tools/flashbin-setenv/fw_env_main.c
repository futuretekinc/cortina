/*
 * (C) Copyright 2001-2010
 * Cortina Systems Inc.
 *
 * This is a program to write the u-boot env partition into a flash.bin
 * file.  It is based on a hacked up version of fw_printenv, from
 * uboot-envtools.  The original copyright notices have been left
 * intact, and remain true.
*/
/*
 * (C) Copyright 2000-2008
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "fw_env.h"

static char *Usage = "-binfile flash.bin -envoffset <offset> -envsize <envsize> [-printenv | -setenv [file | var value]]";

static int sete( int, char** );

int
main(int argc, char *argv[])
{
  char *binfile = NULL;
  unsigned int eoff = -1;
  unsigned int esz  = -1;
  int print = 1;
  int c = 0;
  char *v[10];

  v[0] = argv[0];
  c = 1;

  int i=1;
  while( i < argc ) {
    if ( strcmp( argv[i], "-help" ) == 0 ) {
      printf( "%s\n", Usage );
      return( 0 );
    }

    if ( strcmp( argv[i], "-binfile" ) == 0 ) {
      i += 1;
      binfile = argv[i];
    }
    else if ( strcmp( argv[i], "-envoffset" ) == 0 ) {
      i += 1;
      eoff = strtol( argv[i], NULL, 0 );
    }
    else if ( strcmp( argv[i], "-envsize" ) == 0 ) {
      i += 1;
      esz = strtol( argv[i], NULL, 0 );
    }
    else if ( strcmp( argv[i], "-printenv" ) == 0 ) {
      print = 1;
    }
    else if ( strcmp( argv[i], "-setenv" ) == 0 ) {
      print = 0;
    }
    else {
      v[c++] = argv[i];
    }

    i = i + 1;
  }

  if ( binfile == NULL ||
       eoff    == -1   ||
       esz     == -1   ||
       ( print == 0 && c == 1 ) ) {
    printf( "%s\n", Usage );
    return( 1 );
  }

  fw_setconfig( binfile, eoff, esz, 0, 0 );

  if ( print ) {
    int st = fw_printenv( c, v );
    return( st );
  }

  return( sete( c, v ) );
}

int sete( int argc, char **argv )
{
  struct stat st;

  if (stat (argv[1], &st) == -1 ) {
    // Setting on the command line.
    return( fw_setenv( argc, argv ) );
  }
  else {
    // Setting from reading from a file
    char *fname = argv[1];
    int c = 1;
    char *v[3];
    FILE *fp;
    int rc;
    char dump[512];
    char *VAR;
    char *VAL;
    char *SE;

    fp = fopen (fname, "r");
    if (fp == NULL) {
      fprintf (stderr, "Cannot read setenv file %s: %s\n",
	       fname, strerror(errno) );
      return( 1 );
    }

    while (fgets (dump, sizeof (dump), fp)) {
      /* Skip incomplete conversions and comment strings */
      if (dump[0] == '#')
	continue;
      if (dump[0] == '\n')
	continue;

      c = 0;

      SE = strtok( dump, " " );
      v[c++] = SE;

      VAR = strtok( NULL, " \n" );
      v[c++] = VAR;

      VAL = strtok( NULL, "\n" );
      if ( VAL )
	v[c++] = VAL;
      rc = fw_setenv( c, v );
      if ( rc ) 
	return( 1 );
    }
    fclose(fp);
    return( 0 );
  }
}
