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
#include <sys/time.h>

/*********************** FREQUENCIES/CHANNELS ***********************/

/*------------------------------------------------------------------*/
/*
 * Print the number of channels and available frequency for the device
 */
static int
print_freq_info(int		skfd,
		char *		ifname,
		char *		args[],		/* Command line args */
		int		count)		/* Args count */
{
  struct iwreq		wrq;
  struct iw_range	range;
  double		freq;
  int			k;
  int			channel;
  char			buffer[128];	/* Temporary buffer */

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

  /* Get list of frequencies / channels */
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
	      printf("          Channel %.2d : ", range.freq[k].i);
	      freq = iw_freq2float(&(range.freq[k]));
	      if(freq >= GIGA)
		printf("%g GHz\n", freq / GIGA);
	      else
		if(freq >= MEGA)
		  printf("%g MHz\n", freq / MEGA);
		else
		  printf("%g kHz\n", freq / KILO);
	    }
	}
      else
	printf("%-8.8s  %d channels\n",
	       ifname, range.num_channels);

      /* Get current frequency / channel and display it */
      if(iw_get_ext(skfd, ifname, SIOCGIWFREQ, &wrq) >= 0)
	{
	  freq = iw_freq2float(&(wrq.u.freq));
	  iw_print_freq(buffer, freq);
	  channel = iw_freq_to_channel(freq, &range);
	  if(channel >= 0)
	    printf("          Current %s (channel %.2d)\n\n", buffer, channel);
	  else
	    printf("          Current %s\n\n", buffer);
	}
    }
  return(0);
}

/************************ ACCESS POINT LIST ************************/
/*
 * Note : now that we have scanning support, this is depracted and
 * won't survive long. Actually, next version it's out !
 */

/*------------------------------------------------------------------*/
/*
 * Display the list of ap addresses and the associated stats
 * Exacly the same as the spy list, only with different IOCTL and messages
 */
static int
print_ap_info(int	skfd,
	      char *	ifname,
	      char *	args[],		/* Command line args */
	      int	count)		/* Args count */
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

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

  /* Collect stats */
  wrq.u.data.pointer = (caddr_t) buffer;
  wrq.u.data.length = IW_MAX_AP;
  wrq.u.data.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWAPLIST, &wrq) < 0)
    {
      fprintf(stderr, "%-8.8s  Interface doesn't have a list of Peers/Access-Points\n\n", ifname);
      return(-1);
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
      return(-2);
    }

  /* Get range info if we can */
  if(iw_get_range_info(skfd, ifname, &(range)) >= 0)
    has_range = 1;

  /* Display it */
  if(n == 0)
    printf("%-8.8s  No Peers/Access-Point in range\n", ifname);
  else
    printf("%-8.8s  Peers/Access-Points in range:\n", ifname);
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
  return(0);
}

/***************************** BITRATES *****************************/

/*------------------------------------------------------------------*/
/*
 * Print the number of available bitrates for the device
 */
static int
print_bitrate_info(int		skfd,
		   char *	ifname,
		   char *	args[],		/* Command line args */
		   int		count)		/* Args count */
{
  struct iwreq		wrq;
  struct iw_range	range;
  int			k;
  char			buffer[128];

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

  /* Extract range info */
  if(iw_get_range_info(skfd, ifname, &range) < 0)
      fprintf(stderr, "%-8.8s  no bit-rate information.\n\n",
		      ifname);
  else
    {
      if((range.num_bitrates > 0) && (range.num_bitrates <= IW_MAX_BITRATES))
	{
	  printf("%-8.8s  %d available bit-rates :\n",
		 ifname, range.num_bitrates);
	  /* Print them all */
	  for(k = 0; k < range.num_bitrates; k++)
	    {
	      iw_print_bitrate(buffer, range.bitrate[k]);
	      /* Maybe this should be %10s */
	      printf("\t  %s\n", buffer);
	    }
	}
      else
	printf("%-8.8s  No bit-rates ? Please update driver...\n", ifname);

      /* Get current bit rate */
      if(iw_get_ext(skfd, ifname, SIOCGIWRATE, &wrq) >= 0)
	{
	  iw_print_bitrate(buffer, wrq.u.bitrate.value);
	  printf("          Current Bit Rate%c%s\n\n",
		 (wrq.u.bitrate.fixed ? '=' : ':'), buffer);
	}
    }
  return(0);
}

