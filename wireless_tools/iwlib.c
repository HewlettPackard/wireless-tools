/*
 *	Wireless Tools
 *
 *		Jean II - HPLB 97->99 - HPL 99->01
 *
 * Common subroutines to all the wireless tools...
 *
 * This file is released under the GPL license.
 *     Copyright (c) 1997-2002 Jean Tourrilhes <jt@hpl.hp.com>
 */

#include "iwlib.h"		/* Header */

/**************************** VARIABLES ****************************/

const char *	iw_operation_mode[] = { "Auto",
					"Ad-Hoc",
					"Managed",
					"Master",
					"Repeater",
					"Secondary" };

/************************ SOCKET SUBROUTINES *************************/

/*------------------------------------------------------------------*/
/*
 * Open a socket.
 * Depending on the protocol present, open the right socket. The socket
 * will allow us to talk to the driver.
 */
int
iw_sockets_open(void)
{
        int ipx_sock = -1;              /* IPX socket                   */
        int ax25_sock = -1;             /* AX.25 socket                 */
        int inet_sock = -1;             /* INET socket                  */
        int ddp_sock = -1;              /* Appletalk DDP socket         */

        /*
         * Now pick any (exisiting) useful socket family for generic queries
	 * Note : don't open all the socket, only returns when one matches,
	 * all protocols might not be valid.
	 * Workaround by Jim Kaba <jkaba@sarnoff.com>
	 * Note : in 99% of the case, we will just open the inet_sock.
	 * The remaining 1% case are not fully correct...
         */
        inet_sock=socket(AF_INET, SOCK_DGRAM, 0);
        if(inet_sock!=-1)
                return inet_sock;
        ipx_sock=socket(AF_IPX, SOCK_DGRAM, 0);
        if(ipx_sock!=-1)
                return ipx_sock;
        ax25_sock=socket(AF_AX25, SOCK_DGRAM, 0);
        if(ax25_sock!=-1)
                return ax25_sock;
        ddp_sock=socket(AF_APPLETALK, SOCK_DGRAM, 0);
        /*
         * If this is -1 we have no known network layers and its time to jump.
         */
        return ddp_sock;
}

/*********************** WIRELESS SUBROUTINES ************************/

/*------------------------------------------------------------------*/
/*
 * Get the range information out of the driver
 */
int
iw_get_range_info(int		skfd,
		  char *	ifname,
		  iwrange *	range)
{
  struct iwreq		wrq;
  char			buffer[sizeof(iwrange) * 2];	/* Large enough */

  /* Cleanup */
  memset(buffer, 0, sizeof(iwrange) * 2);

  wrq.u.data.pointer = (caddr_t) buffer;
  wrq.u.data.length = sizeof(iwrange) * 2;
  wrq.u.data.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWRANGE, &wrq) < 0)
    return(-1);

  /* Copy stuff at the right place, ignore extra */
  memcpy((char *) range, buffer, sizeof(iwrange));

  /* Lot's of people have driver and tools out of sync as far as Wireless
   * Extensions are concerned. It's because /usr/include/linux/wireless.h
   * and /usr/src/linux/include/linux/wireless.h are different.
   * We try to catch this stuff here... */

  /* For new versions, we can check the version directly, for old versions
   * we use magic. 300 bytes is a also magic number, don't touch... */
  if((WIRELESS_EXT > 10) && (wrq.u.data.length >= 300))
    {
#if WIRELESS_EXT > 10
      /* Version verification - for new versions */
      if(range->we_version_compiled != WIRELESS_EXT)
	{
	  fprintf(stderr, "Warning : Device %s has been compiled with version %d\n", ifname, range->we_version_compiled);
	  fprintf(stderr, "of Wireless Extension, while we are using version %d.\n", WIRELESS_EXT);
	  fprintf(stderr, "Some things may be broken...\n\n");
	}
#endif /* WIRELESS_EXT > 10 */
    }
  else
    {
      /* Version verification - for old versions */
      if(wrq.u.data.length != sizeof(iwrange))
	{
	  fprintf(stderr, "Warning : Device %s has been compiled with a different version\n", ifname);
	  fprintf(stderr, "of Wireless Extension than ours (we are using version %d).\n", WIRELESS_EXT);
	  fprintf(stderr, "Some things may be broken...\n\n");
	}
    }

  /* Note : we are only trying to catch compile difference, not source.
   * If the driver source has not been updated to the latest, it doesn't
   * matter because the new fields are set to zero */

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Get information about what private ioctls are supported by the driver
 */
