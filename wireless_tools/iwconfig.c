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
  FILE *f=fopen("/proc/net/wireless","r");
  char buf[256];
  char *bp;
  if(f==NULL)
  	return -1;
  while(fgets(buf,255,f))
  {
  	bp=buf;
  	while(*bp&&isspace(*bp))
  		bp++;
  	if(strncmp(bp,ifname,strlen(ifname))==0 && bp[strlen(ifname)]==':')
  	{
 		bp=strchr(bp,':');
 		bp++;
		bp = strtok(bp, " .");
 		sscanf(bp, "%X", (unsigned int *)&stats->status);
		bp = strtok(NULL, " .");
 		sscanf(bp, "%d", (unsigned int *)&stats->qual.qual);
		bp = strtok(NULL, " .");
		sscanf(bp, "%d", (unsigned int *)&stats->qual.level);
		bp = strtok(NULL, " .");
		sscanf(bp, "%d", (unsigned int *)&stats->qual.noise);
		bp = strtok(NULL, " .");
		sscanf(bp, "%d", &stats->discard.nwid);
		bp = strtok(NULL, " .");
		sscanf(bp, "%d", &stats->discard.code);
		bp = strtok(NULL, " .");
		sscanf(bp, "%d", &stats->discard.misc);
 		fclose(f);
 		return 0;
  	}
  }
  fclose(f);
  return 0;
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

  /* Get network ID */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWNWID, &wrq) >= 0)
    {
      info->has_nwid = 1;
      info->nwid_on = wrq.u.nwid.on;
      info->nwid = wrq.u.nwid.nwid;
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
      info->sens = wrq.u.sensitivity;
    }

   /* Get encryption information */
   strcpy(wrq.ifr_name, ifname);
   if(ioctl(skfd, SIOCGIWENCODE, &wrq) >= 0)
     {
       info->has_enc = 1;
       info->enc_method = wrq.u.encoding.method;
       info->enc_key = wrq.u.encoding.code;
     }

#if WIRELESS_EXT > 5
  /* Get ESSID */
  strcpy(wrq.ifr_name, ifname);
  wrq.u.data.pointer = (caddr_t) info->essid;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
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
#endif	/* WIRELESS_EXT > 5 */

#if WIRELESS_EXT > 7
  /* Get NickName */
  strcpy(wrq.ifr_name, ifname);
  wrq.u.data.pointer = (caddr_t) info->nickname;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCGIWNICKN, &wrq) >= 0)
    if(wrq.u.data.length > 1)
      info->has_nickname = 1;

  /* Get bit rate */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWRATE, &wrq) >= 0)
    {
      info->has_bitrate = 1;
      info->bitrate_fixed = wrq.u.bitrate.fixed;
      info->bitrate = wrq.u.bitrate.value;
    }

  /* Get RTS threshold */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWRTS, &wrq) >= 0)
    {
      info->has_rts = 1;
      info->rts_fixed = wrq.u.rts.fixed;
      info->rts = wrq.u.rts.value;
    }

  /* Get fragmentation thershold */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWFRAG, &wrq) >= 0)
    {
      info->has_frag = 1;
      info->frag_fixed = wrq.u.frag.fixed;
      info->frag = wrq.u.frag.value;
    }