/************************* ENCRYPTION KEYS *************************/

/*------------------------------------------------------------------*/
/*
 * Print the number of available encryption key for the device
 */
static int
print_keys_info(int		skfd,
		char *		ifname,
		char *		args[],		/* Command line args */
		int		count)		/* Args count */
{
  struct iwreq		wrq;
  struct iw_range	range;
  unsigned char		key[IW_ENCODING_TOKEN_MAX];
  int			k;
  char			buffer[128];

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

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
	  return(-1);
	}
      printf("          Current Transmit Key: [%d]\n",
	     wrq.u.data.flags & IW_ENCODE_INDEX);
      if(wrq.u.data.flags & IW_ENCODE_RESTRICTED)
	printf("          Security mode:restricted\n");
      if(wrq.u.data.flags & IW_ENCODE_OPEN)
	printf("          Security mode:open\n");

      printf("\n\n");
    }
  return(0);
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
static int
print_pm_info(int		skfd,
	      char *		ifname,
	      char *		args[],		/* Command line args */
	      int		count)		/* Args count */
{
  struct iwreq		wrq;
  struct iw_range	range;
  char			buffer[128];

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

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
	      printf("Current %s", buffer);

	      /* Let's check if nothing (simply on) */
	      if((flags & IW_POWER_MODE) == IW_POWER_ON)
		printf("mode:on");
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
  return(0);
}

/************************** TRANSMIT POWER **************************/

/*------------------------------------------------------------------*/
/*
 * Print the number of available transmit powers for the device
 */
static int
print_txpower_info(int		skfd,
		   char *	ifname,
		   char *	args[],		/* Command line args */
		   int		count)		/* Args count */
{
  struct iwreq		wrq;
  struct iw_range	range;
  int			dbm;
  int			mwatt;
  int			k;

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

#if WIRELESS_EXT > 9
  /* Extract range info */
  if(iw_get_range_info(skfd, ifname, &range) < 0)
      fprintf(stderr, "%-8.8s  no transmit-power information.\n\n",
		      ifname);
  else
    {
      if((range.num_txpower <= 0) || (range.num_txpower > IW_MAX_TXPOWER))
	printf("%-8.8s  No transmit-powers ? Please update driver...\n\n", ifname);
      else
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

	  /* Get current Transmit Power */
	  if(iw_get_ext(skfd, ifname, SIOCGIWTXPOW, &wrq) >= 0)
	    {
	      printf("          Current Tx-Power");
	      /* Disabled ? */
	      if(wrq.u.txpower.disabled)
		printf(":off\n\n");
	      else
		{
		  /* Fixed ? */
		  if(wrq.u.txpower.fixed)
		    printf("=");
		  else
		    printf(":");
		  if(wrq.u.txpower.flags & IW_TXPOW_MWATT)
		    {
		      dbm = iw_mwatt2dbm(wrq.u.txpower.value);
		      mwatt = wrq.u.txpower.value;
		    }
		  else
		    {
		      dbm = wrq.u.txpower.value;
		      mwatt = iw_dbm2mwatt(wrq.u.txpower.value);
		    }
		  printf("%d dBm  \t(%d mW)\n\n", dbm, mwatt);
		}
	    }
	}
    }
#endif /* WIRELESS_EXT > 9 */
  return(0);
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
static int
print_retry_info(int		skfd,
		 char *		ifname,
		 char *		args[],		/* Command line args */
		 int		count)		/* Args count */
{
  struct iwreq		wrq;
  struct iw_range	range;
  char			buffer[128];

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

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
  return(0);
}

#endif	/* WIRELESS_EXT > 10 */

/***************************** SCANNING *****************************/
/*
 * This one behave quite differently from the others
 */
#if WIRELESS_EXT > 13
/*------------------------------------------------------------------*/
/*
 * Print one element from the scanning results
 */