int
iw_get_priv_info(int		skfd,
		 char *		ifname,
		 iwprivargs *	priv)
{
  struct iwreq		wrq;

  /* Ask the driver */
  wrq.u.data.pointer = (caddr_t) priv;
  wrq.u.data.length = 32;
  wrq.u.data.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWPRIV, &wrq) < 0)
    return(-1);

  /* Return the number of ioctls */
  return(wrq.u.data.length);
}

/*------------------------------------------------------------------*/
/*
 * Get essential wireless config from the device driver
 * We will call all the classical wireless ioctl on the driver through
 * the socket to know what is supported and to get the settings...
 * Note : compare to the version in iwconfig, we extract only
 * what's *really* needed to configure a device...
 */
int
iw_get_basic_config(int			skfd,
		    char *		ifname,
		    wireless_config *	info)
{
  struct iwreq		wrq;

  memset((char *) info, 0, sizeof(struct wireless_config));

  /* Get wireless name */
  if(iw_get_ext(skfd, ifname, SIOCGIWNAME, &wrq) < 0)
    /* If no wireless name : no wireless extensions */
    return(-1);
  else
    strcpy(info->name, wrq.u.name);

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
  wrq.u.essid.length = IW_ESSID_MAX_SIZE;
  wrq.u.essid.flags = 0;
  if(iw_get_ext(skfd, ifname, SIOCGIWESSID, &wrq) >= 0)
    {
      info->has_essid = 1;
      info->essid_on = wrq.u.data.flags;
    }

  /* Get operation mode */
  if(iw_get_ext(skfd, ifname, SIOCGIWMODE, &wrq) >= 0)
    {
      if((wrq.u.mode < 6) && (wrq.u.mode >= 0))
	info->has_mode = 1;
      info->mode = wrq.u.mode;
    }

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Set essential wireless config in the device driver
 * We will call all the classical wireless ioctl on the driver through
 * the socket to know what is supported and to set the settings...
 * We support only the restricted set as above...
 */
int
iw_set_basic_config(int			skfd,
		    char *		ifname,
		    wireless_config *	info)
{
  struct iwreq		wrq;
  int			ret = 0;

  /* Get wireless name (check if interface is valid) */
  if(iw_get_ext(skfd, ifname, SIOCGIWNAME, &wrq) < 0)
    /* If no wireless name : no wireless extensions */
    return(-2);

  /* Set Network ID, if available (this is for non-802.11 cards) */
  if(info->has_nwid)
    {
      memcpy(&(wrq.u.nwid), &(info->nwid), sizeof(iwparam));
      wrq.u.nwid.fixed = 1;	/* Hum... When in Rome... */

      if(iw_set_ext(skfd, ifname, SIOCSIWNWID, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCSIWNWID: %s\n", strerror(errno));
	  ret = -1;
	}
    }

  /* Set frequency / channel */
  if(info->has_freq)
    {
      iw_float2freq(info->freq, &(wrq.u.freq));

      if(iw_set_ext(skfd, ifname, SIOCSIWFREQ, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCSIWFREQ: %s\n", strerror(errno));
	  ret = -1;
	}
    }

  /* Set encryption information */
  if(info->has_key)
    {
      int		flags = info->key_flags;

      /* Check if there is a key index */
      if((flags & IW_ENCODE_INDEX) > 0)
	{
	  /* Set the index */
	  wrq.u.data.pointer = (caddr_t) NULL;
	  wrq.u.data.flags = (flags & (IW_ENCODE_INDEX)) | IW_ENCODE_NOKEY;
	  wrq.u.data.length = 0;

	  if(iw_set_ext(skfd, ifname, SIOCSIWENCODE, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWENCODE(%d): %s\n",
		      errno, strerror(errno));
	      ret = -1;
	    }
	}

      /* Mask out index to minimise probability of reject when setting key */
      flags = flags & (~IW_ENCODE_INDEX);

      /* Set the key itself (set current key in this case) */
      wrq.u.data.pointer = (caddr_t) info->key;
      wrq.u.data.length = info->key_size;
      wrq.u.data.flags = flags;

      if(iw_set_ext(skfd, ifname, SIOCSIWENCODE, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCSIWENCODE(%d): %s\n",
		  errno, strerror(errno));
	  ret = -1;
	}
    }

  /* Set ESSID (extended network), if available */
  if(info->has_essid)
    {
      wrq.u.essid.pointer = (caddr_t) info->essid;
      wrq.u.essid.length = strlen(info->essid) + 1;
      wrq.u.data.flags = info->essid_on;

      if(iw_set_ext(skfd, ifname, SIOCSIWESSID, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCSIWESSID: %s\n", strerror(errno));
	  ret = -1;
	}
    }

  /* Set the current mode of operation */
  if(info->has_mode)
    {
      strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
      wrq.u.mode = info->mode;

      if(iw_get_ext(skfd, ifname, SIOCSIWMODE, &wrq) < 0)
	{
	  fprintf(stderr, "SIOCSIWMODE: %s\n", strerror(errno));
	  ret = -1;
	}
    }

  return(ret);
}

/********************** FREQUENCY SUBROUTINES ***********************/

/*------------------------------------------------------------------*/
/*
 * Convert a floating point the our internal representation of
 * frequencies.
 * The kernel doesn't want to hear about floating point, so we use
 * this custom format instead.
 */
void
iw_float2freq(double	in,
	      iwfreq *	out)
{
  out->e = (short) (floor(log10(in)));
  if(out->e > 8)
    {
      out->m = ((long) (floor(in / pow(10,out->e - 6)))) * 100;
      out->e -= 8;
    }
  else
    {
      out->m = in;
      out->e = 0;
    }
}

/*------------------------------------------------------------------*/
/*
 * Convert our internal representation of frequencies to a floating point.
 */
double
iw_freq2float(iwfreq *	in)
{
  return ((double) in->m) * pow(10,in->e);
}

/************************ POWER SUBROUTINES *************************/

/*------------------------------------------------------------------*/
/*
 * Convert a value in dBm to a value in milliWatt.
 */
int
iw_dbm2mwatt(int	in)
{
  return((int) (floor(pow(10.0, (((double) in) / 10.0)))));
}

/*------------------------------------------------------------------*/
/*
 * Convert a value in milliWatt to a value in dBm.
 */
int
iw_mwatt2dbm(int	in)
{
  return((int) (ceil(10.0 * log10((double) in))));
}

/********************** STATISTICS SUBROUTINES **********************/

/*------------------------------------------------------------------*/
/*
 * Read /proc/net/wireless to get the latest statistics
 */
int
iw_get_stats(int	skfd,
	     char *	ifname,
	     iwstats *	stats)
{
#if WIRELESS_EXT > 11
  struct iwreq		wrq;
  wrq.u.data.pointer = (caddr_t) stats;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 1;		/* Clear updated flag */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  if(iw_get_ext(skfd, ifname, SIOCGIWSTATS, &wrq) < 0)
    return(-1);

  return(0);
#else /* WIRELESS_EXT > 11 */
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
#endif /* WIRELESS_EXT > 11 */
}

/*------------------------------------------------------------------*/
/*
 * Output the link statistics, taking care of formating
 */
void
iw_print_stats(char *		buffer,
	       iwqual *		qual,
	       iwrange *	range,
	       int		has_range)
{
  /* Just do it */
  if(has_range && (qual->level != 0))
    {
      /* If the statistics are in dBm */
      if(qual->level > range->max_qual.level)
	{
	  /* Statistics are in dBm (absolute power measurement) */
	  sprintf(buffer,
		  "Quality:%d/%d  Signal level:%d dBm  Noise level:%d dBm%s",
		  qual->qual, range->max_qual.qual,
		  qual->level - 0x100, qual->noise - 0x100,
		  (qual->updated & 0x7) ? " (updated)" : "");
	}
      else
	{
	  /* Statistics are relative values (0 -> max) */
	  sprintf(buffer,
		  "Quality:%d/%d  Signal level:%d/%d  Noise level:%d/%d%s",
		  qual->qual, range->max_qual.qual,
		  qual->level, range->max_qual.level,
		  qual->noise, range->max_qual.noise,
		  (qual->updated & 0x7) ? " (updated)" : "");
	}
    }
  else
    {
      /* We can't read the range, so we don't know... */
      sprintf(buffer, "Quality:%d  Signal level:%d  Noise level:%d%s",
	      qual->qual, qual->level, qual->noise,
	      (qual->updated & 0x7) ? " (updated)" : "");
    }
}

/*********************** ENCODING SUBROUTINES ***********************/

/*------------------------------------------------------------------*/
/*
 * Output the encoding key, with a nice formating
 */
void
iw_print_key(char *		buffer,
	     unsigned char *	key,
	     int		key_size,
	     int		key_flags)
{
  int	i;

  /* Is the key present ??? */
  if(key_flags & IW_ENCODE_NOKEY)
    {
      /* Nope : print dummy */
      strcpy(buffer, "**");
      buffer +=2;
      for(i = 1; i < key_size; i++)
	{
	  if((i & 0x1) == 0)
	    strcpy(buffer++, "-");
	  strcpy(buffer, "**");
	  buffer +=2;
	}
    }
  else
    {
      /* Yes : print the key */
      sprintf(buffer, "%.2X", key[0]);
      buffer +=2;
      for(i = 1; i < key_size; i++)
	{
	  if((i & 0x1) == 0)
	    strcpy(buffer++, "-");
	  sprintf(buffer, "%.2X", key[i]);
	  buffer +=2;
	}
    }
}


/******************* POWER MANAGEMENT SUBROUTINES *******************/

/*------------------------------------------------------------------*/
/*
 * Output a power management value with all attributes...
 */
void
iw_print_pm_value(char *	buffer,
		  int		value,
		  int		flags)
{
  /* Modifiers */
  if(flags & IW_POWER_MIN)
    {
      strcpy(buffer, " min");
      buffer += 4;
    }
  if(flags & IW_POWER_MAX)
    {
      strcpy(buffer, " max");
      buffer += 4;
    }

  /* Type */
  if(flags & IW_POWER_TIMEOUT)
    {
      strcpy(buffer, " timeout:");
      buffer += 9;
    }
  else
    {
      strcpy(buffer, " period:");
      buffer += 8;
    }

  /* Display value without units */
  if(flags & IW_POWER_RELATIVE)
    sprintf(buffer, "%g  ", ((double) value) / MEGA);
  else
    {
      /* Display value with units */
      if(value >= (int) MEGA)
	sprintf(buffer, "%gs  ", ((double) value) / MEGA);
      else
	if(value >= (int) KILO)
	  sprintf(buffer, "%gms  ", ((double) value) / KILO);
	else
	  sprintf(buffer, "%dus  ", value);
    }
}

/*------------------------------------------------------------------*/
/*
 * Output a power management mode
 */
void
iw_print_pm_mode(char *	buffer,
		 int	flags)
{
  /* Print the proper mode... */
  switch(flags & IW_POWER_MODE)
    {
    case IW_POWER_UNICAST_R:
      strcpy(buffer, " mode:Unicast only received");
      break;
    case IW_POWER_MULTICAST_R:
      strcpy(buffer, " mode:Multicast only received");
      break;
    case IW_POWER_ALL_R:
      strcpy(buffer, " mode:All packets received");
      break;
    case IW_POWER_FORCE_S:
      strcpy(buffer, " mode:Force sending");
      break;
    case IW_POWER_REPEATER:
      strcpy(buffer, " mode:Repeat multicasts");
      break;
    default:
    }
}

/***************** RETRY LIMIT/LIFETIME SUBROUTINES *****************/

#if WIRELESS_EXT > 10
/*------------------------------------------------------------------*/
/*
 * Output a retry value with all attributes...
 */
void
iw_print_retry_value(char *	buffer,
		     int	value,
		     int	flags)
{
  /* Modifiers */
  if(flags & IW_RETRY_MIN)
    {
      strcpy(buffer, " min");
      buffer += 4;
    }
  if(flags & IW_RETRY_MAX)
    {
      strcpy(buffer, " max");
      buffer += 4;
    }

  /* Type lifetime of limit */
  if(flags & IW_RETRY_LIFETIME)
    {
      strcpy(buffer, " lifetime:");
      buffer += 10;

      /* Display value without units */
      if(flags & IW_POWER_RELATIVE)
	sprintf(buffer, "%g", ((double) value) / MEGA);
      else
	{
	  /* Display value with units */
	  if(value >= (int) MEGA)
	    sprintf(buffer, "%gs", ((double) value) / MEGA);
	  else
	    if(value >= (int) KILO)
	      sprintf(buffer, "%gms", ((double) value) / KILO);
	    else
	      sprintf(buffer, "%dus", value);
	}
    }
  else
    sprintf(buffer, " limit:%d", value);
}
#endif	/* WIRELESS_EXT > 10 */

/*********************** ADDRESS SUBROUTINES ************************/
/*
 * This section is mostly a cut & past from net-tools-1.2.0
 * manage address display and input...
 */

/*------------------------------------------------------------------*/
/*
 * Check if interface support the right MAC address type...
 */
int
iw_check_mac_addr_type(int		skfd,
		       char *		ifname)
{
  struct ifreq		ifr;

  /* Get the type of hardware address */
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  if((ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) ||
     (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER))
    {
      /* Deep trouble... */
      fprintf(stderr, "Interface %s doesn't support MAC addresses\n",
	     ifname);
      return(-1);
    }

#ifdef DEBUG
  printf("Hardware : %d - %s\n", ifr.ifr_hwaddr.sa_family,
	 pr_ether(ifr.ifr_hwaddr.sa_data));
#endif

  return(0);
}


/*------------------------------------------------------------------*/
/*
 * Check if interface support the right interface address type...
 */
int
iw_check_if_addr_type(int		skfd,
		      char *		ifname)
{
  struct ifreq		ifr;

  /* Get the type of interface address */
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  if((ioctl(skfd, SIOCGIFADDR, &ifr) < 0) ||
     (ifr.ifr_addr.sa_family !=  AF_INET))
    {
      /* Deep trouble... */
      fprintf(stderr, "Interface %s doesn't support IP addresses\n", ifname);
      return(-1);
    }

#ifdef DEBUG
  printf("Interface : %d - 0x%lX\n", ifr.ifr_addr.sa_family,
	 *((unsigned long *) ifr.ifr_addr.sa_data));
#endif

  return(0);
}

#if 0
/*------------------------------------------------------------------*/
/*
 * Check if interface support the right address types...
 */
int
iw_check_addr_type(int		skfd,
		   char *	ifname)
{
  /* Check the interface address type */
  if(iw_check_if_addr_type(skfd, ifname) < 0)
    return(-1);

  /* Check the interface address type */
  if(iw_check_mac_addr_type(skfd, ifname) < 0)
    return(-1);

  return(0);
}
#endif

/*------------------------------------------------------------------*/
/*
 * Display an Ethernet address in readable format.
 */
char *
iw_pr_ether(char *		buffer,
	    unsigned char *	ptr)
{
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
	  (ptr[0] & 0xFF), (ptr[1] & 0xFF), (ptr[2] & 0xFF),
	  (ptr[3] & 0xFF), (ptr[4] & 0xFF), (ptr[5] & 0xFF)
  );
  return(buffer);
}

/*------------------------------------------------------------------*/
/*
 * Input an Ethernet address and convert to binary.
 */
int
iw_in_ether(char *bufp, struct sockaddr *sap)
{
  unsigned char *ptr;
  char c, *orig;
  int i, val;

  sap->sa_family = ARPHRD_ETHER;
  ptr = sap->sa_data;

  i = 0;
  orig = bufp;
  while((*bufp != '\0') && (i < ETH_ALEN)) {
	val = 0;
	c = *bufp++;
	if (isdigit(c)) val = c - '0';
	  else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
	  else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
	  else {
#ifdef DEBUG
		fprintf(stderr, "in_ether(%s): invalid ether address!\n", orig);
#endif
		errno = EINVAL;
		return(-1);
	}
	val <<= 4;
	c = *bufp++;
	if (isdigit(c)) val |= c - '0';
	  else if (c >= 'a' && c <= 'f') val |= c - 'a' + 10;
	  else if (c >= 'A' && c <= 'F') val |= c - 'A' + 10;
	  else {
#ifdef DEBUG
		fprintf(stderr, "in_ether(%s): invalid ether address!\n", orig);
#endif
		errno = EINVAL;
		return(-1);
	}
	*ptr++ = (unsigned char) (val & 0377);
	i++;

	/* We might get a semicolon here - not required. */
	if (*bufp == ':') {
		if (i == ETH_ALEN) {
#ifdef DEBUG
			fprintf(stderr, "in_ether(%s): trailing : ignored!\n",
				orig)
#endif
						; /* nothing */
		}
		bufp++;
	}
  }

  /* That's it.  Any trailing junk? */
  if ((i == ETH_ALEN) && (*bufp != '\0')) {
#ifdef DEBUG
	fprintf(stderr, "in_ether(%s): trailing junk!\n", orig);
	errno = EINVAL;
	return(-1);
#endif
  }

#ifdef DEBUG
  fprintf(stderr, "in_ether(%s): %s\n", orig, pr_ether(sap->sa_data));
#endif

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Input an Internet address and convert to binary.
 */
int
iw_in_inet(char *bufp, struct sockaddr *sap)
{
  struct hostent *hp;
  struct netent *np;
  char *name = bufp;
  struct sockaddr_in *sin = (struct sockaddr_in *) sap;

  /* Grmpf. -FvK */
  sin->sin_family = AF_INET;
  sin->sin_port = 0;

  /* Default is special, meaning 0.0.0.0. */
  if (!strcmp(name, "default")) {
	sin->sin_addr.s_addr = INADDR_ANY;
	return(1);
  }

  /* Try the NETWORKS database to see if this is a known network. */
  if ((np = getnetbyname(name)) != (struct netent *)NULL) {
	sin->sin_addr.s_addr = htonl(np->n_net);
	strcpy(name, np->n_name);
	return(1);
  }

  if ((hp = gethostbyname(name)) == (struct hostent *)NULL) {
	errno = h_errno;
	return(-1);
  }
  memcpy((char *) &sin->sin_addr, (char *) hp->h_addr_list[0], hp->h_length);
  strcpy(name, hp->h_name);
  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Input an address and convert to binary.
 */
int
iw_in_addr(int		skfd,
	   char *	ifname,
	   char *	bufp,
	   struct sockaddr *sap)
{
  /* Check if it is a hardware or IP address */
  if(index(bufp, ':') == NULL)
    {
      struct sockaddr	if_address;
      struct arpreq	arp_query;

      /* Check if we have valid interface address type */
      if(iw_check_if_addr_type(skfd, ifname) < 0)
	{
	  fprintf(stderr, "%-8.8s  Interface doesn't support IP addresses\n", ifname);
	  return(-1);
	}

      /* Read interface address */
      if(iw_in_inet(bufp, &if_address) < 0)
	{
	  fprintf(stderr, "Invalid interface address %s\n", bufp);
	  return(-1);
	}

      /* Translate IP addresses to MAC addresses */
      memcpy((char *) &(arp_query.arp_pa),
	     (char *) &if_address,
	     sizeof(struct sockaddr));
      arp_query.arp_ha.sa_family = 0;
      arp_query.arp_flags = 0;
      /* The following restrict the search to the interface only */
      /* For old kernels which complain, just comment it... */
      strncpy(arp_query.arp_dev, ifname, IFNAMSIZ);
      if((ioctl(skfd, SIOCGARP, &arp_query) < 0) ||
	 !(arp_query.arp_flags & ATF_COM))
	{
	  fprintf(stderr, "Arp failed for %s on %s... (%d)\nTry to ping the address before setting it.\n",
		  bufp, ifname, errno);
	  return(-1);
	}

      /* Store new MAC address */
      memcpy((char *) sap,
	     (char *) &(arp_query.arp_ha),
	     sizeof(struct sockaddr));

#ifdef DEBUG
      printf("IP Address %s => Hw Address = %s\n",
	     bufp, pr_ether(sap->sa_data));
#endif
    }
  else	/* If it's an hardware address */
    {
      /* Check if we have valid mac address type */
      if(iw_check_mac_addr_type(skfd, ifname) < 0)
	{
	  fprintf(stderr, "%-8.8s  Interface doesn't support MAC addresses\n", ifname);
	  return(-1);
	}

      /* Get the hardware address */
      if(iw_in_ether(bufp, sap) < 0)
	{
	  fprintf(stderr, "Invalid hardware address %s\n", bufp);
	  return(-1);
	}
    }

#ifdef DEBUG
  printf("Hw Address = %s\n", pr_ether(sap->sa_data));
#endif

  return(0);
}

/************************* MISC SUBROUTINES **************************/

/*------------------------------------------------------------------*/
/*
 */
int
iw_byte_size(int	args)
{
  int	ret = args & IW_PRIV_SIZE_MASK;

  if(((args & IW_PRIV_TYPE_MASK) == IW_PRIV_TYPE_INT) ||
     ((args & IW_PRIV_TYPE_MASK) == IW_PRIV_TYPE_FLOAT))
    ret <<= 2;

  if((args & IW_PRIV_TYPE_MASK) == IW_PRIV_TYPE_NONE)
    return 0;

  return ret;
}

