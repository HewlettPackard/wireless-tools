/*
 *	Wireless Tools
 *
 *		Jean II - HPLB '99 - HPL 99->01
 *
 * This tool can access various piece of information on the card
 * not part of iwconfig...
 * You need to link this code against "iwcommon.c" and "-lm".
 *
 * This file is released under the GPL license.
 */

#include "iwcommon.h"		/* Header */

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

  if(get_range_info(skfd, ifname, &range) < 0)
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
	{
	  /* Print stats for this address */
	  printf("    %s : ", pr_ether(hwa[i].sa_data));
	  print_stats(stdout, &qual[i], &range, has_range);
	}
      else
	/* Only print the address */
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
  if(get_range_info(skfd, ifname, &range) < 0)
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

  /* Extract range info */
  if(get_range_info(skfd, ifname, &range) < 0)
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
	  strcpy(wrq.ifr_name, ifname);
	  wrq.u.data.pointer = (caddr_t) key;
	  wrq.u.data.length = 0;
	  wrq.u.data.flags = k;
	  if(ioctl(skfd, SIOCGIWENCODE, &wrq) < 0)
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
	      printf("\t\t[%d]: ", k);
	      print_key(stdout, key, wrq.u.data.length, wrq.u.data.flags);

	      /* Other info... */
	      printf(" (%d bits)", wrq.u.data.length * 8);
	      printf("\n");
	    }
	}
      /* Print current key and mode */
      strcpy(wrq.ifr_name, ifname);
      wrq.u.data.pointer = (caddr_t) key;
      wrq.u.data.length = 0;
      wrq.u.data.flags = 0;	/* Set index to zero to get current */
      if(ioctl(skfd, SIOCGIWENCODE, &wrq) < 0)
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

/*------------------------------------------------------------------*/
/*
 * Get encryption info on all devices and print it on the screen
 */
static void
print_keys_devices(int		skfd)
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
    print_keys_info(skfd, ifr->ifr_name);
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
	     int		flags)
{
  /* Get Another Power Management value */
  strcpy(pwrq->ifr_name, ifname);
  pwrq->u.power.flags = flags;
  if(ioctl(skfd, SIOCGIWPOWER, pwrq) >= 0)
    {
      /* Let's check the value and its type */
      if(pwrq->u.power.flags & IW_POWER_TYPE)
	{
	  printf("\n                 ");
	  print_pm_value(stdout, pwrq->u.power.value, pwrq->u.power.flags);
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

  /* Extract range info */
  if(get_range_info(skfd, ifname, &range) < 0)
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
	  print_pm_value(stdout, range.min_pmp, flags | IW_POWER_MIN);
	  printf("\n                          ");
	  print_pm_value(stdout, range.max_pmp, flags | IW_POWER_MAX);
	  printf("\n          ");
	  
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
	  print_pm_value(stdout, range.min_pmt, flags | IW_POWER_MIN);
	  printf("\n                          ");
	  print_pm_value(stdout, range.max_pmt, flags | IW_POWER_MAX);
	  printf("\n          ");
	  
	}
#endif /* WIRELESS_EXT > 9 */

      /* Get current Power Management settings */
      strcpy(wrq.ifr_name, ifname);
      wrq.u.power.flags = 0;
      if(ioctl(skfd, SIOCGIWPOWER, &wrq) >= 0)
	{
	  int	flags = wrq.u.power.flags;

	  /* Is it disabled ? */
	  if(wrq.u.power.disabled)
	    printf("Current mode:off\n          ");
	  else
	    {
	      int	pm_mask = 0;

	      /* Let's check the mode */
	      printf("Current");
	      print_pm_mode(stdout, flags);

	      /* Let's check if nothing (simply on) */
	      if((flags & IW_POWER_MODE) == IW_POWER_ON)
		printf(" mode:on");
	      printf("\n                 ");

	      /* Let's check the value and its type */
	      if(wrq.u.power.flags & IW_POWER_TYPE)
		print_pm_value(stdout, wrq.u.power.value, wrq.u.power.flags);

	      /* If we have been returned a MIN value, ask for the MAX */
	      if(flags & IW_POWER_MIN)
		pm_mask = IW_POWER_MAX;
	      /* If we have been returned a MAX value, ask for the MIN */
	      if(flags & IW_POWER_MAX)
		pm_mask = IW_POWER_MIN;
	      /* If we have something to ask for... */
	      if(pm_mask)
		get_pm_value(skfd, ifname, &wrq, pm_mask);

#if WIRELESS_EXT > 9
	      /* And if we have both a period and a timeout, ask the other */
	      pm_mask = (range.pm_capa & (~(wrq.u.power.flags) &
					  IW_POWER_TYPE));
	      if(pm_mask)
		{
		  int	base_mask = pm_mask;
		  flags = get_pm_value(skfd, ifname, &wrq, pm_mask);
		  pm_mask = 0;

		  /* If we have been returned a MIN value, ask for the MAX */
		  if(flags & IW_POWER_MIN)
		    pm_mask = IW_POWER_MAX | base_mask;
		  /* If we have been returned a MAX value, ask for the MIN */
		  if(flags & IW_POWER_MAX)
		    pm_mask = IW_POWER_MIN | base_mask;
		  /* If we have something to ask for... */
		  if(pm_mask)
		    get_pm_value(skfd, ifname, &wrq, pm_mask);
		}
#endif /* WIRELESS_EXT > 9 */
	    }
	}
      printf("\n");
    }
}

