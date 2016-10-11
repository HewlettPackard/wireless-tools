/*
 *	Wireless Tools
 *
 *		Jean II - HPLB '99
 *
 * Main code for "iwconfig". This is the generic tool for most
 * manipulations...
 * You need to link this code against "iwcommon.c" and "-lm".
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
      if(has_range && (qual[i].level != 0))
	/* If the statistics are in dBm */
	if(qual[i].level > range.max_qual.level)
	  printf("    %s : Quality %d/%d ; Signal %d dBm ; Noise %d dBm %s\n",
		 pr_ether(hwa[i].sa_data),
		 qual[i].qual, range.max_qual.qual,
		 qual[i].level - 0x100, qual[i].noise - 0x100,
		 qual[i].updated & 0x7 ? "(updated)" : "");
	else
	  printf("    %s : Quality %d/%d ; Signal %d/%d ; Noise %d/%d %s\n",
		 pr_ether(hwa[i].sa_data),
		 qual[i].qual, range.max_qual.qual,
		 qual[i].level, range.max_qual.level,
		 qual[i].noise, range.max_qual.noise,
		 qual[i].updated & 0x7 ? "(updated)" : "");
      else
	printf("    %s : Quality %d ; Signal %d ; Noise %d %s\n",
	       pr_ether(hwa[i].sa_data),
	       qual[i].qual, qual[i].level, qual[i].noise,
	       qual[i].updated & 0x7 ? "(updated)" : "");
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

/*------------------------------------------------------------------*/
/*
 * Print the number of channels and available frequency for the device
 */
static void
print_freq_info(int		skfd,
		char *		ifname)
{
  struct iwreq		wrq;
  float			freq;
  struct iw_range	range;
  int			k;

  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  wrq.u.data.pointer = (caddr_t) &range;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCGIWRANGE, &wrq) < 0)
      fprintf(stderr, "%-8.8s  no frequency information.\n\n",
		      ifname);
  else
    {
      if(range.num_frequency > 0)
	{
	  printf("%-8.8s  %d channels in total; available frequencies :\n",
		 ifname, range.num_channels);
	  /* Print them all */
	  for(k = 0; k < range.num_frequency; k++)
	    {
#if WIRELESS_EXT > 7
		printf("\t  Channel %.2d : ", range.freq[k].i);
#else
		printf("\t  ");
#endif
	      freq = freq2float(&(range.freq[k]));
	      if(freq >= GIGA)
		printf("%g GHz\n", freq / GIGA);
	      else
		if(freq >= MEGA)
		  printf("%g MHz\n", freq / MEGA);
		else
		  printf("%g kHz\n", freq / KILO);
	    }
	  printf("\n\n");
	}
      else
	printf("%-8.8s  %d channels\n\n",
	       ifname, range.num_channels);
    }
}

/*------------------------------------------------------------------*/
/*
 * Get frequency info on all devices and print it on the screen
 */
static void
print_freq_devices(int		skfd)
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
    print_freq_info(skfd, ifr->ifr_name);
}

#if WIRELESS_EXT > 5
/*------------------------------------------------------------------*/
/*
 * Display the list of ap addresses and the associated stats
 * Exacly the same as the spy list, only with different IOCTL and messages
 */
static void
print_ap_info(int	skfd,
	       char *	ifname)
{
  struct iwreq		wrq;
  char		buffer[(sizeof(struct iw_quality) +
			sizeof(struct sockaddr)) * IW_MAX_AP];
  struct sockaddr *	hwa;
  struct iw_quality *	qual;
  iwrange	range;
  int		has_range = 0;
  int		has_qual = 0;
  int		n;
  int		i;

  /* Collect stats */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  wrq.u.data.pointer = (caddr_t) buffer;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCGIWAPLIST, &wrq) < 0)
    {
      fprintf(stderr, "%-8.8s  Interface doesn't have a list of Access Points\n\n", ifname);
      return;
    }

  /* Number of addresses */
  n = wrq.u.data.length;
  has_qual = wrq.u.data.flags;

  /* The two lists */
  hwa = (struct sockaddr *) buffer;
  qual = (struct iw_quality *) (buffer + (sizeof(struct sockaddr) * n));

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
    printf("%-8.8s  No Access Point in range\n", ifname);
  else
    printf("%-8.8s  Access Points in range:\n", ifname);
  for(i = 0; i < n; i++)
    {
      if(has_qual)
	if(has_range)
	  /* If the statistics are in dBm */
	  if(qual[i].level > range.max_qual.level)
	    printf("    %s : Quality %d/%d ; Signal %d dBm ; Noise %d dBm %s\n",
		   pr_ether(hwa[i].sa_data),
		   qual[i].qual, range.max_qual.qual,
		   qual[i].level - 0x100, qual[i].noise - 0x100,
		   qual[i].updated & 0x7 ? "(updated)" : "");
	  else
	    printf("    %s : Quality %d/%d ; Signal %d/%d ; Noise %d/%d %s\n",
		   pr_ether(hwa[i].sa_data),
		   qual[i].qual, range.max_qual.qual,
		   qual[i].level, range.max_qual.level,
		   qual[i].noise, range.max_qual.noise,
		   qual[i].updated & 0x7 ? "(updated)" : "");
	else
	  printf("    %s : Quality %d ; Signal %d ; Noise %d %s\n",
		 pr_ether(hwa[i].sa_data),
		 qual[i].qual, qual[i].level, qual[i].noise,
		 qual[i].updated & 0x7 ? "(updated)" : "");
      else
	printf("    %s\n", pr_ether(hwa[i].sa_data));
    }
  printf("\n");
}

