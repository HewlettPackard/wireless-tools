/*
 *	Wireless Tools
 *
 *		Jean II - HPLB 97->99 - HPL 99->01
 *
 * Main code for "iwconfig". This is the generic tool for most
 * manipulations...
 * You need to link this code against "iwcommon.c" and "-lm".
 *
 * This file is released under the GPL license.
 */

#include "iwcommon.h"		/* Header */

/**************************** VARIABLES ****************************/
char *	operation_mode[] = { "Auto",
			     "Ad-Hoc",
			     "Managed",
			     "Master",
			     "Repeater",
			     "Secondary" };

/************************* MISC SUBROUTINES **************************/

/*------------------------------------------------------------------*/
/*
 * Print usage string
 */
static void
iw_usage(void)
{
  fprintf(stderr, "Usage: iwconfig interface [essid {NN|on|off}]\n");
  fprintf(stderr, "                          [nwid {NN|on|off}]\n");
  fprintf(stderr, "                          [freq N.NNNN[k|M|G]]\n");
  fprintf(stderr, "                          [channel N]\n");
  fprintf(stderr, "                          [sens N]\n");
  fprintf(stderr, "                          [nick N]\n");
  fprintf(stderr, "                          [rate {N|auto|fixed}]\n");
  fprintf(stderr, "                          [rts {N|auto|fixed|off}]\n");
  fprintf(stderr, "                          [frag {N|auto|fixed|off}]\n");
  fprintf(stderr, "                          [enc NNNN-NNNN]\n");
  fprintf(stderr, "                          [power { period N|timeout N}]\n");
  fprintf(stderr, "                          [txpower N {mW|dBm}]\n");
  exit(1);
}


/************************* DISPLAY ROUTINES **************************/

/*------------------------------------------------------------------*/
/*
 * Read /proc/net/wireless to get the latest statistics
 */
static int
iw_getstats(char *	ifname,
	    iwstats *	stats)
{
  FILE *	f=fopen("/proc/net/wireless","r");
  char		buf[256];
  char *	bp;
  int		t;
  if(f==NULL)
    return -1;
  /* Loop on all devices */
  while(fgets(buf,255,f))
    {
      bp=buf;
      while(*bp&&isspace(*bp))
	bp++;
      /* Is it the good device ? */
      if(strncmp(bp,ifname,strlen(ifname))==0 && bp[strlen(ifname)]==':')
  	{
	  /* Skip ethX: */
	  bp=strchr(bp,':');
	  bp++;
	  /* -- status -- */
	  bp = strtok(bp, " ");
	  sscanf(bp, "%X", &t);
	  stats->status = (unsigned short) t;
	  /* -- link quality -- */
	  bp = strtok(NULL, " ");
	  if(strchr(bp,'.') != NULL)
	    stats->qual.updated |= 1;
	  sscanf(bp, "%d", &t);
	  stats->qual.qual = (unsigned char) t;
	  /* -- signal level -- */
	  bp = strtok(NULL, " ");
	  if(strchr(bp,'.') != NULL)
	    stats->qual.updated |= 2;
	  sscanf(bp, "%d", &t);
	  stats->qual.level = (unsigned char) t;
	  /* -- noise level -- */
	  bp = strtok(NULL, " ");
	  if(strchr(bp,'.') != NULL)
	    stats->qual.updated += 4;
	  sscanf(bp, "%d", &t);
	  stats->qual.noise = (unsigned char) t;
	  /* -- discarded packets -- */
	  bp = strtok(NULL, " ");
	  sscanf(bp, "%d", &stats->discard.nwid);
	  bp = strtok(NULL, " ");
	  sscanf(bp, "%d", &stats->discard.code);
	  bp = strtok(NULL, " ");
	  sscanf(bp, "%d", &stats->discard.misc);
	  fclose(f);
	  return 0;
  	}
    }
  fclose(f);
  return -1;
}

/*------------------------------------------------------------------*/
/*
 * Get wireless informations & config from the device driver
 * We will call all the classical wireless ioctl on the driver through
 * the socket to know what is supported and to get the settings...
 */
static int
get_info(int			skfd,
	 char *			ifname,
	 struct wireless_info *	info)
{
  struct iwreq		wrq;

  memset((char *) info, 0, sizeof(struct wireless_info));

