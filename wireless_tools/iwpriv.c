/*
 *	Wireless Tools
 *
 *		Jean II - HPLB 97->99 - HPL 99->00
 *
 * Main code for "iwconfig". This is the generic tool for most
 * manipulations...
 * You need to link this code against "iwcommon.c" and "-lm".
 *
 * This file is released under the GPL license.
 */

#include "iwcommon.h"		/* Header */

/************************* MISC SUBROUTINES **************************/

/*------------------------------------------------------------------*/
/*
 * Print usage string
 */
static void
iw_usage(void)
{
  fprintf(stderr, "Usage: iwpriv interface [private-command [private-arguments]]\n");
  fprintf(stderr, "              interface [roam {on|off}]\n");
  fprintf(stderr, "              interface [port {ad-hoc|managed|N}]\n");
  exit(1);
}

/************************ GENERIC FUNCTIONS *************************/

/*------------------------------------------------------------------*/
/*
 * Print on the screen in a neat fashion all the info we have collected
 * on a device.
 */
static void
print_priv_info(int		skfd,
		char *		ifname)
{
  int		k;
  iwprivargs	priv[16];
  int		n;
  char *	argtype[] = { "    ", "byte", "char", "", "int", "float" };

  /* Read the private ioctls */
  n = get_priv_info(skfd, ifname, priv);

  /* Is there any ? */
  if(n <= 0)
    {
      /* Could skip this message ? */
      fprintf(stderr, "%-8.8s  no private ioctls.\n\n",
	      ifname);
    }
  else
    {
      printf("%-8.8s  Available private ioctl :\n", ifname);
      /* Print the all */
      for(k = 0; k < n; k++)
	printf("          %s (%X) : set %3d %s & get %3d %s\n",
	       priv[k].name, priv[k].cmd,
	       priv[k].set_args & IW_PRIV_SIZE_MASK,
	       argtype[(priv[k].set_args & IW_PRIV_TYPE_MASK) >> 12],
	       priv[k].get_args & IW_PRIV_SIZE_MASK,
	       argtype[(priv[k].get_args & IW_PRIV_TYPE_MASK) >> 12]);
      printf("\n");
    }
}

/*------------------------------------------------------------------*/
/*
 * Get info on all devices and print it on the screen
 */
static void
print_priv_devices(int		skfd)
{
  char		buff[1024];
  struct ifconf ifc;
  struct ifreq *ifr;
  int i;

  /* Get list of active devices */
  ifc.ifc_len = sizeof(buff);
  ifc.ifc_buf = buff;
  if(ioctl(skfd, SIOCGIFCONF, &ifc) < 0)
    {
      fprintf(stderr, "SIOCGIFCONF: %s\n", strerror(errno));
      return;
    }
  ifr = ifc.ifc_req;

  /* Print them */
  for(i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; ifr++)
    print_priv_info(skfd, ifr->ifr_name);
}

/************************* SETTING ROUTINES **************************/

/*------------------------------------------------------------------*/
/*
 * Execute a private command on the interface
 */