/*------------------------------------------------------------------*/
/*
 * Get list of AP on all devices and print it on the screen
 */
static void
print_ap_devices(int		skfd)
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
    print_ap_info(skfd, ifr->ifr_name);
}
#endif	/* WIRELESS_EXT > 5 */

#if WIRELESS_EXT > 7
/*------------------------------------------------------------------*/
/*
 * Print the number of available bitrates for the device
 */
static void
print_bitrate_info(int		skfd,
		   char *	ifname)
{
  struct iwreq		wrq;
  float			bitrate;
  struct iw_range	range;
  int			k;

  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  wrq.u.data.pointer = (caddr_t) &range;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCGIWRANGE, &wrq) < 0)
      fprintf(stderr, "%-8.8s  no bit-rate information.\n\n",
		      ifname);
  else
    {
      if((range.num_bitrates > 0) && (range.num_bitrates < IW_MAX_BITRATES))
	{
	  printf("%-8.8s  %d available bit-rates :\n",
		 ifname, range.num_bitrates);
	  /* Print them all */
	  for(k = 0; k < range.num_bitrates; k++)
	    {
	      printf("\t  ");
	      bitrate = range.bitrate[k];
	      if(bitrate >= GIGA)
		printf("%g Gb/s\n", bitrate / GIGA);
	      else
		if(bitrate >= MEGA)
		  printf("%g Mb/s\n", bitrate / MEGA);
		else
		  printf("%g kb/s\n", bitrate / KILO);
	    }
	  printf("\n\n");
	}
      else
	printf("%-8.8s  No bit-rates ? Please update driver...\n\n", ifname);
    }
}

/*------------------------------------------------------------------*/
/*
 * Get bit-rate info on all devices and print it on the screen
 */
static void
print_bitrate_devices(int		skfd)
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
    print_bitrate_info(skfd, ifr->ifr_name);
}
#endif	/* WIRELESS_EXT > 7 */

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

  /* Frequency list */
  if((!strncmp(argv[1], "freq", 4)) ||
     (!strncmp(argv[1], "channel", 7)))
    {
      print_freq_devices(skfd);
      close(skfd);
      exit(0);
    }

#if WIRELESS_EXT > 5
  /* Access Point list */
  if(!strcasecmp(argv[1], "ap"))
    {
      print_ap_devices(skfd);
      close(skfd);
      exit(0);
    }
#endif	/* WIRELESS_EXT > 5 */

#if WIRELESS_EXT > 7
  /* Bit-rate list */
  if((!strncmp(argv[1], "bit", 3)) ||
     (!strcmp(argv[1], "rate")))
    {
      print_bitrate_devices(skfd);
      close(skfd);
      exit(0);
    }
#endif	/* WIRELESS_EXT > 7 */

  /* The device name must be the first argument */
  /* Name only : show spy list for that device only */
  if(argc == 2)
    {
      print_spy_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

  /* Special cases take two... */
  /* Frequency list */
  if((!strncmp(argv[2], "freq", 4)) ||
     (!strncmp(argv[2], "channel", 7)))
    {
      print_freq_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

#if WIRELESS_EXT > 5
  /* Access Point  list */
  if(!strcasecmp(argv[2], "ap"))
    {
      print_ap_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }
#endif	/* WIRELESS_EXT > 5 */

#if WIRELESS_EXT > 7
  /* Access Point  list */
  if((!strncmp(argv[2], "bit", 3)) ||
     (!strcmp(argv[2], "rate")))
    {
      print_bitrate_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }
#endif	/* WIRELESS_EXT > 7 */

  /* Otherwise, it's a list of address to set in the spy list */
  goterr = set_spy_info(skfd, argv + 2, argc - 2, argv[1]);

  /* Close the socket. */
  close(skfd);

  return(1);
}