static inline int
print_scanning_token(struct iw_event *	event,	/* Extracted token */
		     int		ap_num,	/* AP number */
		     struct iw_range *	iwrange,	/* Range info */
		     int		has_range)
{
  char		buffer[128];	/* Temporary buffer */

  /* Now, let's decode the event */
  switch(event->cmd)
    {
    case SIOCGIWAP:
      printf("          Cell %02d - Address: %s\n", ap_num,
	     iw_pr_ether(buffer, event->u.ap_addr.sa_data));
      ap_num++;
      break;
    case SIOCGIWNWID:
      if(event->u.nwid.disabled)
	printf("                    NWID:off/any\n");
      else
	printf("                    NWID:%X\n", event->u.nwid.value);
      break;
    case SIOCGIWFREQ:
      {
	double		freq;			/* Frequency/channel */
	freq = iw_freq2float(&(event->u.freq));
	iw_print_freq(buffer, freq);
	printf("                    %s\n", buffer);
      }
      break;
    case SIOCGIWMODE:
      printf("                    Mode:%s\n",
	     iw_operation_mode[event->u.mode]);
      break;
    case SIOCGIWNAME:
      printf("                    Protocol:%-1.16s\n", event->u.name);
      break;
    case SIOCGIWESSID:
      {
	char essid[IW_ESSID_MAX_SIZE+1];
	if((event->u.essid.pointer) && (event->u.essid.length))
	  memcpy(essid, event->u.essid.pointer, event->u.essid.length);
	essid[event->u.essid.length] = '\0';
	if(event->u.essid.flags)
	  {
	    /* Does it have an ESSID index ? */
	    if((event->u.essid.flags & IW_ENCODE_INDEX) > 1)
	      printf("                    ESSID:\"%s\" [%d]\n", essid,
		     (event->u.essid.flags & IW_ENCODE_INDEX));
	    else
	      printf("                    ESSID:\"%s\"\n", essid);
	  }
	else
	  printf("                    ESSID:off/any\n");
      }
      break;
    case SIOCGIWENCODE:
      {
	unsigned char	key[IW_ENCODING_TOKEN_MAX];
	if(event->u.data.pointer)
	  memcpy(key, event->u.essid.pointer, event->u.data.length);
	else
	  event->u.data.flags |= IW_ENCODE_NOKEY;
	printf("                    Encryption key:");
	if(event->u.data.flags & IW_ENCODE_DISABLED)
	  printf("off\n");
	else
	  {
	    /* Display the key */
	    iw_print_key(buffer, key, event->u.data.length,
			 event->u.data.flags);
	    printf("%s", buffer);

	    /* Other info... */
	    if((event->u.data.flags & IW_ENCODE_INDEX) > 1)
	      printf(" [%d]", event->u.data.flags & IW_ENCODE_INDEX);
	    if(event->u.data.flags & IW_ENCODE_RESTRICTED)
	      printf("   Security mode:restricted");
	    if(event->u.data.flags & IW_ENCODE_OPEN)
	      printf("   Security mode:open");
	    printf("\n");
	  }
      }
      break;
    case SIOCGIWRATE:
      iw_print_bitrate(buffer, event->u.bitrate.value);
      printf("                    Bit Rate:%s\n", buffer);
      break;
    case IWEVQUAL:
      {
	event->u.qual.updated = 0x0;	/* Not that reliable, disable */
	iw_print_stats(buffer, &event->u.qual, iwrange, has_range);
	printf("                    %s\n", buffer);
	break;
      }
#if WIRELESS_EXT > 14
    case IWEVCUSTOM:
      {
	char custom[IW_CUSTOM_MAX+1];
	if((event->u.data.pointer) && (event->u.data.length))
	  memcpy(custom, event->u.data.pointer, event->u.data.length);
	custom[event->u.data.length] = '\0';
	printf("                    Extra:%s\n", custom);
      }
      break;
#endif /* WIRELESS_EXT > 14 */
    default:
      printf("                    (Unknown Wireless Token 0x%04X)\n",
	     event->cmd);
   }	/* switch(event->cmd) */

  /* May have changed */
  return(ap_num);
}

/*------------------------------------------------------------------*/
/*
 * Perform a scanning on one device
 */
