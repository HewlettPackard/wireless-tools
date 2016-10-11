/*
 *	Wireless Tools
 *
 *		Jean II - HPLB 97->99 - HPL 99->01
 *
 * Main code for "iwconfig". This is the generic tool for most
 * manipulations...
 * You need to link this code against "iwlib.c" and "-lm".
 *
 * This file is released under the GPL license.
 *     Copyright (c) 1997-2002 Jean Tourrilhes <jt@hpl.hp.com>
 */

#include "iwlib.h"		/* Header */

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
  fprintf(stderr, "                          [mode {managed|ad-hoc|...}\n");
  fprintf(stderr, "                          [freq N.NNNN[k|M|G]]\n");
  fprintf(stderr, "                          [channel N]\n");
  fprintf(stderr, "                          [sens N]\n");
  fprintf(stderr, "                          [nick N]\n");
  fprintf(stderr, "                          [rate {N|auto|fixed}]\n");
  fprintf(stderr, "                          [rts {N|auto|fixed|off}]\n");
  fprintf(stderr, "                          [frag {N|auto|fixed|off}]\n");
  fprintf(stderr, "                          [enc {NNNN-NNNN|off}]\n");
  fprintf(stderr, "                          [power {period N|timeout N}]\n");
  fprintf(stderr, "                          [txpower N {mW|dBm}]\n");
  fprintf(stderr, "                          [commit]\n");
  fprintf(stderr, "       Check man pages for more details.\n\n");
}


/************************* DISPLAY ROUTINES **************************/

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
  if(iw_get_ext(skfd, ifname, SIOCGIWNAME, &wrq) < 0)
    {
      /* If no wireless name : no wireless extensions */
      /* But let's check if the interface exists at all */
      struct ifreq ifr;

      strcpy(ifr.ifr_name, ifname);
      if(ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
	return(-ENODEV);
      else
	return(-ENOTSUP);
    }
  else
    {
      strncpy(info->name, wrq.u.name, IFNAMSIZ);
      info->name[IFNAMSIZ] = '\0';
    }

  /* Get ranges */
  if(iw_get_range_info(skfd, ifname, &(info->range)) >= 0)
    info->has_range = 1;

  /* Get network ID */
  if(iw_get_ext(skfd, ifname, SIOCGIWNWID, &wrq) >= 0)
    {
      info->has_nwid = 1;
      memcpy(&(info->nwid), &(wrq.u.nwid), sizeof(iwparam));
    }

  /* Get frequency / channel */
  if(iw_get_ext(skfd, ifname, SIOCGIWFREQ, &wrq) >= 0)
    {
      info->has_freq = 1;
      info->freq = iw_freq2float(&(wrq.u.freq));
    }

  /* Get sensitivity */
  if(iw_get_ext(skfd, ifname, SIOCGIWSENS, &wrq) >= 0)
    {
      info->has_sens = 1;
      memcpy(&(info->sens), &(wrq.u.sens), sizeof(iwparam));
    }

  /* Get encryption information */
  wrq.u.data.pointer = (caddr_t) info->key;
  wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
  wrq.u.data.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWENCODE, &wrq) >= 0)
    {
      info->has_key = 1;
      info->key_size = wrq.u.data.length;
      info->key_flags = wrq.u.data.flags;
    }

  /* Get ESSID */
  wrq.u.essid.pointer = (caddr_t) info->essid;
  wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
  wrq.u.essid.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWESSID, &wrq) >= 0)
    {
      info->has_essid = 1;
      info->essid_on = wrq.u.data.flags;
    }

  /* Get AP address */
  if(iw_get_ext(skfd, ifname, SIOCGIWAP, &wrq) >= 0)
    {
      info->has_ap_addr = 1;
      memcpy(&(info->ap_addr), &(wrq.u.ap_addr), sizeof (sockaddr));
    }

  /* Get NickName */
  wrq.u.essid.pointer = (caddr_t) info->nickname;
  wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
  wrq.u.essid.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWNICKN, &wrq) >= 0)
    if(wrq.u.data.length > 1)
      info->has_nickname = 1;

  /* Get bit rate */
  if(iw_get_ext(skfd, ifname, SIOCGIWRATE, &wrq) >= 0)
    {
      info->has_bitrate = 1;
      memcpy(&(info->bitrate), &(wrq.u.bitrate), sizeof(iwparam));
    }

  /* Get RTS threshold */
  if(iw_get_ext(skfd, ifname, SIOCGIWRTS, &wrq) >= 0)
    {
      info->has_rts = 1;
      memcpy(&(info->rts), &(wrq.u.rts), sizeof(iwparam));
    }

  /* Get fragmentation threshold */
  if(iw_get_ext(skfd, ifname, SIOCGIWFRAG, &wrq) >= 0)
    {
      info->has_frag = 1;
      memcpy(&(info->frag), &(wrq.u.frag), sizeof(iwparam));
    }

  /* Get operation mode */
  if(iw_get_ext(skfd, ifname, SIOCGIWMODE, &wrq) >= 0)
    {
      info->mode = wrq.u.mode;
      if((info->mode < IW_NUM_OPER_MODE) && (info->mode >= 0))
	info->has_mode = 1;
    }

  /* Get Power Management settings */
  wrq.u.power.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWPOWER, &wrq) >= 0)
    {
      info->has_power = 1;
      memcpy(&(info->power), &(wrq.u.power), sizeof(iwparam));
    }