/*------------------------------------------------------------------*/
/*
 * Get Power Management info on all devices and print it on the screen
 */
static void
print_pm_devices(int		skfd)
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
    print_pm_info(skfd, ifr->ifr_name);
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
  if(get_range_info(skfd, ifname, &range) < 0)
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
		  dbm = mwatt2dbm(range.txpower[k]);
		  mwatt = range.txpower[k];
		}
	      else
		{
		  dbm = range.txpower[k];
		  mwatt = dbm2mwatt(range.txpower[k]);
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

/*------------------------------------------------------------------*/
/*
 * Get tx-power info on all devices and print it on the screen
 */
static void
print_txpower_devices(int		skfd)
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
    print_txpower_info(skfd, ifr->ifr_name);
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
		int		flags)
{
  /* Get Another retry value */
  strcpy(pwrq->ifr_name, ifname);
  pwrq->u.retry.flags = flags;
  if(ioctl(skfd, SIOCGIWRETRY, pwrq) >= 0)
    {
      /* Let's check the value and its type */
      if(pwrq->u.retry.flags & IW_RETRY_TYPE)
	{
	  printf("\n                 ");
	  print_retry_value(stdout, pwrq->u.retry.value, pwrq->u.retry.flags);
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

  /* Extract range info */
  if(get_range_info(skfd, ifname, &range) < 0)
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
	  print_retry_value(stdout, range.min_retry, flags | IW_RETRY_MIN);
	  printf("\n                           ");
	  print_retry_value(stdout, range.max_retry, flags | IW_RETRY_MAX);
	  printf("\n          ");
	  
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
	  print_retry_value(stdout, range.min_r_time, flags | IW_RETRY_MIN);
	  printf("\n                           ");
	  print_retry_value(stdout, range.max_r_time, flags | IW_RETRY_MAX);
	  printf("\n          ");
	  
	}

      /* Get current retry settings */
      strcpy(wrq.ifr_name, ifname);
      wrq.u.retry.flags = 0;
      if(ioctl(skfd, SIOCGIWRETRY, &wrq) >= 0)
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
		print_retry_value(stdout, wrq.u.retry.value, wrq.u.retry.flags);

	      /* If we have been returned a MIN value, ask for the MAX */
	      if(flags & IW_RETRY_MIN)
		retry_mask = IW_RETRY_MAX;
	      /* If we have been returned a MAX value, ask for the MIN */
	      if(flags & IW_RETRY_MAX)
		retry_mask = IW_RETRY_MIN;
	      /* If we have something to ask for... */
	      if(retry_mask)
		get_retry_value(skfd, ifname, &wrq, retry_mask);

	      /* And if we have both a period and a timeout, ask the other */
	      retry_mask = (range.retry_capa & (~(wrq.u.retry.flags) &
					  IW_RETRY_TYPE));
	      if(retry_mask)
		{
		  int	base_mask = retry_mask;
		  flags = get_retry_value(skfd, ifname, &wrq, retry_mask);
		  retry_mask = 0;

		  /* If we have been returned a MIN value, ask for the MAX */
		  if(flags & IW_RETRY_MIN)
		    retry_mask = IW_RETRY_MAX | base_mask;
		  /* If we have been returned a MAX value, ask for the MIN */
		  if(flags & IW_RETRY_MAX)
		    retry_mask = IW_RETRY_MIN | base_mask;
		  /* If we have something to ask for... */
		  if(retry_mask)
		    get_retry_value(skfd, ifname, &wrq, retry_mask);
		}
	    }
	}
      printf("\n");
    }
}