  /* Get wireless name */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWNAME, &wrq) < 0)
    /* If no wireless name : no wireless extensions */
    return(-1);
  else
    strcpy(info->name, wrq.u.name);

  /* Get ranges */
  if(get_range_info(skfd, ifname, &(info->range)) >= 0)
    info->has_range = 1;

  /* Get network ID */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWNWID, &wrq) >= 0)
    {
      info->has_nwid = 1;
      memcpy(&(info->nwid), &(wrq.u.nwid), sizeof(iwparam));
    }

  /* Get frequency / channel */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWFREQ, &wrq) >= 0)
    {
      info->has_freq = 1;
      info->freq = freq2float(&(wrq.u.freq));
    }

  /* Get sensitivity */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWSENS, &wrq) >= 0)
    {
      info->has_sens = 1;
      memcpy(&(info->sens), &(wrq.u.sens), sizeof(iwparam));
    }

  /* Get encryption information */
  strcpy(wrq.ifr_name, ifname);
  wrq.u.data.pointer = (caddr_t) info->key;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCGIWENCODE, &wrq) >= 0)
    {
      info->has_key = 1;
      info->key_size = wrq.u.data.length;
      info->key_flags = wrq.u.data.flags;
    }

  /* Get ESSID */
  strcpy(wrq.ifr_name, ifname);
  wrq.u.essid.pointer = (caddr_t) info->essid;
  wrq.u.essid.length = 0;
  wrq.u.essid.flags = 0;
  if(ioctl(skfd, SIOCGIWESSID, &wrq) >= 0)
    {
      info->has_essid = 1;
      info->essid_on = wrq.u.data.flags;
    }

  /* Get AP address */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWAP, &wrq) >= 0)
    {
      info->has_ap_addr = 1;
      memcpy(&(info->ap_addr), &(wrq.u.ap_addr), sizeof (sockaddr));
    }

  /* Get NickName */
  strcpy(wrq.ifr_name, ifname);
  wrq.u.essid.pointer = (caddr_t) info->nickname;
  wrq.u.essid.length = 0;
  wrq.u.essid.flags = 0;
  if(ioctl(skfd, SIOCGIWNICKN, &wrq) >= 0)
    if(wrq.u.data.length > 1)
      info->has_nickname = 1;

  /* Get bit rate */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWRATE, &wrq) >= 0)
    {
      info->has_bitrate = 1;
      memcpy(&(info->bitrate), &(wrq.u.bitrate), sizeof(iwparam));
    }

  /* Get RTS threshold */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWRTS, &wrq) >= 0)
    {
      info->has_rts = 1;
      memcpy(&(info->rts), &(wrq.u.rts), sizeof(iwparam));
    }

  /* Get fragmentation threshold */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWFRAG, &wrq) >= 0)
    {
      info->has_frag = 1;
      memcpy(&(info->frag), &(wrq.u.frag), sizeof(iwparam));
    }

  /* Get operation mode */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWMODE, &wrq) >= 0)
    {
      if((wrq.u.mode < 6) && (wrq.u.mode >= 0))
	info->has_mode = 1;
      info->mode = wrq.u.mode;
    }

  /* Get Power Management settings */
  strcpy(wrq.ifr_name, ifname);
  wrq.u.power.flags = 0;
  if(ioctl(skfd, SIOCGIWPOWER, &wrq) >= 0)
    {
      info->has_power = 1;
      memcpy(&(info->power), &(wrq.u.power), sizeof(iwparam));
    }

#if WIRELESS_EXT > 9
  /* Get Transmit Power */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWTXPOW, &wrq) >= 0)
    {
      info->has_txpower = 1;
      memcpy(&(info->txpower), &(wrq.u.txpower), sizeof(iwparam));
    }
#endif

#if WIRELESS_EXT > 10
  /* Get retry limit/lifetime */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWRETRY, &wrq) >= 0)
    {
      info->has_retry = 1;
      memcpy(&(info->retry), &(wrq.u.retry), sizeof(iwparam));
    }
#endif	/* WIRELESS_EXT > 10 */

  /* Get stats */
  if(iw_getstats(ifname, &(info->stats)) >= 0)
    {
      info->has_stats = 1;
    }

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Print on the screen in a neat fashion all the info we have collected
 * on a device.
 */
