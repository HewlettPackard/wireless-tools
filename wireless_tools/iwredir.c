/*
 *	Wireless Tools
 *
 *		Jean II - HPL 03
 *
 * Main code for "iwredir". This is a hack to match multiple version
 * of the tools with multiple kernels on the same system...
 *
 * This file is released under the GPL license.
 *     Copyright (c) 2003 Jean Tourrilhes <jt@hpl.hp.com>
 */

/***************************** INCLUDES *****************************/

#include "iwlib.h"		/* Header */

/*********************** VERSION SUBROUTINES ***********************/

/*------------------------------------------------------------------*/
/*
 * Extract WE version number from /proc/net/wireless
 * If we have WE-16 and later, the WE version is available at the
 * end of the header line of the file.
 * For version prior to that, we can only detect the change from
 * v11 to v12, so we do an approximate job. Fortunately, v12 to v15
 * are highly binary compatible (on the struct level).
 */
static int
iw_get_kernel_we_version(void)
{
  char		buff[1024];
  FILE *	fh;
  char *	p;
  int		v;

  /* Check if /proc/net/wireless is available */
  fh = fopen(PROC_NET_WIRELESS, "r");

  if(fh == NULL)
    {
      fprintf(stderr, "Cannot read " PROC_NET_WIRELESS "\n");
      return(-1);
    }

  /* Read the first line of buffer */
  fgets(buff, sizeof(buff), fh);

  if(strstr(buff, "| WE") == NULL)
    {
      /* Prior to WE16, so explicit version not present */

      /* Black magic */
      if(strstr(buff, "| Missed") == NULL)
	v = 11;
      else
	v = 15;
      fclose(fh);
      return(v);
    }

  /* Read the second line of buffer */
  fgets(buff, sizeof(buff), fh);

  /* Get to the last separator, to get the version */
  p = strrchr(buff, '|');
  if((p == NULL) || (sscanf(p + 1, "%d", &v) != 1))
    {
      fprintf(stderr, "Cannot parse " PROC_NET_WIRELESS "\n");
      fclose(fh);
      return(-1);
    }

  fclose(fh);
  return(v);
}

/******************************* MAIN ********************************/

/*------------------------------------------------------------------*/
/*
 * The main !
 */
int
main(int	argc,
     char **	argv)
{
  int	version = 0;
  int	goterr = 0;
  char	file[512];
  int	flen;

  /* Get current version */
  version = iw_get_kernel_we_version();
  if(version <= 0)
    version = 15;	/* We can only read only WE16 and higher */

  /* Special case for Wireless Extension Version... */
  /* This is mostly used in the Makefile, we use an "unlikely" option
   * to maximise transparency to the tool we masquerade - Jean II */
  if((argc > 1) && !strcmp(argv[1], "-wev"))
    {
      printf("%d\n", version);
      return(version);
    }

  /* Mangle the command name */
  flen = strlen(argv[0]);
  if((flen + 3) >= 512)
    {
      fprintf(stderr, "Command name too long [%s]\n", argv[0]);
      return(-1);
    }
  memcpy(file, argv[0], flen + 1);
  sprintf(file + flen, "%d", version);

  /* Execute (won't return) */
  goterr = execvp(file, argv);

  /* In case of error */
  fprintf(stderr, "Can't execute command %s: %s\n", file, strerror(errno));
  return(goterr);
}


