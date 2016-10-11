/*
 *	Wireless Tools
 *
 *		Jean II - HPLB '99
 *
 * This tool can manipulate the spy list : add addresses and display stat
 * You need to link this code against "iwcommon.c" and "-lm".
 *
 * This file is released under the GPL license.
 */

#include "iwcommon.h"		/* Header */

/************************* DISPLAY ROUTINES **************************/

/*------------------------------------------------------------------*/
/*
 * Display the spy list of addresses and the associated stats
 */
static void
print_spy_info(int	skfd,
	       char *	ifname)
{
  struct iwreq		wrq;
  char		buffer[(sizeof(struct iw_quality) +
			sizeof(struct sockaddr)) * IW_MAX_SPY];
  struct sockaddr 	hwa[IW_MAX_SPY];
  struct iw_quality 	qual[IW_MAX_SPY];
  iwrange	range;
  int		has_range = 0;
  int		n;
  int		i;

  /* Collect stats */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  wrq.u.data.pointer = (caddr_t) buffer;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCGIWSPY, &wrq) < 0)
    {
      fprintf(stderr, "%-8.8s  Interface doesn't support wireless statistic collection\n\n", ifname);
      return;
    }

  /* Number of addresses */
  n = wrq.u.data.length;



  /* Check if we have valid address types */
  if(check_addr_type(skfd, ifname) < 0)
    {
      fprintf(stderr, "%-8.8s  Interface doesn't support MAC & IP addresses\n\n", ifname);
      return;
    }

  /* Get range info if we can */
  if(get_range_info(skfd, ifname, &(range)) >= 0)
    has_range = 1;

  /* Display it */
  if(n == 0)
    printf("%-8.8s  No statistics to collect\n", ifname);
  else
    printf("%-8.8s  Statistics collected:\n", ifname);
 
  /* The two lists */

  memcpy(hwa, buffer, n * sizeof(struct sockaddr));
  memcpy(qual, buffer + n*sizeof(struct sockaddr), n*sizeof(struct iw_quality));

  for(i = 0; i < n; i++)
    {
      /* Print stats for each address */
      printf("    %s : ", pr_ether(hwa[i].sa_data));
      print_stats(stdout, &qual[i], &range, has_range);
    }
  printf("\n");
}

/*------------------------------------------------------------------*/
/*
 * Get info on all devices and print it on the screen
 */
static void
print_spy_devices(int		skfd)
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
    print_spy_info(skfd, ifr->ifr_name);
}

/************************* SETTING ROUTINES **************************/

/*------------------------------------------------------------------*/
/*
 * Set list of addresses specified on command line in the driver.
 */
static int
set_spy_info(int		skfd,		/* The socket */
	     char *		args[],		/* Command line args */
	     int		count,		/* Args count */
	     char *		ifname)		/* Dev name */
{
  struct iwreq		wrq;
  int			i;
  int			nbr;		/* Number of valid addresses */
  struct sockaddr	hw_address[IW_MAX_SPY];

  /* Read command line */
  i = 0;	/* first arg to read */
  nbr = 0;	/* Number of args readen so far */

  /* Check if we have valid address types */
  if(check_addr_type(skfd, ifname) < 0)
    {
      fprintf(stderr, "%-8.8s  Interface doesn't support MAC & IP addresses\n", ifname);
      return(-1);
    }

  /* "off" : disable functionality (set 0 addresses) */
  if(!strcmp(args[0], "off"))
    i = count;	/* hack */

  /* "+" : add all addresses already in the driver */
  if(!strcmp(args[0], "+"))
    {
      char	buffer[(sizeof(struct iw_quality) +
			sizeof(struct sockaddr)) * IW_MAX_SPY];

      strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
      wrq.u.data.pointer = (caddr_t) buffer;
      wrq.u.data.length = 0;
      wrq.u.data.flags = 0;
      if(ioctl(skfd, SIOCGIWSPY, &wrq) < 0)
	{
	  fprintf(stderr, "Interface doesn't accept reading addresses...\n");
	  fprintf(stderr, "SIOCGIWSPY: %s\n", strerror(errno));
	  return(-1);
	}

      /* Copy old addresses */
      nbr = wrq.u.data.length;
      memcpy(hw_address, buffer, nbr * sizeof(struct sockaddr));

      i = 1;	/* skip the "+" */
    }

  /* Read other args on command line */
  while((i < count) && (nbr < IW_MAX_SPY))
    {
      if(in_addr(skfd, ifname, args[i++], &(hw_address[nbr])) < 0)
	continue;
      nbr++;
    }

  /* Check the number of addresses */
  if((nbr == 0) && strcmp(args[0], "off"))
    {
      fprintf(stderr, "No valid addresses found : exiting...\n");
      exit(0);
    }

  /* Check if there is some remaining arguments */
  if(i < count)
    {
      fprintf(stderr, "Got only the first %d addresses, remaining discarded\n", IW_MAX_SPY);
    }

  /* Time to do send addresses to the driver */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  wrq.u.data.pointer = (caddr_t) hw_address;
  wrq.u.data.length = nbr;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCSIWSPY, &wrq) < 0)
    {
      fprintf(stderr, "Interface doesn't accept addresses...\n");
      fprintf(stderr, "SIOCSIWSPY: %s\n", strerror(errno));
      return(-1);
    }

  return(0);
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
      print_spy_devices(skfd);
      close(skfd);
      exit(0);
    }

  /* Special cases take one... */
  /* Help */
  if((!strncmp(argv[1], "-h", 9)) ||
     (!strcmp(argv[1], "--help")))
    {
      fprintf(stderr, "Usage: iwspy interface [+] [MAC address] [IP address]\n");
      fprintf(stderr, "             interface [freq]\n");
      fprintf(stderr, "             interface [ap]\n");
      close(skfd);
      exit(0);
    }

  /* The device name must be the first argument */
  /* Name only : show spy list for that device only */
  if(argc == 2)
    {
      print_spy_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

  /* Otherwise, it's a list of address to set in the spy list */
  goterr = set_spy_info(skfd, argv + 2, argc - 2, argv[1]);

  /* Close the socket. */
  close(skfd);

  return(1);
}