#if WIRELESS_EXT > 9
  /* Get Transmit Power */
  if(iw_get_ext(skfd, ifname, SIOCGIWTXPOW, &wrq) >= 0)
    {
      info->has_txpower = 1;
      memcpy(&(info->txpower), &(wrq.u.txpower), sizeof(iwparam));
    }
#endif

#if WIRELESS_EXT > 10
  /* Get retry limit/lifetime */
  if(iw_get_ext(skfd, ifname, SIOCGIWRETRY, &wrq) >= 0)
    {
      info->has_retry = 1;
      memcpy(&(info->retry), &(wrq.u.retry), sizeof(iwparam));
    }
#endif	/* WIRELESS_EXT > 10 */

  /* Get stats */
  if(iw_get_stats(skfd, ifname, &(info->stats)) >= 0)
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
  char		buffer[128];	/* Temporary buffer */

  /* One token is more of less 5 characters, 14 tokens per line */
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
	printf("ESSID:off/any  ");
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
      /* Note : should display proper number of digit according to info
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
      printf("Mode:%s  ", iw_operation_mode[info->mode]);
      tokens +=3;
    }

  /* Display frequency / channel */
  if(info->has_freq)
    {
      iw_print_freq(buffer, info->freq);
      printf("%s  ", buffer);
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
      printf(" %s  ", iw_pr_ether(buffer, info->ap_addr.sa_data));
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

      /* Display it */
      iw_print_bitrate(buffer, info->bitrate.value);
      printf("Bit Rate%c%s   ", (info->bitrate.fixed ? '=' : ':'), buffer);
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
	    dbm = iw_mwatt2dbm(info->txpower.value);
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
	    {
	      iw_print_retry_value(buffer,
				   info->retry.value, info->retry.flags);
	      printf("%s", buffer);
	    }

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
	  iw_print_key(buffer, info->key, info->key_size, info->key_flags);
	  printf("%s", buffer);

	  /* Other info... */
	  if((info->key_flags & IW_ENCODE_INDEX) > 1)
	    printf(" [%d]", info->key_flags & IW_ENCODE_INDEX);
	  if(info->key_flags & IW_ENCODE_RESTRICTED)
	    printf("   Security mode:restricted");
	  if(info->key_flags & IW_ENCODE_OPEN)
	    printf("   Security mode:open");
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
	    {
	      iw_print_pm_value(buffer, info->power.value, info->power.flags);
	      printf("%s  ", buffer);
	    }

	  /* Let's check the mode */
	  iw_print_pm_mode(buffer, info->power.flags);
	  printf("%s", buffer);

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
      iw_print_stats(buffer, &info->stats.qual, &info->range, info->has_range);
      printf("Link %s\n", buffer);

#if WIRELESS_EXT > 11
      printf("          Rx invalid nwid:%d  Rx invalid crypt:%d  Rx invalid frag:%d\n          Tx excessive retries:%d  Invalid misc:%d   Missed beacon:%d\n",
	     info->stats.discard.nwid,
	     info->stats.discard.code,
	     info->stats.discard.fragment,
	     info->stats.discard.retries,
	     info->stats.discard.misc,
	     info->stats.miss.beacon);
#else /* WIRELESS_EXT > 11 */
      printf("          Rx invalid nwid:%d  invalid crypt:%d  invalid misc:%d\n",
	     info->stats.discard.nwid,
	     info->stats.discard.code,
	     info->stats.discard.misc);
#endif /* WIRELESS_EXT > 11 */
    }

  printf("\n");
}