/*------------------------------------------------------------------*/
/*
 * Get retry info on all devices and print it on the screen
 */
static void
print_retry_devices(int		skfd)
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
    print_retry_info(skfd, ifr->ifr_name);
}
#endif	/* WIRELESS_EXT > 10 */

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

  /* Create a channel to the NET kernel. */
  if((skfd = sockets_open()) < 0)
    {
      perror("socket");
      exit(-1);
    }

  /* Help */
  if((argc == 1) || (argc > 3) ||
     (!strncmp(argv[1], "-h", 9)) || (!strcmp(argv[1], "--help")))
    {
      fprintf(stderr, "Usage: iwlist [interface] freq\n");
      fprintf(stderr, "              [interface] ap\n");
      fprintf(stderr, "              [interface] bitrate\n");
      fprintf(stderr, "              [interface] keys\n");
      fprintf(stderr, "              [interface] power\n");
      fprintf(stderr, "              [interface] txpower\n");
      fprintf(stderr, "              [interface] retries\n");
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

  /* Access Point list */
  if(!strcasecmp(argv[1], "ap"))
    {
      print_ap_devices(skfd);
      close(skfd);
      exit(0);
    }

  /* Bit-rate list */
  if((!strncmp(argv[1], "bit", 3)) ||
     (!strcmp(argv[1], "rate")))
    {
      print_bitrate_devices(skfd);
      close(skfd);
      exit(0);
    }

  /* Encryption key list */
  if((!strncmp(argv[1], "enc", 3)) ||
     (!strncmp(argv[1], "key", 3)))
    {
      print_keys_devices(skfd);
      close(skfd);
      exit(0);
    }

  /* Power Management list */
  if(!strncmp(argv[1], "power", 3))
    {
      print_pm_devices(skfd);
      close(skfd);
      exit(0);
    }

  /* Transmit Power list */
  if(!strncmp(argv[1], "txpower", 3))
    {
      print_txpower_devices(skfd);
      close(skfd);
      exit(0);
    }

#if WIRELESS_EXT > 10
  /* Retry limit/lifetime */
  if(!strncmp(argv[1], "retry", 4))
    {
      print_retry_devices(skfd);
      close(skfd);
      exit(0);
    }
#endif

  /* Special cases take two... */
  /* Frequency list */
  if((!strncmp(argv[2], "freq", 4)) ||
     (!strncmp(argv[2], "channel", 7)))
    {
      print_freq_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

  /* Access Point  list */
  if(!strcasecmp(argv[2], "ap"))
    {
      print_ap_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

  /* Bit-rate list */
  if((!strncmp(argv[2], "bit", 3)) ||
     (!strcmp(argv[2], "rate")))
    {
      print_bitrate_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

  /* Encryption key list */
  if((!strncmp(argv[2], "enc", 3)) ||
     (!strncmp(argv[2], "key", 3)))
    {
      print_keys_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

  /* Power Management list */
  if(!strncmp(argv[2], "power", 3))
    {
      print_pm_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

  /* Transmit Power list */
  if(!strncmp(argv[2], "txpower", 3))
    {
      print_txpower_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

#if WIRELESS_EXT > 10
  /* Retry limit/lifetime */
  if(!strncmp(argv[2], "retry", 4))
    {
      print_retry_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }
#endif

  /* Close the socket. */
  close(skfd);

  return(1);
}