static int
print_scanning_info(int		skfd,
		    char *	ifname,
		    char *	args[],		/* Command line args */
		    int		count)		/* Args count */
{
  struct iwreq		wrq;
  unsigned char		buffer[IW_SCAN_MAX_DATA];	/* Results */
  struct timeval	tv;				/* Select timeout */
  int			timeout = 5000000;		/* 5s */

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

  /* Init timeout value -> 250ms*/
  tv.tv_sec = 0;
  tv.tv_usec = 250000;

  /*
   * Here we should look at the command line args and set the IW_SCAN_ flags
   * properly
   */
  wrq.u.param.flags = IW_SCAN_DEFAULT;
  wrq.u.param.value = 0;		/* Later */

  /* Initiate Scanning */
  if(iw_set_ext(skfd, ifname, SIOCSIWSCAN, &wrq) < 0)
    {
      if(errno != EPERM)
	{
	  fprintf(stderr, "%-8.8s  Interface doesn't support scanning : %s\n\n",
		  ifname, strerror(errno));
	  return(-1);
	}
      /* If we don't have the permission to initiate the scan, we may
       * still have permission to read left-over results.
       * But, don't wait !!! */
#if 0
      /* Not cool, it display for non wireless interfaces... */
      fprintf(stderr, "%-8.8s  (Could not trigger scanning, just reading left-over results)\n", ifname);
#endif
      tv.tv_usec = 0;
    }
  timeout -= tv.tv_usec;

  /* Forever */
  while(1)
    {
      fd_set		rfds;		/* File descriptors for select */
      int		last_fd;	/* Last fd */
      int		ret;

      /* Guess what ? We must re-generate rfds each time */
      FD_ZERO(&rfds);
      last_fd = -1;

      /* In here, add the rtnetlink fd in the list */

      /* Wait until something happens */
      ret = select(last_fd + 1, &rfds, NULL, NULL, &tv);

      /* Check if there was an error */
      if(ret < 0)
	{
	  if(errno == EAGAIN || errno == EINTR)
	    continue;
	  fprintf(stderr, "Unhandled signal - exiting...\n");
	  return(-1);
	}

      /* Check if there was a timeout */
      if(ret == 0)
	{
	  /* Try to read the results */
	  wrq.u.data.pointer = buffer;
	  wrq.u.data.flags = 0;
	  wrq.u.data.length = sizeof(buffer);
	  if(iw_get_ext(skfd, ifname, SIOCGIWSCAN, &wrq) < 0)
	    {
	      /* Check if results not available yet */
	      if(errno == EAGAIN)
		{
		  /* Restart timer for only 100ms*/
		  tv.tv_sec = 0;
		  tv.tv_usec = 100000;
		  timeout -= tv.tv_usec;
		  if(timeout > 0)
		    continue;	/* Try again later */
		}

	      /* Bad error */
	      fprintf(stderr, "%-8.8s  Failed to read scan data : %s\n\n",
		      ifname, strerror(errno));
	      return(-2);
	    }
	  else
	    /* We have the results, go to process them */
	    break;
	}

      /* In here, check if event and event type
       * if scan event, read results. All errors bad & no reset timeout */
    }

  if(wrq.u.data.length)
    {
      struct iw_event		iwe;
      struct stream_descr	stream;
      int			ap_num = 1;
      int			ret;
      struct iw_range		range;
      int			has_range;
#if 0
      /* Debugging code. In theory useless, because it's debugged ;-) */
      int	i;
      printf("Scan result [%02X", buffer[0]);
      for(i = 1; i < wrq.u.data.length; i++)
	printf(":%02X", buffer[i]);
      printf("]\n");
#endif
      has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);
      printf("%-8.8s  Scan completed :\n", ifname);
      iw_init_event_stream(&stream, buffer, wrq.u.data.length);
      do
	{
	  /* Extract an event and print it */
	  ret = iw_extract_event_stream(&stream, &iwe);
	  if(ret > 0)
	    ap_num = print_scanning_token(&iwe, ap_num, &range, has_range);
	}
      while(ret > 0);
      printf("\n");
    }
  else
    printf("%-8.8s  No scan results\n", ifname);

  return(0);
}
#endif	/* WIRELESS_EXT > 13 */

/************************* COMMON UTILITIES *************************/
/*
 * This section was written by Michael Tokarev <mjt@tls.msk.ru>
 * But modified by me ;-)
 */

/* command list */
typedef struct iwlist_entry {
  const char *cmd;
  iw_enum_handler fn;
  int min_count;
  int max_count;
} iwlist_cmd;