/*------------------------------------------------------------------*/
/*
 * Print on the screen in a neat fashion all the info we have collected
 * on a device.
 */
static int
print_info(int		skfd,
	   char *	ifname,
	   char *	args[],
	   int		count)
{
  struct wireless_info	info;
  int			rc;

  /* Avoid "Unused parameter" warning */
  args = args; count = count;

  rc = get_info(skfd, ifname, &info);
  switch(rc)
    {
    case 0:	/* Success */
      /* Display it ! */
      display_info(&info, ifname);
      break;

    case -ENOTSUP:
      fprintf(stderr, "%-8.8s  no wireless extensions.\n\n",
	      ifname);
      break;

    default:
      fprintf(stderr, "%-8.8s  %s\n\n", ifname, strerror(-rc));
    }
  return(rc);
}

/************************* SETTING ROUTINES **************************/

/*------------------------------------------------------------------*/
/*
 * Macro to handle errors when setting WE
 * Print a nice error message and exit...
 * We define them as macro so that "return" do the right thing.
 * The "do {...} while(0)" is a standard trick
 */
#define ERR_SET_EXT(rname, request) \
	fprintf(stderr, "Error for wireless request \"%s\" (%X) :\n", \
		rname, request)

#define ABORT_ARG_NUM(rname, request) \
	do { \
		ERR_SET_EXT(rname, request); \
		fprintf(stderr, "    too few arguments.\n"); \
		return(-1); \
	} while(0)

#define ABORT_ARG_TYPE(rname, request, arg) \
	do { \
		ERR_SET_EXT(rname, request); \
		fprintf(stderr, "    invalid argument \"%s\".\n", arg); \
		return(-2); \
	} while(0)

#define ABORT_ARG_SIZE(rname, request, max) \
	do { \
		ERR_SET_EXT(rname, request); \
		fprintf(stderr, "    argument too big (max %d)\n", max); \
		return(-3); \
	} while(0)

/*------------------------------------------------------------------*/
/*
 * Wrapper to push some Wireless Parameter in the driver
 * Use standard wrapper and add pretty error message if fail...
 */
#define IW_SET_EXT_ERR(skfd, ifname, request, wrq, rname) \
	do { \
	if(iw_set_ext(skfd, ifname, request, wrq) < 0) { \
		ERR_SET_EXT(rname, request); \
		fprintf(stderr, "    SET failed on device %-1.16s ; %s.\n", \
			ifname, strerror(errno)); \
		return(-5); \
	} } while(0)

/*------------------------------------------------------------------*/
/*
 * Wrapper to extract some Wireless Parameter out of the driver
 * Use standard wrapper and add pretty error message if fail...
 */
