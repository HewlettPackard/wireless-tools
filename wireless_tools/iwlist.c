/*
 *	Wireless Tools
 *
 *		Jean II - HPLB '99 - HPL 99->01
 *
 * This tool can access various piece of information on the card
 * not part of iwconfig...
 * You need to link this code against "iwlist.c" and "-lm".
 *
 * This file is released under the GPL license.
 *     Copyright (c) 1997-2002 Jean Tourrilhes <jt@hpl.hp.com>
 */

#include "iwlib.h"		/* Header */

/*********************** FREQUENCIES/CHANNELS ***********************/

/*------------------------------------------------------------------*/
/*
 * Print the number of channels and available frequency for the device
 */
static void
print_freq_info(int		skfd,
		char *		ifname)
{
  float			freq;
  struct iw_range	range;
  int			k;

  if(iw_get_range_info(skfd, ifname, &range) < 0)
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
	      printf("\t  Channel %.2d : ", range.freq[k].i);
	      freq = iw_freq2float(&(range.freq[k]));
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

/************************ ACCESS POINT LIST ************************/

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
  char		temp[128];
  struct sockaddr *	hwa;
  struct iw_quality *	qual;
  iwrange	range;
  int		has_range = 0;
  int		has_qual = 0;
  int		n;
  int		i;

  /* Collect stats */
  wrq.u.data.pointer = (caddr_t) buffer;
  wrq.u.data.length = IW_MAX_AP;
  wrq.u.data.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWAPLIST, &wrq) < 0)
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

  /* Check if we have valid mac address type */
  if(iw_check_mac_addr_type(skfd, ifname) < 0)
    {
      fprintf(stderr, "%-8.8s  Interface doesn't support MAC addresses\n\n", ifname);
      return;
    }

  /* Get range info if we can */
  if(iw_get_range_info(skfd, ifname, &(range)) >= 0)
    has_range = 1;

  /* Display it */
  if(n == 0)
    printf("%-8.8s  No Access Point in range\n", ifname);
  else
    printf("%-8.8s  Access Points in range:\n", ifname);
  for(i = 0; i < n; i++)
    {
      if(has_qual)
	{
	  /* Print stats for this address */
	  printf("    %s : ", iw_pr_ether(temp, hwa[i].sa_data));
	  iw_print_stats(temp, &qual[i], &range, has_range);
	  printf("%s\n", temp);
	}
      else
	/* Only print the address */
	printf("    %s\n", iw_pr_ether(temp, hwa[i].sa_data));
    }
  printf("\n");
}

/***************************** BITRATES *****************************/

/*------------------------------------------------------------------*/
/*
 * Print the number of available bitrates for the device
 */