static const struct iwlist_entry iwlist_cmds[] = {
  { "frequency",	print_freq_info,	0, 0 },
  { "channel",		print_freq_info,	0, 0 },
  { "ap",		print_ap_info,		0, 0 },
  { "accesspoints",	print_ap_info,		0, 0 },
  { "peers",		print_ap_info,		0, 0 },
  { "bitrate",		print_bitrate_info,	0, 0 },
  { "rate",		print_bitrate_info,	0, 0 },
  { "encryption",	print_keys_info,	0, 0 },
  { "key",		print_keys_info,	0, 0 },
  { "power",		print_pm_info,		0, 0 },
  { "txpower",		print_txpower_info,	0, 0 },
#if WIRELESS_EXT > 10
  { "retry",		print_retry_info,	0, 0 },
#endif
#if WIRELESS_EXT > 13
  { "scanning",		print_scanning_info,	0, 5 },
#endif
  { NULL, NULL, 0, 0 },
};

/*------------------------------------------------------------------*/
/*
 * Find the most appropriate command matching the command line
 */
static inline const iwlist_cmd *
find_command(const char *	cmd)
{
  const iwlist_cmd *	found = NULL;
  int			ambig = 0;
  unsigned int		len = strlen(cmd);
  int			i;

  /* Go through all commands */
  for(i = 0; iwlist_cmds[i].cmd != NULL; ++i)
    {
      /* No match -> next one */
      if(strncasecmp(iwlist_cmds[i].cmd, cmd, len) != 0)
	continue;

      /* Exact match -> perfect */
      if(len == strlen(iwlist_cmds[i].cmd))
	return &iwlist_cmds[i];

      /* Partial match */
      if(found == NULL)
	/* First time */
	found = &iwlist_cmds[i];
      else
	/* Another time */
	if (iwlist_cmds[i].fn != found->fn)
	  ambig = 1;
    }

  if(found == NULL)
    {
      fprintf(stderr, "iwlist: unknown command `%s'\n", cmd);
      return NULL;
    }

  if(ambig)
    {
      fprintf(stderr, "iwlist: command `%s' is ambiguous\n", cmd);
      return NULL;
    }

  return found;
}

/*------------------------------------------------------------------*/
/*
 * Display help
 */
static void iw_usage(int status)
{
  FILE* f = status ? stderr : stdout;
  int i;

  fprintf(f,   "Usage: iwlist [interface] %s\n", iwlist_cmds[0].cmd);
  for(i = 1; iwlist_cmds[i].cmd != NULL; ++i)
    fprintf(f, "              [interface] %s\n", iwlist_cmds[i].cmd);
  exit(status);
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
  int skfd;			/* generic raw socket desc.	*/
  char *dev;			/* device name			*/
  char *cmd;			/* command			*/
  char **args;			/* Command arguments */
  int count;			/* Number of arguments */
  const iwlist_cmd *iwcmd;

  if(argc == 1 || argc > 3)
    iw_usage(1);

  if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
    iw_usage(0);

  /* This is also handled slightly differently */
  if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
    return(iw_print_version_info("iwlist"));

  if (argc == 2)
    {
      cmd = argv[1];
      dev = NULL;
      args = NULL;
      count = 0;
    }
  else
    {
      cmd = argv[2];
      dev = argv[1];
      args = argv + 3;
      count = argc - 3;
    }

  /* find a command */
  iwcmd = find_command(cmd);
  if(iwcmd == NULL)
    return 1;

  /* Check arg numbers */
  if(count < iwcmd->min_count)
    {
      fprintf(stderr, "iwlist: command `%s' needs more arguments\n", cmd);
      return 1;
    }
  if(count > iwcmd->max_count)
    {
      fprintf(stderr, "iwlist: command `%s' needs fewer arguments\n", cmd);
      return 1;
    }

  /* Create a channel to the NET kernel. */
  if((skfd = iw_sockets_open()) < 0)
    {
      perror("socket");
      return -1;
    }

  /* do the actual work */
  if (dev)
    (*iwcmd->fn)(skfd, dev, args, count);
  else
    iw_enum_devices(skfd, iwcmd->fn, args, count);

  /* Close the socket. */
  close(skfd);

  return 0;
}