#define IW_GET_EXT_ERR(skfd, ifname, request, wrq, rname) \
	do { \
	if(iw_get_ext(skfd, ifname, request, wrq) < 0) { \
		ERR_SET_EXT(rname, request); \
		fprintf(stderr, "    GET failed on device %-1.16s ; %s.\n", \
			ifname, strerror(errno)); \
		return(-6); \
	} } while(0)

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

  /* if nothing after the device name - will never happen */
  if(count < 1)
    {
      fprintf(stderr, "Error : too few arguments.\n");
      return(-1);
    }

  /* The other args on the line specify options to be set... */
  for(i = 0; i < count; i++)
    {
      /* ---------- Commit changes to driver ---------- */
      if(!strncmp(args[i], "commit", 6))
	{
	  /* No args */
	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWCOMMIT, &wrq,
			 "Commit changes");
	  continue;
	}

      /* ---------- Set network ID ---------- */
      if((!strcasecmp(args[i], "nwid")) ||
	 (!strcasecmp(args[i], "domain")))
	{
	  i++;
	  if(i >= count)
	    ABORT_ARG_NUM("Set NWID", SIOCSIWNWID);
	  if((!strcasecmp(args[i], "off")) ||
	     (!strcasecmp(args[i], "any")))
	    wrq.u.nwid.disabled = 1;
	  else
	    if(!strcasecmp(args[i], "on"))
	      {
		/* Get old nwid */
		IW_GET_EXT_ERR(skfd, ifname, SIOCGIWNWID, &wrq,
			       "Set NWID");
		wrq.u.nwid.disabled = 0;
	      }
	    else
	      if(sscanf(args[i], "%lX", (unsigned long *) &(wrq.u.nwid.value))
		 != 1)
		ABORT_ARG_TYPE("Set NWID", SIOCSIWNWID, args[i]);
	      else
		wrq.u.nwid.disabled = 0;
	  wrq.u.nwid.fixed = 1;

	  /* Set new nwid */
	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWNWID, &wrq,
			 "Set NWID");
	  continue;
	}

      /* ---------- Set frequency / channel ---------- */
      if((!strncmp(args[i], "freq", 4)) ||
	 (!strcmp(args[i], "channel")))
	{
	  double		freq;

	  if(++i >= count)
	    ABORT_ARG_NUM("Set Frequency", SIOCSIWFREQ);
	  if(sscanf(args[i], "%lg", &(freq)) != 1)
	    ABORT_ARG_TYPE("Set Frequency", SIOCSIWFREQ, args[i]);
	  if(index(args[i], 'G')) freq *= GIGA;
	  if(index(args[i], 'M')) freq *= MEGA;
	  if(index(args[i], 'k')) freq *= KILO;

	  iw_float2freq(freq, &(wrq.u.freq));

	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWFREQ, &wrq,
			 "Set Frequency");
	  continue;
	}

      /* ---------- Set sensitivity ---------- */
      if(!strncmp(args[i], "sens", 4))
	{
	  if(++i >= count)
	    ABORT_ARG_NUM("Set Sensitivity", SIOCSIWSENS);
	  if(sscanf(args[i], "%i", &(wrq.u.sens.value)) != 1)
	    ABORT_ARG_TYPE("Set Sensitivity", SIOCSIWSENS, args[i]);

	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWSENS, &wrq,
			 "Set Sensitivity");
	  continue;
	}

      /* ---------- Set encryption stuff ---------- */
      if((!strncmp(args[i], "enc", 3)) ||
	 (!strcmp(args[i], "key")))
	{
	  unsigned char	key[IW_ENCODING_TOKEN_MAX];

	  if(++i >= count)
	    ABORT_ARG_NUM("Set Encode", SIOCSIWENCODE);

	  if(!strcasecmp(args[i], "on"))
	    {
	      /* Get old encryption information */
	      wrq.u.data.pointer = (caddr_t) key;
	      wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
	      wrq.u.data.flags = 0;
	      IW_GET_EXT_ERR(skfd, ifname, SIOCGIWENCODE, &wrq,
			     "Set Encode");
	      wrq.u.data.flags &= ~IW_ENCODE_DISABLED;	/* Enable */
	    }
	  else
	    {
	      int	gotone = 0;
	      int	oldone;
	      int	keylen;
	      int	temp;

	      wrq.u.data.pointer = (caddr_t) NULL;
	      wrq.u.data.flags = 0;
	      wrq.u.data.length = 0;

	      /* Allow arguments in any order (it's safe) */
	      do
		{
		  oldone = gotone;

		  /* -- Check for the key -- */
		  if(i < count)
		    {
		      keylen = iw_in_key_full(skfd, ifname,
					      args[i], key, &wrq.u.data.flags);
		      if(keylen > 0)
			{
			  wrq.u.data.length = keylen;
			  wrq.u.data.pointer = (caddr_t) key;
			  ++i;
			  gotone++;
			}
		    }

		  /* -- Check for token index -- */
		  if((i < count) &&
		     (sscanf(args[i], "[%i]", &temp) == 1) &&
		     (temp > 0) && (temp < IW_ENCODE_INDEX))
		    {
		      wrq.u.encoding.flags |= temp;
		      ++i;
		      gotone++;
		    }

		  /* -- Check the various flags -- */
		  if((i < count) && (!strcasecmp(args[i], "off")))
		    {
		      wrq.u.data.flags |= IW_ENCODE_DISABLED;
		      ++i;
		      gotone++;
		    }
		  if((i < count) && (!strcasecmp(args[i], "open")))
		    {
		      wrq.u.data.flags |= IW_ENCODE_OPEN;
		      ++i;
		      gotone++;
		    }
		  if((i < count) && (!strncasecmp(args[i], "restricted", 5)))
		    {
		      wrq.u.data.flags |= IW_ENCODE_RESTRICTED;
		      ++i;
		      gotone++;
		    }
#if WIRELESS_EXT > 15
		  if((i < count) && (!strncasecmp(args[i], "temporary", 4)))
		    {
		      wrq.u.data.flags |= IW_ENCODE_TEMP;
		      ++i;
		      gotone++;
		    }
#endif
		}
	      while(gotone != oldone);

	      /* Pointer is absent in new API */
	      if(wrq.u.data.pointer == NULL)
		wrq.u.data.flags |= IW_ENCODE_NOKEY;

	      /* Check if we have any invalid argument */
	      if(!gotone)
		ABORT_ARG_TYPE("Set Encode", SIOCSIWENCODE, args[i]);
	      /* Get back to last processed argument */
	      --i;
	    }

	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWENCODE, &wrq,
			 "Set Encode");
	  continue;
  	}

      /* ---------- Set ESSID ---------- */
      if(!strcasecmp(args[i], "essid"))
	{
	  char		essid[IW_ESSID_MAX_SIZE + 1];

	  i++;
	  if(i >= count)
	    ABORT_ARG_NUM("Set ESSID", SIOCSIWESSID);
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
		wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
		wrq.u.essid.flags = 0;
		IW_GET_EXT_ERR(skfd, ifname, SIOCGIWESSID, &wrq,
			       "Set ESSID");
		wrq.u.essid.flags = 1;
	      }
	    else
	      {
		/* '-' allow to escape the ESSID string, allowing
		 * to set it to the string "any" or "off".
		 * This is a big ugly, but it will do for now */
		if(!strcmp(args[i], "-"))
		  {
		    i++;
		    if(i >= count)
		      ABORT_ARG_NUM("Set ESSID", SIOCSIWESSID);
		  }

		/* Check the size of what the user passed us to avoid
		 * buffer overflows */
		if(strlen(args[i]) > IW_ESSID_MAX_SIZE)
		  ABORT_ARG_SIZE("Set ESSID", SIOCSIWESSID, IW_ESSID_MAX_SIZE);
		else
		  {
		    int		temp;

		    wrq.u.essid.flags = 1;
		    strcpy(essid, args[i]);	/* Size checked, all clear */

		    /* Check for ESSID index */
		    if(((i+1) < count) &&
		       (sscanf(args[i+1], "[%i]", &temp) == 1) &&
		       (temp > 0) && (temp < IW_ENCODE_INDEX))
		      {
			wrq.u.essid.flags = temp;
			++i;
		      }
		  }
	      }

	  wrq.u.essid.pointer = (caddr_t) essid;
	  wrq.u.essid.length = strlen(essid) + 1;
	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWESSID, &wrq,
			 "Set ESSID");
	  continue;
	}

      /* ---------- Set AP address ---------- */
      if(!strcasecmp(args[i], "ap"))
	{
	  if(++i >= count)
	    ABORT_ARG_NUM("Set AP Address", SIOCSIWAP);

	  if((!strcasecmp(args[i], "auto")) ||
	     (!strcasecmp(args[i], "any")))
	    {
	      /* Send a broadcast address */
	      iw_broad_ether(&(wrq.u.ap_addr));
	    }
	  else
	    {
	      if(!strcasecmp(args[i], "off"))
		{
		  /* Send a NULL address */
		  iw_null_ether(&(wrq.u.ap_addr));
		}
	      else
		{
		  /* Get the address and check if the interface supports it */
		  if(iw_in_addr(skfd, ifname, args[i++], &(wrq.u.ap_addr)) < 0)
		    ABORT_ARG_TYPE("Set AP Address", SIOCSIWAP, args[i-1]);
		}
	    }

	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWAP, &wrq,
			 "Set AP Address");
	  continue;
	}

      /* ---------- Set NickName ---------- */
      if(!strncmp(args[i], "nick", 4))
	{
	  i++;
	  if(i >= count)
	    ABORT_ARG_NUM("Set Nickname", SIOCSIWNICKN);
	  if(strlen(args[i]) > IW_ESSID_MAX_SIZE)
	    ABORT_ARG_SIZE("Set Nickname", SIOCSIWNICKN, IW_ESSID_MAX_SIZE);

	  wrq.u.essid.pointer = (caddr_t) args[i];
	  wrq.u.essid.length = strlen(args[i]) + 1;
	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWNICKN, &wrq,
			 "Set Nickname");
	  continue;
	}

      /* ---------- Set Bit-Rate ---------- */
      if((!strncmp(args[i], "bit", 3)) ||
	 (!strcmp(args[i], "rate")))
	{
	  if(++i >= count)
	    ABORT_ARG_NUM("Set Bit Rate", SIOCSIWRATE);
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
		  IW_GET_EXT_ERR(skfd, ifname, SIOCGIWRATE, &wrq,
				 "Set Bit Rate");
		  wrq.u.bitrate.fixed = 1;
		}
	      else			/* Should be a numeric value */
		{
		  double		brate;

		  if(sscanf(args[i], "%lg", &(brate)) != 1)
		    ABORT_ARG_TYPE("Set Bit Rate", SIOCSIWRATE, args[i]);
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

	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWRATE, &wrq,
			 "Set Bit Rate");
	  continue;
	}

      /* ---------- Set RTS threshold ---------- */
      if(!strncasecmp(args[i], "rts", 3))
	{
	  i++;
	  if(i >= count)
	    ABORT_ARG_NUM("Set RTS Threshold", SIOCSIWRTS);
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
		    IW_GET_EXT_ERR(skfd, ifname, SIOCGIWRTS, &wrq,
				   "Set RTS Threshold");
		    wrq.u.rts.fixed = 1;
		  }
		else			/* Should be a numeric value */
		  if(sscanf(args[i], "%li", (unsigned long *) &(wrq.u.rts.value))
		     != 1)
		    ABORT_ARG_TYPE("Set RTS Threshold", SIOCSIWRTS, args[i]);
	    }

	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWRTS, &wrq,
			 "Set RTS Threshold");
	  continue;
	}

      /* ---------- Set fragmentation threshold ---------- */
      if(!strncmp(args[i], "frag", 4))
	{
	  i++;
	  if(i >= count)
	    ABORT_ARG_NUM("Set Fragmentation Threshold", SIOCSIWFRAG);
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
		    IW_GET_EXT_ERR(skfd, ifname, SIOCGIWFRAG, &wrq,
				   "Set Fragmentation Threshold");
		    wrq.u.frag.fixed = 1;
		  }
		else			/* Should be a numeric value */
		  if(sscanf(args[i], "%li",
			    (unsigned long *) &(wrq.u.frag.value))
		     != 1)
		    ABORT_ARG_TYPE("Set Fragmentation Threshold", SIOCSIWFRAG,
				   args[i]);
	    }

	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWFRAG, &wrq,
			 "Set Fragmentation Threshold");
	  continue;
	}

      /* ---------- Set operation mode ---------- */
      if(!strcmp(args[i], "mode"))
	{
	  int	k;

	  i++;
	  if(i >= count)
	    ABORT_ARG_NUM("Set Mode", SIOCSIWMODE);

	  if(sscanf(args[i], "%i", &k) != 1)
	    {
	      k = 0;
	      while((k < IW_NUM_OPER_MODE) &&
		    strncasecmp(args[i], iw_operation_mode[k], 3))
		k++;
	    }
	  if((k >= IW_NUM_OPER_MODE) || (k < 0))
	    ABORT_ARG_TYPE("Set Mode", SIOCSIWMODE, args[i]);

	  wrq.u.mode = k;
	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWMODE, &wrq,
			 "Set Mode");
	  continue;
	}

      /* ---------- Set Power Management ---------- */
      if(!strncmp(args[i], "power", 3))
	{
	  if(++i >= count)
	    ABORT_ARG_NUM("Set Power Management", SIOCSIWPOWER);

	  if(!strcasecmp(args[i], "off"))
	    wrq.u.power.disabled = 1;	/* i.e. max size */
	  else
	    if(!strcasecmp(args[i], "on"))
	      {
		/* Get old Power info */
		IW_GET_EXT_ERR(skfd, ifname, SIOCGIWPOWER, &wrq,
			       "Set Power Management");
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
		      ABORT_ARG_NUM("Set Power Management", SIOCSIWPOWER);
		  }
		else
		  if(!strcasecmp(args[i], "max"))
		    {
		      wrq.u.power.flags |= IW_POWER_MAX;
		      if(++i >= count)
			ABORT_ARG_NUM("Set Power Management", SIOCSIWPOWER);
		    }

		/* Check value type */
		if(!strcasecmp(args[i], "period"))
		  {
		    wrq.u.power.flags |= IW_POWER_PERIOD;
		    if(++i >= count)
		      ABORT_ARG_NUM("Set Power Management", SIOCSIWPOWER);
		  }
		else
		  if(!strcasecmp(args[i], "timeout"))
		    {
		      wrq.u.power.flags |= IW_POWER_TIMEOUT;
		      if(++i >= count)
			ABORT_ARG_NUM("Set Power Management", SIOCSIWPOWER);
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
		  ABORT_ARG_TYPE("Set Power Management", SIOCSIWPOWER,
				 args[i]);
		--i;
	      }

	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWPOWER, &wrq,
		       "Set Power Management");
	  continue;
  	}