#endif	/* WIRELESS_EXT > 7 */

  /* Get stats */
  if(iw_getstats(ifname, &(info->stats)) == 0)
    {
      info->has_stats = 1;
    }

  /* Get ranges */
  if(get_range_info(skfd, ifname, &(info->range)) >= 0)
    info->has_range = 1;

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
  /* Display device name and wireless name (name of the protocol used) */
  printf("%-8.8s  %s  ", ifname, info->name);

  /* Display ESSID (extended network), if any */
  if(info->has_essid)
    {
      if(info->essid_on)
	printf("ESSID:\"%s\"  ", info->essid);
      else
	printf("ESSID:off  ");
    }

  /* Display NickName (station name), if any */
  if(info->has_nickname)
    printf("Nickname:\"%s\"", info->nickname);

  /* Formatting */
  if(info->has_essid || info->has_nickname)
    printf("\n          ");

  /* Display Network ID */
  if(info->has_nwid)
    {
      /* Note : should display right number of digit according to info
       * in range structure */
      if(info->nwid_on)
	printf("NWID:%lX  ", info->nwid);
      else
	printf("NWID:off/any  ");
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
    }

  /* Display sensitivity */
  if(info->has_sens)
    {
      if(info->has_range)
	/* Display in dBm ? */
	if(info->sens < 0)
	  printf("Sensitivity:%d dBm  ", info->sens);
	else
	  printf("Sensitivity:%d/%d  ", info->sens, info->range.sensitivity);
      else
	printf("Sensitivity:%d  ", info->sens);
    }

  /* Display the address of the current Access Point */
  if(info->has_ap_addr)
    {
      /* A bit of clever formatting */
      if((info->has_nwid + 2*info->has_freq + 2*info->has_sens
	  + !info->has_essid) > 3)
	printf("\n          ");

      printf("Access Point: %s", pr_ether(info->ap_addr.sa_data));
    }

  printf("\n          ");

  /* Display the currently used/set bit-rate */
  if(info->has_bitrate)
    {
      printf("Bit Rate:");
      if(info->bitrate >= GIGA)
	printf("%g Gb/s", info->bitrate / GIGA);
      else
	if(info->bitrate >= MEGA)
	  printf("%g Mb/s", info->bitrate / MEGA);
	else
	  printf("%g kb/s", info->bitrate / KILO);

      /* Fixed ? */
      if(info->bitrate_fixed)
	printf(" (f)   ");
      else
	printf("   ");
    }

  /* Display the RTS threshold */
  if(info->has_rts)
    {
      printf("RTS thr:%ld B", info->rts);

      /* Fixed ? */
      if(info->rts_fixed)
	printf(" (f)   ");
      else
	printf("   ");
    }

  /* Display the fragmentation threshold */
  if(info->has_bitrate)
    {
      printf("Frag thr:%ld B", info->frag);

      /* Fixed ? */
      if(info->frag_fixed)
	printf(" (f)   ");
      else
	printf("   ");
    }

  /* Formating */
  if((info->has_bitrate) || (info->has_rts) || (info->has_bitrate))
    printf("\n          ");

  if(info->has_enc)
    {
      printf("Encryption key:");
      if(info->enc_method)
	{
	  int		i = 0;
	  u_short	parts[4];
	  long long	key = info->enc_key;

	  for(i = 3; i >= 0; i--)
	    {
	      parts[i] = key & 0xFFFF;
	      key >>= 16;
	    }

	  i = 0;
	  while((parts[i] == 0) && (i < 3))
	    i++;
	  for(; i < 3; i++)
	    printf("%.4X-", parts[i]);
	  printf("%.4X", parts[3]);

	  if(info->enc_method > 1)
	    printf(" (%d)", info->enc_method);
	  printf("\n          ");
 	}
      else
	printf("off\n          ");
    }

  if(info->has_stats)
    {
      if(info->has_range && (info->stats.qual.level != 0))
	/* If the statistics are in dBm */
	if(info->stats.qual.level > info->range.max_qual.level)
	  printf("Link quality:%d/%d  Signal level:%d dBm  Noise level:%d dBm\n",
		 info->stats.qual.qual, info->range.max_qual.qual,
		 info->stats.qual.level - 0x100,
		 info->stats.qual.noise - 0x100);
	else
	  /* Statistics are relative values (0 -> max) */
	  printf("Link quality:%d/%d  Signal level:%d/%d  Noise level:%d/%d\n",
		 info->stats.qual.qual, info->range.max_qual.qual,
		 info->stats.qual.level, info->range.max_qual.level,
		 info->stats.qual.noise, info->range.max_qual.noise);
      else
	/* We can't read the range, so we don't know... */
	printf("Link quality:%d  Signal level:%d  Noise level:%d\n",
	       info->stats.qual.qual,
	       info->stats.qual.level,
	       info->stats.qual.noise);

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
	    wrq.u.nwid.on = 0;
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
		wrq.u.nwid.on = 1;
	      }
	    else
	      if(sscanf(args[i], "%lX", (unsigned long *) &(wrq.u.nwid.nwid))
		 != 1)
		iw_usage();
	      else
		wrq.u.nwid.on = 1;

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
	  if(sscanf(args[i], "%d", &(wrq.u.sensitivity)) != 1)
	    iw_usage();

	  if(ioctl(skfd, SIOCSIWSENS, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWSENS: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set encryption stuff ---------- */
      if(!strncmp(args[i], "enc", 3 ))
	{
	  unsigned long long	key = 0;

	  if(++i >= count)
	    iw_usage();

	  if(!strcasecmp(args[i], "off"))
	    wrq.u.encoding.method = 0;
	  else
	    {
	      if(!strcasecmp(args[i], "on"))
		{
		  /* Get old encryption information */
		  if(ioctl(skfd, SIOCGIWENCODE, &wrq) < 0)
		    {
		      fprintf(stderr, "SIOCGIWENCODE: %s\n", strerror(errno));
		      return(-1);
		    }
		  strcpy(wrq.ifr_name, ifname);
		}
	      else
		{
		  char *	buff;
		  char *	p;
		  u_long	temp;

		  p = buff = malloc(strlen(args[i] + 1));
		  strcpy(buff, args[i]);

		  p = strtok(buff, "-:;.,*#");
		  while(p != (char *) NULL)
		    {
		      key = key << 16;
		      if(sscanf(p, "%lX", &temp) != 1)
			iw_usage();
		      key += temp;
		      p = strtok((char *) NULL, "-:;.,*#");
		    }

		  free(buff);
		  wrq.u.encoding.code = key;
		}
	      /* TODO : check for "(method)" in args list */
	      wrq.u.encoding.method = 1;
	    }

	  if(ioctl(skfd, SIOCSIWENCODE, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWENCODE(%d): %s\n",
		      errno, strerror(errno));
	      return(-1);
	    }
	  continue;
  	}

#if WIRELESS_EXT > 5
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
	      wrq.u.data.flags = 0;
	      essid[0] = '\0';
	    }
	  else
	    if(!strcasecmp(args[i], "on"))
	      {
		/* Get old essid */
		wrq.u.data.pointer = (caddr_t) essid;
		wrq.u.data.length = 0;
		wrq.u.data.flags = 0;
		if(ioctl(skfd, SIOCGIWESSID, &wrq) < 0)
		  {
		    fprintf(stderr, "SIOCGIWESSID: %s\n", strerror(errno));
		    return(-1);
		  }
		strcpy(wrq.ifr_name, ifname);
		wrq.u.data.flags = 1;
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
		  wrq.u.data.flags = 1;
		  strcpy(essid, args[i]);
		}

	  wrq.u.data.pointer = (caddr_t) essid;
	  wrq.u.data.length = strlen(essid) + 1;
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
#endif	/* WIRELESS_EXT > 5 */

#if WIRELESS_EXT > 7
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

	  wrq.u.data.pointer = (caddr_t) args[i];
	  wrq.u.data.length = strlen(args[i]) + 1;
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
	  i++;
	  if(i >= count)
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
		}
	      wrq.u.bitrate.fixed = 1;
	    }

	  if(ioctl(skfd, SIOCSIWRATE, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWRATE: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* ---------- Set RTS threshold ---------- */
      if(!strncmp(args[i], "rts", 3))
	{
	  i++;
	  if(i >= count)
	    iw_usage();
	  if(!strcasecmp(args[i], "auto"))
	    {
	      wrq.u.rts.value = -1;
	      wrq.u.rts.fixed = 0;
	    }
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
		}
	      else
		if(!strcasecmp(args[i], "off"))
		  wrq.u.rts.value = -1;	/* i.e. max size */
		else			/* Should be a numeric value */
		  if(sscanf(args[i], "%ld", (unsigned long *) &(wrq.u.rts.value))
		     != 1)
		    iw_usage();

	      wrq.u.rts.fixed = 1;
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
	  if(!strcasecmp(args[i], "auto"))
	    {
	      wrq.u.frag.value = -1;
	      wrq.u.frag.fixed = 0;
	    }
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
		}
	      else
		if(!strcasecmp(args[i], "off"))
		  wrq.u.frag.value = -1;	/* i.e. max size */
		else			/* Should be a numeric value */
		  if(sscanf(args[i], "%ld", (unsigned long *) &(wrq.u.frag.value))
		     != 1)
		    iw_usage();

	      wrq.u.frag.fixed = 1;
	    }

	  if(ioctl(skfd, SIOCSIWFRAG, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWFRAG: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}
#endif	/* WIRELESS_EXT > 7 */

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