static int
set_private(int		skfd,		/* Socket */
	    char *	args[],		/* Command line args */
	    int		count,		/* Args count */
	    char *	ifname)		/* Dev name */
{
  u_char	buffer[1024];
  struct iwreq		wrq;
  int		i = 0;		/* Start with first arg */
  int		k;
  iwprivargs	priv[16];
  int		number;

  /* Read the private ioctls */
  number = get_priv_info(skfd, ifname, priv);

  /* Is there any ? */
  if(number <= 0)
    {
      /* Could skip this message ? */
      fprintf(stderr, "%-8.8s  no private ioctls.\n\n",
	      ifname);
      return(-1);
    }

  /* Search the correct ioctl */
  k = -1;
  while((++k < number) && strcmp(priv[k].name, args[i]));

  /* If not found... */
  if(k == number)
    {
      fprintf(stderr, "Invalid command : %s\n", args[i]);
      return(-1);
    }
	  
  /* Next arg */
  i++;

  /* If we have to set some data */
  if((priv[k].set_args & IW_PRIV_TYPE_MASK) &&
     (priv[k].set_args & IW_PRIV_SIZE_MASK))
    {
      switch(priv[k].set_args & IW_PRIV_TYPE_MASK)
	{
	case IW_PRIV_TYPE_BYTE:
	  /* Number of args to fetch */
	  wrq.u.data.length = count - 1;
	  if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
	    wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

	  /* Fetch args */
	  for(; i < wrq.u.data.length + 1; i++)
	    sscanf(args[i], "%d", (int *)(buffer + i - 1));
	  break;

	case IW_PRIV_TYPE_INT:
	  /* Number of args to fetch */
	  wrq.u.data.length = count - 1;
	  if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
	    wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

	  /* Fetch args */
	  for(; i < wrq.u.data.length + 1; i++)
	    sscanf(args[i], "%d", ((u_int *) buffer) + i - 1);
	  break;

	case IW_PRIV_TYPE_CHAR:
	  if(i < count)
	    {
	      /* Size of the string to fetch */
	      wrq.u.data.length = strlen(args[i]) + 1;
	      if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
		wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

	      /* Fetch string */
	      memcpy(buffer, args[i], wrq.u.data.length);
	      buffer[sizeof(buffer) - 1] = '\0';
	      i++;
	    }
	  else
	    {
	      wrq.u.data.length = 1;
	      buffer[0] = '\0';
	    }
	  break;

	default:
	  fprintf(stderr, "Not yet implemented...\n");
	  return(-1);
	}
	  
      if((priv[k].set_args & IW_PRIV_SIZE_FIXED) &&
	 (wrq.u.data.length != (priv[k].set_args & IW_PRIV_SIZE_MASK)))
	{
	  printf("The command %s need exactly %d argument...\n",
		 priv[k].name, priv[k].set_args & IW_PRIV_SIZE_MASK);
	  return(-1);
	}
    }	/* if args to set */
  else
    {
      wrq.u.data.length = 0L;
    }

  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

  if((priv[k].set_args & IW_PRIV_SIZE_FIXED) &&
     (byte_size(priv[k].set_args) < IFNAMSIZ))
    memcpy(wrq.u.name, buffer, IFNAMSIZ);
  else
    {
      wrq.u.data.pointer = (caddr_t) buffer;
      wrq.u.data.flags = 0;
    }

  /* Perform the private ioctl */
  if(ioctl(skfd, priv[k].cmd, &wrq) < 0)
    {
      fprintf(stderr, "Interface doesn't accept private ioctl...\n");
      fprintf(stderr, "%X: %s\n", priv[k].cmd, strerror(errno));
      return(-1);
    }

  /* If we have to get some data */
  if((priv[k].get_args & IW_PRIV_TYPE_MASK) &&
     (priv[k].get_args & IW_PRIV_SIZE_MASK))
    {
      int	j;
      int	n = 0;		/* number of args */

      printf("%-8.8s  %s:", ifname, priv[k].name);

      if((priv[k].get_args & IW_PRIV_SIZE_FIXED) &&
	 (byte_size(priv[k].get_args) < IFNAMSIZ))
	{
	  memcpy(buffer, wrq.u.name, IFNAMSIZ);
	  n = priv[k].get_args & IW_PRIV_SIZE_MASK;
	}
      else
	n = wrq.u.data.length;

      switch(priv[k].get_args & IW_PRIV_TYPE_MASK)
	{
	case IW_PRIV_TYPE_BYTE:
	  /* Display args */
	  for(j = 0; j < n; j++)
	    printf("%d  ", buffer[j]);
	  printf("\n");
	  break;

	case IW_PRIV_TYPE_INT:
	  /* Display args */
	  for(j = 0; j < n; j++)
	    printf("%d  ", ((u_int *) buffer)[j]);
	  printf("\n");
	  break;

	case IW_PRIV_TYPE_CHAR:
	  /* Display args */
	  buffer[wrq.u.data.length - 1] = '\0';
	  printf("%s\n", buffer);
	  break;

	default:
	  fprintf(stderr, "Not yet implemented...\n");
	  return(-1);
	}
    }	/* if args to set */

  return(0);
}

/********************** PRIVATE IOCTLS MANIPS ***********************/
/*
 * Convenient access to some private ioctls of some devices
 */

/*------------------------------------------------------------------*/
/*
 * Set roaming mode on and off
 * Found in wavelan_cs driver
 */
static int
set_roaming(int		skfd,		/* Socket */
	    char *	args[],		/* Command line args */
	    int		count,		/* Args count */
	    char *	ifname)		/* Dev name */
{
  u_char	buffer[1024];
  struct iwreq		wrq;
  int		i = 0;		/* Start with first arg */
  int		k;
  iwprivargs	priv[16];
  int		number;
  char		RoamState;		/* buffer to hold new roam state */
  char		ChangeRoamState=0;	/* whether or not we are going to
					   change roam states */

  /* Read the private ioctls */
  number = get_priv_info(skfd, ifname, priv);

  /* Is there any ? */
  if(number <= 0)
    {
      /* Could skip this message ? */
      fprintf(stderr, "%-8.8s  no private ioctls.\n\n",
	      ifname);
      return(-1);
    }

  if(count != 1)
    iw_usage();

  if(!strcasecmp(args[i], "on"))
    {
      printf("%-8.8s  enable roaming\n", ifname);
      if(!number)
	{
	  fprintf(stderr, "This device doesn't support roaming\n");
	  return(-1);
	}
      ChangeRoamState=1;
      RoamState=1;
    }
  else
    if(!strcasecmp(args[i], "off"))
      {
	i++;
	printf("%-8.8s  disable roaming\n",  ifname);
	if(!number)
	  {
	    fprintf(stderr, "This device doesn't support roaming\n");
	    return(-1);
	  }
	ChangeRoamState=1;
	RoamState=0;
      }
    else
      {
	iw_usage();
	return(-1);
      }

  if(ChangeRoamState)
    {
      k = -1;
      while((++k < number) && strcmp(priv[k].name, "setroam"));
      if(k == number)
	{
	  fprintf(stderr, "This device doesn't support roaming\n");
	  return(-1);
	}
      strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

      buffer[0]=RoamState;

      memcpy(wrq.u.name, &buffer, IFNAMSIZ);

      if(ioctl(skfd, priv[k].cmd, &wrq) < 0)
	{
	  fprintf(stderr, "Roaming support is broken.\n");
	  exit(0);
	}
    }
  i++;

  return(i);
}