#if WIRELESS_EXT > 9
      /* ---------- Set Transmit-Power ---------- */
      if(!strncmp(args[i], "txpower", 3))
	{
	  struct iw_range	range;

	  if(++i >= count)
	    ABORT_ARG_NUM("Set Tx Power", SIOCSIWTXPOW);

	  /* Extract range info */
	  if(iw_get_range_info(skfd, ifname, &range) < 0)
	    memset(&range, 0, sizeof(range));

	  /* Prepare the request */
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
		    IW_GET_EXT_ERR(skfd, ifname, SIOCGIWTXPOW, &wrq,
				   "Set Tx Power");
		    wrq.u.txpower.fixed = 1;
		  }
		else			/* Should be a numeric value */
		  {
		    int		power;
		    int		ismwatt = 0;

		    /* Get the value */
		    if(sscanf(args[i], "%i", &(power)) != 1)
		      ABORT_ARG_TYPE("Set Tx Power", SIOCSIWTXPOW, args[i]);

		    /* Check if milliwatt */
		    ismwatt = (index(args[i], 'm') != NULL);

		    /* Convert */
		    if(range.txpower_capa & IW_TXPOW_MWATT)
		      {
			if(!ismwatt)
			  power = iw_dbm2mwatt(power);
			wrq.u.data.flags = IW_TXPOW_MWATT;
		      }
		    else
		      {
			if(ismwatt)
			  power = iw_mwatt2dbm(power);
			wrq.u.data.flags = IW_TXPOW_DBM;
		      }
		    wrq.u.txpower.value = power;

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

	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWTXPOW, &wrq,
			 "Set Tx Power");
	  continue;
	}