static void
display_info(struct wireless_info *	info,
	     char *			ifname)
{
  /* One token is more of less 5 character, 14 tokens per line */
  int	tokens = 3;	/* For name */

  /* Display device name and wireless name (name of the protocol used) */
  printf("%-8.8s  %s  ", ifname, info->name);

  /* Display ESSID (extended network), if any */
  if(info->has_essid)
    {
      if(info->essid_on)
	{
	  /* Does it have an ESSID index ? */
	  if((info->essid_on & IW_ENCODE_INDEX) > 1)
	    printf("ESSID:\"%s\" [%d]  ", info->essid,
		   (info->essid_on & IW_ENCODE_INDEX));
	  else
	    printf("ESSID:\"%s\"  ", info->essid);
	}
      else
	printf("ESSID:off  ");
    }

  /* Display NickName (station name), if any */
  if(info->has_nickname)
    printf("Nickname:\"%s\"", info->nickname);

  /* Formatting */
  if(info->has_essid || info->has_nickname)
    {
      printf("\n          ");
      tokens = 0;
    }

  /* Display Network ID */
  if(info->has_nwid)
    {
      /* Note : should display right number of digit according to info
       * in range structure */
      if(info->nwid.disabled)
	printf("NWID:off/any  ");
      else
	printf("NWID:%X  ", info->nwid.value);
      tokens +=2;
    }

  /* Display the current mode of operation */
  if(info->has_mode)
    {
      printf("Mode:%s  ", operation_mode[info->mode]);
      tokens +=3;
    }

  /* Display frequency / channel */
  if(info->has_freq)
    {
      if(info->freq < KILO)
	printf("Channel:%g  ", info->freq);
      else
	{
	  if(info->freq >= GIGA)
	    printf("Frequency:%gGHz  ", info->freq / GIGA);
	  else
	    {
	      if(info->freq >= MEGA)
		printf("Frequency:%gMHz  ", info->freq / MEGA);
	      else
		printf("Frequency:%gkHz  ", info->freq / KILO);
	    }
	}
      tokens +=4;
    }

  /* Display the address of the current Access Point */
  if(info->has_ap_addr)
    {
      /* A bit of clever formatting */
      if(tokens > 8)
	{
	  printf("\n          ");
	  tokens = 0;
	}
      tokens +=6;

      /* Oups ! No Access Point in Ad-Hoc mode */
      if((info->has_mode) && (info->mode == IW_MODE_ADHOC))
	printf("Cell:");
      else
	printf("Access Point:");
      printf(" %s", pr_ether(info->ap_addr.sa_data));
    }

  /* Display the currently used/set bit-rate */
  if(info->has_bitrate)
    {
      /* A bit of clever formatting */
      if(tokens > 11)
	{
	  printf("\n          ");
	  tokens = 0;
	}
      tokens +=3;

      /* Fixed ? */
      if(info->bitrate.fixed)
	printf("Bit Rate=");
      else
	printf("Bit Rate:");

      if(info->bitrate.value >= GIGA)
	printf("%gGb/s", info->bitrate.value / GIGA);
      else
	if(info->bitrate.value >= MEGA)
	  printf("%gMb/s", info->bitrate.value / MEGA);
	else
	  printf("%gkb/s", info->bitrate.value / KILO);
      printf("   ");
    }

#if WIRELESS_EXT > 9
  /* Display the Transmit Power */
  if(info->has_txpower)
    {
      /* A bit of clever formatting */
      if(tokens > 11)
	{
	  printf("\n          ");
	  tokens = 0;
	}
      tokens +=3;

      /* Disabled ? */
      if(info->txpower.disabled)
	printf("Tx-Power:off   ");
      else
	{
	  int		dbm;

	  /* Fixed ? */
	  if(info->txpower.fixed)
	    printf("Tx-Power=");
	  else
	    printf("Tx-Power:");

	  /* Convert everything to dBm */
	  if(info->txpower.flags & IW_TXPOW_MWATT)
	    dbm = mwatt2dbm(info->txpower.value);
	  else
	    dbm = info->txpower.value;

	  /* Display */
	  printf("%d dBm   ", dbm);
	}
    }
#endif

  /* Display sensitivity */
  if(info->has_sens)
    {
      /* A bit of clever formatting */
      if(tokens > 10)
	{
	  printf("\n          ");
	  tokens = 0;
	}
      tokens +=4;

      /* Fixed ? */
      if(info->sens.fixed)
	printf("Sensitivity=");
      else
	printf("Sensitivity:");

      if(info->has_range)
	/* Display in dBm ? */
	if(info->sens.value < 0)
	  printf("%d dBm  ", info->sens.value);
	else
	  printf("%d/%d  ", info->sens.value, info->range.sensitivity);
      else
	printf("%d  ", info->sens.value);
    }

  printf("\n          ");
  tokens = 0;

#if WIRELESS_EXT > 10
  /* Display retry limit/lifetime information */
  if(info->has_retry)
    { 
      printf("Retry");
      /* Disabled ? */
      if(info->retry.disabled)
	printf(":off");
      else
	{
	  /* Let's check the value and its type */
	  if(info->retry.flags & IW_RETRY_TYPE)
	    print_retry_value(stdout, info->retry.value, info->retry.flags);

	  /* Let's check if nothing (simply on) */
	  if(info->retry.flags == IW_RETRY_ON)
	    printf(":on");
 	}
      printf("   ");
      tokens += 5;	/* Between 3 and 5, depend on flags */
    }
#endif	/* WIRELESS_EXT > 10 */

  /* Display the RTS threshold */
  if(info->has_rts)
    {
      /* Disabled ? */
      if(info->rts.disabled)
	printf("RTS thr:off   ");
      else
	{
	  /* Fixed ? */
	  if(info->rts.fixed)
	    printf("RTS thr=");
	  else
	    printf("RTS thr:");

	  printf("%d B   ", info->rts.value);
	}
      tokens += 3;
    }

  /* Display the fragmentation threshold */
  if(info->has_frag)
    {
      /* A bit of clever formatting */
      if(tokens > 10)
	{
	  printf("\n          ");
	  tokens = 0;
	}
      tokens +=4;

      /* Disabled ? */
      if(info->frag.disabled)
	printf("Fragment thr:off");
      else
	{
	  /* Fixed ? */
	  if(info->frag.fixed)
	    printf("Fragment thr=");
	  else
	    printf("Fragment thr:");

	  printf("%d B   ", info->frag.value);
	}
    }

  /* Formating */
  if(tokens > 0)
    printf("\n          ");

  /* Display encryption information */
  /* Note : we display only the "current" key, use iwlist to list all keys */
  if(info->has_key)
    {
      printf("Encryption key:");
      if((info->key_flags & IW_ENCODE_DISABLED) || (info->key_size == 0))
	printf("off\n          ");
      else
	{
	  /* Display the key */
	  print_key(stdout, info->key, info->key_size, info->key_flags);

	  /* Other info... */
	  if((info->key_flags & IW_ENCODE_INDEX) > 1)
	    printf(" [%d]", info->key_flags & IW_ENCODE_INDEX);
	  if(info->key_flags & IW_ENCODE_RESTRICTED)
	    printf("   Encryption mode:restricted");
	  if(info->key_flags & IW_ENCODE_OPEN)
	    printf("   Encryption mode:open");
	  printf("\n          ");
 	}
    }

  /* Display Power Management information */
  /* Note : we display only one parameter, period or timeout. If a device
   * (such as HiperLan) has both, the user need to use iwlist... */
  if(info->has_power)	/* I hope the device has power ;-) */
    { 
      printf("Power Management");
      /* Disabled ? */
      if(info->power.disabled)
	printf(":off\n          ");
      else
	{
	  /* Let's check the value and its type */
	  if(info->power.flags & IW_POWER_TYPE)
	    print_pm_value(stdout, info->power.value, info->power.flags);

	  /* Let's check the mode */
	  print_pm_mode(stdout, info->power.flags);

	  /* Let's check if nothing (simply on) */
	  if(info->power.flags == IW_POWER_ON)
	    printf(":on");
	  printf("\n          ");
 	}
    }

  /* Display statistics */
  if(info->has_stats)
    {
      info->stats.qual.updated = 0x0;	/* Not that reliable, disable */
      printf("Link ");
      print_stats(stdout, &info->stats.qual, &info->range, info->has_range);

      printf("          Rx invalid nwid:%d  invalid crypt:%d  invalid misc:%d\n",
	     info->stats.discard.nwid,
	     info->stats.discard.code,
	     info->stats.discard.misc);
    }

  printf("\n");
}