static void
print_bitrate_info(int		skfd,
		   char *	ifname)
{
  float			bitrate;
  struct iw_range	range;
  int			k;

  /* Extract range info */
  if(iw_get_range_info(skfd, ifname, &range) < 0)
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

/************************* ENCRYPTION KEYS *************************/

/*------------------------------------------------------------------*/
/*
 * Print the number of available encryption key for the device
 */
static void
print_keys_info(int		skfd,
		char *		ifname)
{
  struct iwreq		wrq;
  struct iw_range	range;
  unsigned char		key[IW_ENCODING_TOKEN_MAX];
  int			k;
  char			buffer[128];

  /* Extract range info */
  if(iw_get_range_info(skfd, ifname, &range) < 0)
      fprintf(stderr, "%-8.8s  no encryption keys information.\n\n",
		      ifname);
  else
    {
      printf("%-8.8s  ", ifname);
      /* Print key sizes */
      if((range.num_encoding_sizes > 0) &&
	 (range.num_encoding_sizes < IW_MAX_ENCODING_SIZES))
	{
	  printf("%d key sizes : %d", range.num_encoding_sizes,
		 range.encoding_size[0] * 8);
	  /* Print them all */
	  for(k = 1; k < range.num_encoding_sizes; k++)
	    printf(", %d", range.encoding_size[k] * 8);
	  printf("bits\n          ");
	}
      /* Print the keys and associate mode */
      printf("%d keys available :\n", range.max_encoding_tokens);
      for(k = 1; k <= range.max_encoding_tokens; k++)
	{
	  wrq.u.data.pointer = (caddr_t) key;
	  wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
	  wrq.u.data.flags = k;
	  if(iw_get_ext(skfd, ifname, SIOCGIWENCODE, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCGIWENCODE: %s\n", strerror(errno));
	      break;
	    }
	  if((wrq.u.data.flags & IW_ENCODE_DISABLED) ||
	     (wrq.u.data.length == 0))
	    printf("\t\t[%d]: off\n", k);
	  else
	    {
	      /* Display the key */
	      iw_print_key(buffer, key, wrq.u.data.length, wrq.u.data.flags);
	      printf("\t\t[%d]: %s", k, buffer);

	      /* Other info... */
	      printf(" (%d bits)", wrq.u.data.length * 8);
	      printf("\n");
	    }
	}
      /* Print current key and mode */
      wrq.u.data.pointer = (caddr_t) key;
      wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
      wrq.u.data.flags = 0;	/* Set index to zero to get current */
      if(iw_get_ext(skfd, ifname, SIOCGIWENCODE, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCGIWENCODE: %s\n", strerror(errno));
	  return;
	}
      printf("          Current Transmit Key: [%d]\n",
	     wrq.u.data.flags & IW_ENCODE_INDEX);
      if(wrq.u.data.flags & IW_ENCODE_RESTRICTED)
	printf("          Encryption mode:restricted\n");
      if(wrq.u.data.flags & IW_ENCODE_OPEN)
	printf("          Encryption mode:open\n");

      printf("\n\n");
    }
}

/************************* POWER MANAGEMENT *************************/

/*------------------------------------------------------------------*/
/*
 * Print Power Management info for each device
 */
static inline int
get_pm_value(int		skfd,
	     char *		ifname,
	     struct iwreq *	pwrq,
	     int		flags,
	     char *		buffer)
{
  /* Get Another Power Management value */
  pwrq->u.power.flags = flags;
  if(iw_get_ext(skfd, ifname, SIOCGIWPOWER, pwrq) >= 0)
    {
      /* Let's check the value and its type */
      if(pwrq->u.power.flags & IW_POWER_TYPE)
	{
	  iw_print_pm_value(buffer, pwrq->u.power.value, pwrq->u.power.flags);
	  printf("\n                 %s", buffer);
	}
    }
  return(pwrq->u.power.flags);
}

/*------------------------------------------------------------------*/
/*
 * Print Power Management info for each device
 */
static void
print_pm_info(int		skfd,
	      char *		ifname)
{
  struct iwreq		wrq;
  struct iw_range	range;
  char			buffer[128];

  /* Extract range info */
  if(iw_get_range_info(skfd, ifname, &range) < 0)
      fprintf(stderr, "%-8.8s  no power management information.\n\n",
		      ifname);
  else
    {
      printf("%-8.8s  ", ifname);
#if WIRELESS_EXT > 9
      /* Display modes availables */
      if(range.pm_capa & IW_POWER_MODE)
	{
	  printf("Supported modes :\n          ");
	  if(range.pm_capa & (IW_POWER_UNICAST_R | IW_POWER_MULTICAST_R))
	    printf("\t\to Receive all packets (unicast & multicast)\n          ");
	  if(range.pm_capa & IW_POWER_UNICAST_R)
	    printf("\t\to Receive Unicast only (discard multicast)\n          ");
	  if(range.pm_capa & IW_POWER_MULTICAST_R)
	    printf("\t\to Receive Multicast only (discard unicast)\n          ");
	  if(range.pm_capa & IW_POWER_FORCE_S)
	    printf("\t\to Force sending using Power Management\n          ");
	  if(range.pm_capa & IW_POWER_REPEATER)
	    printf("\t\to Repeat multicast\n          ");
	}
      /* Display min/max period availables */
      if(range.pmp_flags & IW_POWER_PERIOD)
	{
	  int	flags = (range.pmp_flags & ~(IW_POWER_MIN | IW_POWER_MAX));
	  /* Display if auto or fixed */
	  if(range.pmp_flags & IW_POWER_MIN)
	    printf("Auto  period  ; ");
	  else
	    printf("Fixed period  ; ");
	  /* Print the range */
	  iw_print_pm_value(buffer, range.min_pmp, flags | IW_POWER_MIN);
	  printf("%s\n                          ", buffer);
	  iw_print_pm_value(buffer, range.max_pmp, flags | IW_POWER_MAX);
	  printf("%s\n          ", buffer);
	  
	}
      /* Display min/max timeout availables */
      if(range.pmt_flags & IW_POWER_TIMEOUT)
	{
	  int	flags = (range.pmt_flags & ~(IW_POWER_MIN | IW_POWER_MAX));
	  /* Display if auto or fixed */
	  if(range.pmt_flags & IW_POWER_MIN)
	    printf("Auto  timeout ; ");
	  else
	    printf("Fixed timeout ; ");
	  /* Print the range */
	  iw_print_pm_value(buffer, range.min_pmt, flags | IW_POWER_MIN);
	  printf("%s\n                          ", buffer);
	  iw_print_pm_value(buffer, range.max_pmt, flags | IW_POWER_MAX);
	  printf("%s\n          ", buffer);
	  
	}
#endif /* WIRELESS_EXT > 9 */

      /* Get current Power Management settings */
      wrq.u.power.flags = 0;
      if(iw_get_ext(skfd, ifname, SIOCGIWPOWER, &wrq) >= 0)
	{
	  int	flags = wrq.u.power.flags;

	  /* Is it disabled ? */
	  if(wrq.u.power.disabled)
	    printf("Current mode:off\n          ");
	  else
	    {
	      int	pm_mask = 0;

	      /* Let's check the mode */
	      iw_print_pm_mode(buffer, flags);
	      printf("Current%s", buffer);

	      /* Let's check if nothing (simply on) */
	      if((flags & IW_POWER_MODE) == IW_POWER_ON)
		printf(" mode:on");
	      printf("\n                 ");

	      /* Let's check the value and its type */
	      if(wrq.u.power.flags & IW_POWER_TYPE)
		{
		  iw_print_pm_value(buffer,
				    wrq.u.power.value, wrq.u.power.flags);
		  printf("%s", buffer);
		}

	      /* If we have been returned a MIN value, ask for the MAX */
	      if(flags & IW_POWER_MIN)
		pm_mask = IW_POWER_MAX;
	      /* If we have been returned a MAX value, ask for the MIN */
	      if(flags & IW_POWER_MAX)
		pm_mask = IW_POWER_MIN;
	      /* If we have something to ask for... */
	      if(pm_mask)
		get_pm_value(skfd, ifname, &wrq, pm_mask, buffer);

#if WIRELESS_EXT > 9
	      /* And if we have both a period and a timeout, ask the other */
	      pm_mask = (range.pm_capa & (~(wrq.u.power.flags) &
					  IW_POWER_TYPE));
	      if(pm_mask)
		{
		  int	base_mask = pm_mask;
		  flags = get_pm_value(skfd, ifname, &wrq, pm_mask, buffer);
		  pm_mask = 0;

		  /* If we have been returned a MIN value, ask for the MAX */
		  if(flags & IW_POWER_MIN)
		    pm_mask = IW_POWER_MAX | base_mask;
		  /* If we have been returned a MAX value, ask for the MIN */
		  if(flags & IW_POWER_MAX)
		    pm_mask = IW_POWER_MIN | base_mask;
		  /* If we have something to ask for... */
		  if(pm_mask)
		    get_pm_value(skfd, ifname, &wrq, pm_mask, buffer);
		}
#endif /* WIRELESS_EXT > 9 */
	    }
	}
      printf("\n");
    }
}

/************************** TRANSMIT POWER **************************/

/*------------------------------------------------------------------*/
/*
 * Print the number of available transmit powers for the device
 */
static void
print_txpower_info(int		skfd,
		   char *	ifname)
{
  struct iw_range	range;
  int			dbm;
  int			mwatt;
  int			k;

#if WIRELESS_EXT > 9
  /* Extract range info */
  if(iw_get_range_info(skfd, ifname, &range) < 0)
      fprintf(stderr, "%-8.8s  no transmit-power information.\n\n",
		      ifname);
  else
    {
      if((range.num_txpower > 0) && (range.num_txpower < IW_MAX_TXPOWER))
	{
	  printf("%-8.8s  %d available transmit-powers :\n",
		 ifname, range.num_txpower);
	  /* Print them all */
	  for(k = 0; k < range.num_txpower; k++)
	    {
	      if(range.txpower_capa & IW_TXPOW_MWATT)
		{
		  dbm = iw_mwatt2dbm(range.txpower[k]);
		  mwatt = range.txpower[k];
		}
	      else
		{
		  dbm = range.txpower[k];
		  mwatt = iw_dbm2mwatt(range.txpower[k]);
		}
	      printf("\t  %d dBm  \t(%d mW)\n", dbm, mwatt);
	    }
	  printf("\n\n");
	}
      else
	printf("%-8.8s  No transmit-powers ? Please update driver...\n\n", ifname);
    }
#endif /* WIRELESS_EXT > 9 */
}

/*********************** RETRY LIMIT/LIFETIME ***********************/

#if WIRELESS_EXT > 10
/*------------------------------------------------------------------*/
/*
 * Print one retry value
 */
static inline int
get_retry_value(int		skfd,
		char *		ifname,
		struct iwreq *	pwrq,
		int		flags,
		char *		buffer)
{
  /* Get Another retry value */
  pwrq->u.retry.flags = flags;
  if(iw_get_ext(skfd, ifname, SIOCGIWRETRY, pwrq) >= 0)
    {
      /* Let's check the value and its type */
      if(pwrq->u.retry.flags & IW_RETRY_TYPE)
	{
	  iw_print_retry_value(buffer,
			       pwrq->u.retry.value, pwrq->u.retry.flags);
	  printf("%s\n                 ", buffer);
	}
    }
  return(pwrq->u.retry.flags);
}

/*------------------------------------------------------------------*/
/*
 * Print Retry info for each device
 */
static void
print_retry_info(int		skfd,
	         char *		ifname)
{
  struct iwreq		wrq;
  struct iw_range	range;
  char			buffer[128];

  /* Extract range info */
  if(iw_get_range_info(skfd, ifname, &range) < 0)
      fprintf(stderr, "%-8.8s  no retry limit/lifetime information.\n\n",
	      ifname);
  else
    {
      printf("%-8.8s  ", ifname);

      /* Display min/max limit availables */
      if(range.retry_flags & IW_RETRY_LIMIT)
	{
	  int	flags = (range.retry_flags & ~(IW_RETRY_MIN | IW_RETRY_MAX));
	  /* Display if auto or fixed */
	  if(range.retry_flags & IW_RETRY_MIN)
	    printf("Auto  limit    ; ");
	  else
	    printf("Fixed limit    ; ");
	  /* Print the range */
	  iw_print_retry_value(buffer, range.min_retry, flags | IW_RETRY_MIN);
	  printf("%s\n                           ", buffer);
	  iw_print_retry_value(buffer, range.max_retry, flags | IW_RETRY_MAX);
	  printf("%s\n          ", buffer);
	  
	}
      /* Display min/max lifetime availables */
      if(range.r_time_flags & IW_RETRY_LIFETIME)
	{
	  int	flags = (range.r_time_flags & ~(IW_RETRY_MIN | IW_RETRY_MAX));
	  /* Display if auto or fixed */
	  if(range.r_time_flags & IW_RETRY_MIN)
	    printf("Auto  lifetime ; ");
	  else
	    printf("Fixed lifetime ; ");
	  /* Print the range */
	  iw_print_retry_value(buffer, range.min_r_time, flags | IW_RETRY_MIN);
	  printf("%s\n                           ", buffer);
	  iw_print_retry_value(buffer, range.max_r_time, flags | IW_RETRY_MAX);
	  printf("%s\n          ", buffer);
	  
	}

      /* Get current retry settings */
      wrq.u.retry.flags = 0;
      if(iw_get_ext(skfd, ifname, SIOCGIWRETRY, &wrq) >= 0)
	{
	  int	flags = wrq.u.retry.flags;

	  /* Is it disabled ? */
	  if(wrq.u.retry.disabled)
	    printf("Current mode:off\n          ");
	  else
	    {
	      int	retry_mask = 0;

	      /* Let's check the mode */
	      printf("Current mode:on\n                 ");

	      /* Let's check the value and its type */
	      if(wrq.u.retry.flags & IW_RETRY_TYPE)
		{
		  iw_print_retry_value(buffer,
				       wrq.u.retry.value, wrq.u.retry.flags);
		  printf("%s", buffer);
		}

	      /* If we have been returned a MIN value, ask for the MAX */
	      if(flags & IW_RETRY_MIN)
		retry_mask = IW_RETRY_MAX;
	      /* If we have been returned a MAX value, ask for the MIN */
	      if(flags & IW_RETRY_MAX)
		retry_mask = IW_RETRY_MIN;
	      /* If we have something to ask for... */
	      if(retry_mask)
		get_retry_value(skfd, ifname, &wrq, retry_mask, buffer);

	      /* And if we have both a period and a timeout, ask the other */
	      retry_mask = (range.retry_capa & (~(wrq.u.retry.flags) &
					  IW_RETRY_TYPE));
	      if(retry_mask)
		{
		  int	base_mask = retry_mask;
		  flags = get_retry_value(skfd, ifname, &wrq, retry_mask,
					  buffer);
		  retry_mask = 0;

		  /* If we have been returned a MIN value, ask for the MAX */
		  if(flags & IW_RETRY_MIN)
		    retry_mask = IW_RETRY_MAX | base_mask;
		  /* If we have been returned a MAX value, ask for the MIN */
		  if(flags & IW_RETRY_MAX)
		    retry_mask = IW_RETRY_MIN | base_mask;
		  /* If we have something to ask for... */
		  if(retry_mask)
		    get_retry_value(skfd, ifname, &wrq, retry_mask, buffer);
		}
	    }
	}
      printf("\n");
    }
}

#endif	/* WIRELESS_EXT > 10 */

/************************* COMMON UTILITIES *************************/
/*
 * This section was written by Michael Tokarev <mjt@tls.msk.ru>
 */

/*------------------------------------------------------------------*/
/*
 * Enumerate devices and call specified routine
 */
static void
enum_devices(int skfd, void (*fn)(int skfd, char *ifname))
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
    (*fn)(skfd, ifr->ifr_name);
}

/* command list */
typedef struct iwlist_entry {
  const char *cmd;
  void (*fn)(int skfd, char *ifname);
} iwlist_cmd;

static const struct iwlist_entry iwlist_cmds[] = {
  { "frequency",    print_freq_info },
  { "channel",      print_freq_info },
  { "ap",           print_ap_info },
  { "accesspoints", print_ap_info },
  { "bitrate",      print_bitrate_info },
  { "rate",         print_bitrate_info },
  { "encryption",   print_keys_info },
  { "key",          print_keys_info },
  { "power",        print_pm_info },
  { "txpower",      print_txpower_info },
#if WIRELESS_EXT > 10
  { "retry",        print_retry_info },
#endif
  { NULL, 0 },
};

/* Display help */
static void usage(FILE *f)
{
  int i;
  for(i = 0; iwlist_cmds[i].cmd != NULL; ++i)
    fprintf(f, "%s [interface] %s\n",
            i ? "             " :
                "Usage: iwlist",
            iwlist_cmds[i].cmd);
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
  char *dev;			/* device name			*/
  char *cmd;			/* command			*/
  int i;

  if(argc == 1 || argc > 3)
    {
      usage(stderr);
      return 1;
    }
  if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
    {
      usage(stdout);
      return 0;
    }
  if (argc == 2)
    {
      cmd = argv[1];
      dev = NULL;
    }
  else
    {
      cmd = argv[2];
      dev = argv[1];
    }

  /* find a command */
  {
    int found = -1, ambig = 0;
    int len = strlen(cmd);
    for(i = 0; iwlist_cmds[i].cmd != NULL; ++i)
      {
	if (strncasecmp(iwlist_cmds[i].cmd, cmd, len) != 0)
	  continue;
	if (len == strlen(iwlist_cmds[i].cmd))	/* exact match */
	  {
	    found = i;
	    ambig = 0;
	    break;
	  }
	if (found < 0)
	  found = i;
	else if (iwlist_cmds[i].fn != iwlist_cmds[found].fn)
	  ambig = 1;
      }
    if (found < 0)
      {
	fprintf(stderr, "iwlist: unknown command `%s'\n", cmd);
	return 1;
      }
    if (ambig)
      {
	fprintf(stderr, "iwlist: command `%s' is ambiguous\n", cmd);
	return 1;
      }
    i = found;
  }

  /* Create a channel to the NET kernel. */
  if((skfd = iw_sockets_open()) < 0)
    {
      perror("socket");
      return -1;
    }

  /* do the actual work */
  if (dev)
    (*iwlist_cmds[i].fn)(skfd, dev);
  else
    enum_devices(skfd, iwlist_cmds[i].fn);

  /* Close the socket. */
  close(skfd);

  return 0;
}