#endif

#if WIRELESS_EXT > 10
      /* ---------- Set Retry limit ---------- */
      if(!strncmp(args[i], "retry", 3))
	{
	  double		temp;
	  int		gotone = 0;

	  if(++i >= count)
	    ABORT_ARG_NUM("Set Retry Limit", SIOCSIWRETRY);

	  /* Default - nope */
	  wrq.u.retry.flags = IW_RETRY_LIMIT;
	  wrq.u.retry.disabled = 0;

	  /* Check value modifier */
	  if(!strcasecmp(args[i], "min"))
	    {
	      wrq.u.retry.flags |= IW_RETRY_MIN;
	      if(++i >= count)
		ABORT_ARG_NUM("Set Retry Limit", SIOCSIWRETRY);
	    }
	  else
	    if(!strcasecmp(args[i], "max"))
	      {
		wrq.u.retry.flags |= IW_RETRY_MAX;
		if(++i >= count)
		  ABORT_ARG_NUM("Set Retry Limit", SIOCSIWRETRY);
	      }

	  /* Check value type */
	  if(!strcasecmp(args[i], "limit"))
	    {
	      wrq.u.retry.flags |= IW_RETRY_LIMIT;
	      if(++i >= count)
		ABORT_ARG_NUM("Set Retry Limit", SIOCSIWRETRY);
	    }
	  else
	    if(!strncasecmp(args[i], "lifetime", 4))
	      {
		wrq.u.retry.flags &= ~IW_RETRY_LIMIT;
		wrq.u.retry.flags |= IW_RETRY_LIFETIME;
		if(++i >= count)
		  ABORT_ARG_NUM("Set Retry Limit", SIOCSIWRETRY);
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
	    ABORT_ARG_TYPE("Set Retry Limit", SIOCSIWRETRY, args[i]);
	  --i;

	  IW_SET_EXT_ERR(skfd, ifname, SIOCSIWRETRY, &wrq,
			 "Set Retry Limit");
	  continue;
	}

#endif	/* WIRELESS_EXT > 10 */

      /* ---------- Other ---------- */
      /* Here we have an unrecognised arg... */
      fprintf(stderr, "Error : unrecognised wireless request \"%s\"\n",
	      args[i]);
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
  int skfd;		/* generic raw socket desc.	*/
  int goterr = 0;

  /* Create a channel to the NET kernel. */
  if((skfd = iw_sockets_open()) < 0)
    {
      perror("socket");
      exit(-1);
    }

  /* No argument : show the list of all device + info */
  if(argc == 1)
    iw_enum_devices(skfd, &print_info, NULL, 0);
  else
    /* Special case for help... */
    if((!strcmp(argv[1], "-h")) || (!strcmp(argv[1], "--help")))
      iw_usage();
    else
      /* Special case for version... */
      if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
	goterr = iw_print_version_info("iwconfig");
      else
	/* The device name must be the first argument */
	if(argc == 2)
	  print_info(skfd, argv[1], NULL, 0);
	else
	  /* The other args on the line specify options to be set... */
	  goterr = set_info(skfd, argv + 2, argc - 2, argv[1]);

  /* Close the socket. */
  close(skfd);

  return(goterr);
}