/*------------------------------------------------------------------*/
/*
 * Get and set the port type
 * Found in wavelan2_cs and wvlan_cs drivers
 */
static int
port_type(int		skfd,		/* Socket */
	  char *	args[],		/* Command line args */
	  int		count,		/* Args count */
	  char *	ifname)		/* Dev name */
{
  struct iwreq	wrq;
  int		i = 0;		/* Start with first arg */
  int		k;
  iwprivargs	priv[16];
  int		number;
  char		ptype = 0;
  char *	modes[] = { "invalid", "managed (BSS)", "reserved", "ad-hoc" };

  /* Read the private ioctls */
  number = get_priv_info(skfd, ifname, priv);

  /* Is there any ? */
  if(number <= 0)
    {
      /* Could skip this message ? */
      fprintf(stderr, "%-8.8s  no private ioctls.\n\n", ifname);
      return(-1);
    }

  /* Arguments ? */
  if(count == 0)
    {
      /* So, we just want to see the current value... */
      k = -1;
      while((++k < number) && strcmp(priv[k].name, "gport_type") &&
	     strcmp(priv[k].name, "get_port"));
      if(k == number)
	{
	  fprintf(stderr, "This device doesn't support getting port type\n");
	  return(-1);
	}
      strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

      /* Get it */
      if(ioctl(skfd, priv[k].cmd, &wrq) < 0)
	{
	  fprintf(stderr, "Port type support is broken.\n");
	  exit(0);
	}
      ptype = *wrq.u.name;

      /* Display it */
      printf("%-8.8s  Current port mode is %s <port type is %d>.\n\n",
	     ifname, modes[(int) ptype], ptype);

      return(0);
    }

  if(count != 1)
    iw_usage();

  /* Read it */
  /* As a string... */
  k = 0;
  while((k < 4) && strncasecmp(args[i], modes[k], 2))
    k++;
  if(k < 4)
    ptype = k;
  else
    /* ...or as an integer */
    if(sscanf(args[i], "%d", (int *) &ptype) != 1)
      iw_usage();
  
  k = -1;
  while((++k < number) && strcmp(priv[k].name, "sport_type") &&
	strcmp(priv[k].name, "set_port"));
  if(k == number)
    {
      fprintf(stderr, "This device doesn't support setting port type\n");
      return(-1);
    }
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

  *(wrq.u.name) = ptype;

  if(ioctl(skfd, priv[k].cmd, &wrq) < 0)
    {
      fprintf(stderr, "Invalid port type (or setting not allowed)\n");
      exit(0);
    }

  i++;
  return(i);
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
  int skfd = -1;		/* generic raw socket desc.	*/
  int goterr = 0;

  /* Create a channel to the NET kernel. */
  if((skfd = sockets_open()) < 0)
    {
      perror("socket");
      exit(-1);
    }

  /* No argument : show the list of all device + info */
  if(argc == 1)
    {
      print_priv_devices(skfd);
      close(skfd);
      exit(0);
    }

  /* Special cases take one... */
  /* Help */
  if((!strncmp(argv[1], "-h", 9)) ||
     (!strcmp(argv[1], "--help")))
    {
      iw_usage();
      close(skfd);
      exit(0);
    }

  /* The device name must be the first argument */
  /* Name only : show for that device only */
  if(argc == 2)
    {
      print_priv_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

  /* Special cases take two... */
  /* Roaming */
  if(!strncmp(argv[2], "roam", 4))
    {
      goterr = set_roaming(skfd, argv + 3, argc - 3, argv[1]);
      close(skfd);
      exit(0);
    }

  /* Port type */
  if(!strncmp(argv[2], "port", 4))
    {
      goterr = port_type(skfd, argv + 3, argc - 3, argv[1]);
      close(skfd);
      exit(0);
    }

  /* Otherwise, it's a private ioctl */
  goterr = set_private(skfd, argv + 2, argc - 2, argv[1]);

  /* Close the socket. */
  close(skfd);

  return(1);
}