/*------------------------------------------------------------------*/
/*
 * Print on the screen in a neat fashion all the info we have collected
 * on a device.
 */
static void
print_info(int		skfd,
	   char *	ifname)
{
  struct wireless_info	info;

  if(get_info(skfd, ifname, &info) < 0)
    {
      fprintf(stderr, "%-8.8s  no wireless extensions.\n\n",
	      ifname);
      return;
    }

  /* Display it ! */
  display_info(&info, ifname);
}

/*------------------------------------------------------------------*/
/*
 * Get info on all devices and print it on the screen
 */
static void
print_devices(int	skfd)
{
  char buff[1024];
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
    print_info(skfd, ifr->ifr_name);
}

/************************* SETTING ROUTINES **************************/

/*------------------------------------------------------------------*/
/*
 * Set the wireless options requested on command line
 * This function is too long and probably should be split,
 * because it look like the perfect definition of spaghetti code,
 * but I'm way to lazy
 */
static int
set_info(int		skfd,		/* The socket */
	 char *		args[],		/* Command line args */
	 int		count,		/* Args count */
	 char *		ifname)		/* Dev name */
{
  struct iwreq		wrq;
  int			i;

  /* Set dev name */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

  /* if nothing after the device name */
  if(count<1)
    iw_usage();

  /* The other args on the line specify options to be set... */
  for(i = 0; i < count; i++)
    {
      /* ---------- Set network ID ---------- */
      if((!strcasecmp(args[i], "nwid")) ||
	 (!strcasecmp(args[i], "domain")))
	{
	  i++;
	  if(i >= count)
	    iw_usage();
	  if((!strcasecmp(args[i], "off")) ||
	     (!strcasecmp(args[i], "any")))
	    wrq.u.nwid.disabled = 1;
	  else
	    if(!strcasecmp(args[i], "on"))
	      {
		/* Get old nwid */
		if(ioctl(skfd, SIOCGIWNWID, &wrq) < 0)
		  {
		    fprintf(stderr, "SIOCGIWNWID: %s\n", strerror(errno));
		    return(-1);
		  }
		strcpy(wrq.ifr_name, ifname);
		wrq.u.nwid.disabled = 0;
	      }
	    else
	      if(sscanf(args[i], "%lX", (unsigned long *) &(wrq.u.nwid.value))
		 != 1)
		iw_usage();
	      else
		wrq.u.nwid.disabled = 0;
	  wrq.u.nwid.fixed = 1;

	  if(ioctl(skfd, SIOCSIWNWID, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWNWID: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set frequency / channel ---------- */
      if((!strncmp(args[i], "freq", 4)) ||
	 (!strcmp(args[i], "channel")))
	{
	  double		freq;

	  if(++i >= count)
	    iw_usage();
	  if(sscanf(args[i], "%lg", &(freq)) != 1)
	    iw_usage();
	  if(index(args[i], 'G')) freq *= GIGA;
	  if(index(args[i], 'M')) freq *= MEGA;
	  if(index(args[i], 'k')) freq *= KILO;

	  float2freq(freq, &(wrq.u.freq));

	  if(ioctl(skfd, SIOCSIWFREQ, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWFREQ: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set sensitivity ---------- */
      if(!strncmp(args[i], "sens", 4))
	{
	  if(++i >= count)
	    iw_usage();
	  if(sscanf(args[i], "%d", &(wrq.u.sens.value)) != 1)
	    iw_usage();

	  if(ioctl(skfd, SIOCSIWSENS, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWSENS: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set encryption stuff ---------- */
      if((!strncmp(args[i], "enc", 3)) ||
	 (!strcmp(args[i], "key")))
	{
	  unsigned char	key[IW_ENCODING_TOKEN_MAX];

	  if(++i >= count)
	    iw_usage();

	  if(!strcasecmp(args[i], "on"))
	    {
	      /* Get old encryption information */
	      wrq.u.data.pointer = (caddr_t) key;
	      wrq.u.data.length = 0;
	      wrq.u.data.flags = 0;
	      if(ioctl(skfd, SIOCGIWENCODE, &wrq) < 0)
		{
		  fprintf(stderr, "SIOCGIWENCODE: %s\n", strerror(errno));
		  return(-1);
		}
	      strcpy(wrq.ifr_name, ifname);
	      wrq.u.data.flags &= ~IW_ENCODE_DISABLED;	/* Enable */
	    }
	  else
	    {
	      char *	buff;
	      char *	p;
	      int		temp;
	      int		k = 0;
	      int		gotone = 1;

	      wrq.u.data.pointer = (caddr_t) NULL;
	      wrq.u.data.flags = 0;
	      wrq.u.data.length = 0;

	      /* -- Check for the key -- */
	      if(!strncmp(args[i], "s:", 2))
		{
		  /* First case : as an ASCII string */
		  wrq.u.data.length = strlen(args[i] + 2);
		  if(wrq.u.data.length > IW_ENCODING_TOKEN_MAX)
		    wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
		  strncpy(key, args[i] + 2, wrq.u.data.length);
		  wrq.u.data.pointer = (caddr_t) key;
		  ++i;
		  gotone = 1;
		}
	      else
		{
		  /* Second case : has hexadecimal digits */
		  p = buff = malloc(strlen(args[i]) + 1);
		  strcpy(buff, args[i]);

		  p = strtok(buff, "-:;.,");
		  while(p != (char *) NULL)
		    {
		      if(sscanf(p, "%2X", &temp) != 1)
			{
			  gotone = 0;
			  break;
			}
		      key[k++] = (unsigned char) (temp & 0xFF);
		      if(strlen(p) > 2)	/* Token not finished yet */
			p += 2;
		      else
			p = strtok((char *) NULL, "-:;.,");
		    }
		  free(buff);

		  if(gotone)
		    {
		      ++i;
		      wrq.u.data.length = k;
		      wrq.u.data.pointer = (caddr_t) key;
		    }
		}

	      /* -- Check for token index -- */
	      if((i < count) &&
		 (sscanf(args[i], "[%d]", &temp) == 1) &&
		 (temp > 0) && (temp < IW_ENCODE_INDEX))
		{
		  wrq.u.encoding.flags |= temp;
		  ++i;
		  gotone = 1;
		}

	      /* -- Check the various flags -- */
	      if(i < count)
		{
		  if(!strcasecmp(args[i], "off"))
		    wrq.u.data.flags |= IW_ENCODE_DISABLED;
		  if(!strcasecmp(args[i], "open"))
		    wrq.u.data.flags |= IW_ENCODE_OPEN;
		  if(!strncasecmp(args[i], "restricted", 5))
		    wrq.u.data.flags |= IW_ENCODE_RESTRICTED;
		  if(wrq.u.data.flags & IW_ENCODE_FLAGS)
		    {
		      ++i;
		      gotone = 1;
		    }
		}

	      if(!gotone)
		iw_usage();
	      --i;
	    }

	  if(ioctl(skfd, SIOCSIWENCODE, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWENCODE(%d): %s\n",
		      errno, strerror(errno));
	      return(-1);
	    }
	  continue;
  	}

      /* ---------- Set ESSID ---------- */
      if(!strcasecmp(args[i], "essid"))
	{
	  char		essid[IW_ESSID_MAX_SIZE + 1];

	  i++;
	  if(i >= count)
	    iw_usage();
	  if((!strcasecmp(args[i], "off")) ||
	     (!strcasecmp(args[i], "any")))
	    {
	      wrq.u.essid.flags = 0;
	      essid[0] = '\0';
	    }
	  else
	    if(!strcasecmp(args[i], "on"))
	      {
		/* Get old essid */
		wrq.u.essid.pointer = (caddr_t) essid;
		wrq.u.essid.length = 0;
		wrq.u.essid.flags = 0;
		if(ioctl(skfd, SIOCGIWESSID, &wrq) < 0)
		  {
		    fprintf(stderr, "SIOCGIWESSID: %s\n", strerror(errno));
		    return(-1);
		  }
		strcpy(wrq.ifr_name, ifname);
		wrq.u.essid.flags = 1;
	      }
	    else
	      if(strlen(args[i]) > IW_ESSID_MAX_SIZE)
		{
		  fprintf(stderr, "ESSID too long (max %d): ``%s''\n",
			  IW_ESSID_MAX_SIZE, args[i]);
		  iw_usage();
		}
	      else
		{
		  int		temp;

		  wrq.u.essid.flags = 1;
		  strcpy(essid, args[i]);

		  /* Check for ESSID index */
		  if(((i+1) < count) &&
		     (sscanf(args[i+1], "[%d]", &temp) == 1) &&
		     (temp > 0) && (temp < IW_ENCODE_INDEX))
		    {
		      wrq.u.essid.flags = temp;
		      ++i;
		    }
		}

	  wrq.u.essid.pointer = (caddr_t) essid;
	  wrq.u.essid.length = strlen(essid) + 1;
	  if(ioctl(skfd, SIOCSIWESSID, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWESSID: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set AP address ---------- */
      if(!strcasecmp(args[i], "ap"))
	{
	  if(++i >= count)
	    iw_usage();

	  /* Check if we have valid address types */
	  if(check_addr_type(skfd, ifname) < 0)
	    {
	      fprintf(stderr, "%-8.8s  Interface doesn't support MAC & IP addresses\n", ifname);
	      return(-1);
	    }

	  /* Get the address */
	  if(in_addr(skfd, ifname, args[i++], &(wrq.u.ap_addr)) < 0)
	    iw_usage();

	  if(ioctl(skfd, SIOCSIWAP, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWAP: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set NickName ---------- */
      if(!strncmp(args[i], "nick", 4))
	{
	  i++;
	  if(i >= count)
	    iw_usage();
	  if(strlen(args[i]) > IW_ESSID_MAX_SIZE)
	    {
	      fprintf(stderr, "Name too long (max %d) : ``%s''\n",
		      IW_ESSID_MAX_SIZE, args[i]);
	      iw_usage();
	    }

	  wrq.u.essid.pointer = (caddr_t) args[i];
	  wrq.u.essid.length = strlen(args[i]) + 1;
	  if(ioctl(skfd, SIOCSIWNICKN, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWNICKN: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set Bit-Rate ---------- */
      if((!strncmp(args[i], "bit", 3)) ||
	 (!strcmp(args[i], "rate")))
	{
	  if(++i >= count)
	    iw_usage();
	  if(!strcasecmp(args[i], "auto"))
	    {
	      wrq.u.bitrate.value = -1;
	      wrq.u.bitrate.fixed = 0;
	    }
	  else
	    {
	      if(!strcasecmp(args[i], "fixed"))
		{
		  /* Get old bitrate */
		  if(ioctl(skfd, SIOCGIWRATE, &wrq) < 0)
		    {
		      fprintf(stderr, "SIOCGIWRATE: %s\n", strerror(errno));
		      return(-1);
		    }
		  strcpy(wrq.ifr_name, ifname);
		  wrq.u.bitrate.fixed = 1;
		}
	      else			/* Should be a numeric value */
		{
		  double		brate;

		  if(sscanf(args[i], "%lg", &(brate)) != 1)
		    iw_usage();
		  if(index(args[i], 'G')) brate *= GIGA;
		  if(index(args[i], 'M')) brate *= MEGA;
		  if(index(args[i], 'k')) brate *= KILO;
		  wrq.u.bitrate.value = (long) brate;
		  wrq.u.bitrate.fixed = 1;

		  /* Check for an additional argument */
		  if(((i+1) < count) &&
		     (!strcasecmp(args[i+1], "auto")))
		    {
		      wrq.u.bitrate.fixed = 0;
		      ++i;
		    }
		  if(((i+1) < count) &&
		     (!strcasecmp(args[i+1], "fixed")))
		    {
		      wrq.u.bitrate.fixed = 1;
		      ++i;
		    }
		}
	    }

	  if(ioctl(skfd, SIOCSIWRATE, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWRATE: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set RTS threshold ---------- */
      if(!strncasecmp(args[i], "rts", 3))
	{
	  i++;
	  if(i >= count)
	    iw_usage();
	  wrq.u.rts.value = -1;
	  wrq.u.rts.fixed = 1;
	  wrq.u.rts.disabled = 0;
	  if(!strcasecmp(args[i], "off"))
	    wrq.u.rts.disabled = 1;	/* i.e. max size */
	  else
	    if(!strcasecmp(args[i], "auto"))
	      wrq.u.rts.fixed = 0;
	    else
	      {
		if(!strcasecmp(args[i], "fixed"))
		  {
		    /* Get old RTS threshold */
		    if(ioctl(skfd, SIOCGIWRTS, &wrq) < 0)
		      {
			fprintf(stderr, "SIOCGIWRTS: %s\n", strerror(errno));
			return(-1);
		      }
		    strcpy(wrq.ifr_name, ifname);
		    wrq.u.rts.fixed = 1;
		  }
		else			/* Should be a numeric value */
		  if(sscanf(args[i], "%ld", (unsigned long *) &(wrq.u.rts.value))
		     != 1)
		    iw_usage();
	    }

	  if(ioctl(skfd, SIOCSIWRTS, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWRTS: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set fragmentation threshold ---------- */
      if(!strncmp(args[i], "frag", 4))
	{
	  i++;
	  if(i >= count)
	    iw_usage();
	  wrq.u.frag.value = -1;
	  wrq.u.frag.fixed = 1;
	  wrq.u.frag.disabled = 0;
	  if(!strcasecmp(args[i], "off"))
	    wrq.u.frag.disabled = 1;	/* i.e. max size */
	  else
	    if(!strcasecmp(args[i], "auto"))
	      wrq.u.frag.fixed = 0;
	    else
	      {
		if(!strcasecmp(args[i], "fixed"))
		  {
		    /* Get old fragmentation threshold */
		    if(ioctl(skfd, SIOCGIWFRAG, &wrq) < 0)
		      {
			fprintf(stderr, "SIOCGIWFRAG: %s\n", strerror(errno));
			return(-1);
		      }
		    strcpy(wrq.ifr_name, ifname);
		    wrq.u.frag.fixed = 1;
		  }
		else			/* Should be a numeric value */
		  if(sscanf(args[i], "%ld", (unsigned long *) &(wrq.u.frag.value))
		     != 1)
		    iw_usage();
	    }

	  if(ioctl(skfd, SIOCSIWFRAG, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWFRAG: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set operation mode ---------- */
      if(!strcmp(args[i], "mode"))
	{
	  int	k;

	  i++;
	  if(i >= count)
	    iw_usage();

	  if(sscanf(args[i], "%d", &k) != 1)
	    {
	      k = 0;
	      while(k < 6 && strncasecmp(args[i], operation_mode[k], 3))
		k++;
	    }
	  if((k > 5) || (k < 0))
	    iw_usage();

	  wrq.u.mode = k;
	  if(ioctl(skfd, SIOCSIWMODE, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWMODE: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set Power Management ---------- */
      if(!strncmp(args[i], "power", 3))
	{
	  if(++i >= count)
	    iw_usage();

	  if(!strcasecmp(args[i], "off"))
	    wrq.u.power.disabled = 1;	/* i.e. max size */
	  else
	    if(!strcasecmp(args[i], "on"))
	      {
		/* Get old Power info */
		if(ioctl(skfd, SIOCGIWPOWER, &wrq) < 0)
		  {
		    fprintf(stderr, "SIOCGIWPOWER: %s\n", strerror(errno));
		    return(-1);
		  }
		strcpy(wrq.ifr_name, ifname);
		wrq.u.power.disabled = 0;
	      }
	    else
	      {
		double		temp;
		int		gotone = 0;
		/* Default - nope */
		wrq.u.power.flags = IW_POWER_ON;
		wrq.u.power.disabled = 0;

		/* Check value modifier */
		if(!strcasecmp(args[i], "min"))
		  {
		    wrq.u.power.flags |= IW_POWER_MIN;
		    if(++i >= count)
		      iw_usage();
		  }
		else
		  if(!strcasecmp(args[i], "max"))
		    {
		      wrq.u.power.flags |= IW_POWER_MAX;
		      if(++i >= count)
			iw_usage();
		    }

		/* Check value type */
		if(!strcasecmp(args[i], "period"))
		  {
		    wrq.u.power.flags |= IW_POWER_PERIOD;
		    if(++i >= count)
		      iw_usage();
		  }
		else
		  if(!strcasecmp(args[i], "timeout"))
		    {
		      wrq.u.power.flags |= IW_POWER_TIMEOUT;
		      if(++i >= count)
			iw_usage();
		    }

		/* Is there any value to grab ? */
		if(sscanf(args[i], "%lg", &(temp)) == 1)
		  {
		    temp *= MEGA;	/* default = s */
		    if(index(args[i], 'u')) temp /= MEGA;
		    if(index(args[i], 'm')) temp /= KILO;
		    wrq.u.power.value = (long) temp;
		    if((wrq.u.power.flags & IW_POWER_TYPE) == 0)
		      wrq.u.power.flags |= IW_POWER_PERIOD;
		    ++i;
		    gotone = 1;
		  }

		/* Now, check the mode */
		if(i < count)
		  {
		    if(!strcasecmp(args[i], "all"))
		      wrq.u.power.flags |= IW_POWER_ALL_R;
		    if(!strncasecmp(args[i], "unicast", 4))
		      wrq.u.power.flags |= IW_POWER_UNICAST_R;
		    if(!strncasecmp(args[i], "multicast", 5))
		      wrq.u.power.flags |= IW_POWER_MULTICAST_R;
		    if(!strncasecmp(args[i], "force", 5))
		      wrq.u.power.flags |= IW_POWER_FORCE_S;
		    if(!strcasecmp(args[i], "repeat"))
		      wrq.u.power.flags |= IW_POWER_REPEATER;
		    if(wrq.u.power.flags & IW_POWER_MODE)
		      {
			++i;
			gotone = 1;
		      }
		  }
		if(!gotone)
		  iw_usage();
		--i;
	      }

	  if(ioctl(skfd, SIOCSIWPOWER, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWPOWER(%d): %s\n",
		      errno, strerror(errno));
	      return(-1);
	    }
	  continue;
  	}

#if WIRELESS_EXT > 9
      /* ---------- Set Transmit-Power ---------- */
      if(!strncmp(args[i], "txpower", 3))
	{
	  struct iw_range	range;

	  if(++i >= count)
	    iw_usage();

	  /* Extract range info */
	  if(get_range_info(skfd, ifname, &range) < 0)
	    memset(&range, 0, sizeof(range));

	  /* Prepare the request */
	  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	  wrq.u.txpower.value = -1;
	  wrq.u.txpower.fixed = 1;
	  wrq.u.txpower.disabled = 0;
	  wrq.u.data.flags = IW_TXPOW_DBM;
	  if(!strcasecmp(args[i], "off"))
	    wrq.u.txpower.disabled = 1;	/* i.e. turn radio off */
	  else
	    if(!strcasecmp(args[i], "auto"))
	      wrq.u.txpower.fixed = 0;	/* i.e. use power control */
	    else
	      {
		if(!strcasecmp(args[i], "fixed"))
		  {
		    /* Get old tx-power */
		    if(ioctl(skfd, SIOCGIWTXPOW, &wrq) < 0)
		      {
			fprintf(stderr, "SIOCGIWTXPOW: %s\n", strerror(errno));
			return(-1);
		      }
		    strcpy(wrq.ifr_name, ifname);
		    wrq.u.txpower.fixed = 1;
		  }
		else			/* Should be a numeric value */
		  {
		    int		power;
		    int		ismwatt = 0;

		    /* Get the value */
		    if(sscanf(args[i], "%ld",
			      (unsigned long *) &(power)) != 1)
		      iw_usage();

		    /* Check if milliwatt */
		    ismwatt = (index(args[i], 'm') != NULL);

		    /* Convert */
		    if(!ismwatt && (range.txpower_capa & IW_TXPOW_MWATT))
		      {
			power = dbm2mwatt(power);
			wrq.u.data.flags = IW_TXPOW_MWATT;
		      }
		    if(ismwatt && !(range.txpower_capa & IW_TXPOW_MWATT))
		      power = mwatt2dbm(power);
		    wrq.u.bitrate.value = power;

		    /* Check for an additional argument */
		    if(((i+1) < count) &&
		       (!strcasecmp(args[i+1], "auto")))
		      {
			wrq.u.txpower.fixed = 0;
			++i;
		      }
		    if(((i+1) < count) &&
		       (!strcasecmp(args[i+1], "fixed")))
		      {
			wrq.u.txpower.fixed = 1;
			++i;
		      }
		  }
	      }

	  if(ioctl(skfd, SIOCSIWTXPOW, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWTXPOW: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}
#endif

#if WIRELESS_EXT > 10
      /* ---------- Set Power Management ---------- */
      if(!strncmp(args[i], "retry", 3))
	{
	  double		temp;
	  int		gotone = 0;

	  if(++i >= count)
	    iw_usage();

	  /* Default - nope */
	  wrq.u.retry.flags = IW_RETRY_LIMIT;
	  wrq.u.retry.disabled = 0;

	  /* Check value modifier */
	  if(!strcasecmp(args[i], "min"))
	    {
	      wrq.u.retry.flags |= IW_RETRY_MIN;
	      if(++i >= count)
		iw_usage();
	    }
	  else
	    if(!strcasecmp(args[i], "max"))
	      {
		wrq.u.retry.flags |= IW_RETRY_MAX;
		if(++i >= count)
		  iw_usage();
	      }

	  /* Check value type */
	  if(!strcasecmp(args[i], "limit"))
	    {
	      wrq.u.retry.flags |= IW_RETRY_LIMIT;
	      if(++i >= count)
		iw_usage();
	    }
	  else
	    if(!strncasecmp(args[i], "lifetime", 4))
	      {
		wrq.u.retry.flags |= IW_RETRY_LIFETIME;
		if(++i >= count)
		  iw_usage();
	      }

	  /* Is there any value to grab ? */
	  if(sscanf(args[i], "%lg", &(temp)) == 1)
	    {
	      /* Limit is absolute, on the other hand lifetime is seconds */
	      if(!(wrq.u.retry.flags & IW_RETRY_LIMIT))
		{
		  /* Normalise lifetime */
		  temp *= MEGA;	/* default = s */
		  if(index(args[i], 'u')) temp /= MEGA;
		  if(index(args[i], 'm')) temp /= KILO;
		}
	      wrq.u.retry.value = (long) temp;
	      ++i;
	      gotone = 1;
	    }

	  if(!gotone)
	    iw_usage();
	  --i;

	  if(ioctl(skfd, SIOCSIWRETRY, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWRETRY(%d): %s\n",
		      errno, strerror(errno));
	      return(-1);
	    }
	  continue;
	}

#endif	/* WIRELESS_EXT > 10 */

      /* ---------- Other ---------- */
      /* Here we have an unrecognised arg... */
      fprintf(stderr, "Invalid argument : %s\n", args[i]);
      iw_usage();
      return(-1);
    }		/* for(index ... */
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
      print_devices(skfd);
      close(skfd);
      exit(0);
    }

  /* Special case for help... */
  if((!strncmp(argv[1], "-h", 9)) ||
     (!strcmp(argv[1], "--help")))
    {
      iw_usage();
      close(skfd);
      exit(0);
    }

  /* The device name must be the first argument */
  if(argc == 2)
    {
      print_info(skfd, argv[1]);
      close(skfd);
      exit(0);
    }

  /* The other args on the line specify options to be set... */
  goterr = set_info(skfd, argv + 2, argc - 2, argv[1]);

  /* Close the socket. */
  close(skfd);

  return(goterr);
}
