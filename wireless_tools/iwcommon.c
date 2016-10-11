/*
 *	Wireless Tools
 *
 *		Jean II - HPLB '99
 *
 * Common subroutines to all the wireless tools...
 */

#include "iwcommon.h"		/* Header */

/************************ SOCKET SUBROUTINES *************************/

/*------------------------------------------------------------------*/
/*
 * Open a socket.
 * Depending on the protocol present, open the right socket. The socket
 * will allow us to talk to the driver.
 */
int
sockets_open(void)
{
	int ipx_sock = -1;		/* IPX socket			*/
	int ax25_sock = -1;		/* AX.25 socket			*/
	int inet_sock = -1;		/* INET socket			*/
	int ddp_sock = -1;		/* Appletalk DDP socket		*/

	inet_sock=socket(AF_INET, SOCK_DGRAM, 0);
	ipx_sock=socket(AF_IPX, SOCK_DGRAM, 0);
	ax25_sock=socket(AF_AX25, SOCK_DGRAM, 0);
	ddp_sock=socket(AF_APPLETALK, SOCK_DGRAM, 0);
	/*
	 * Now pick any (exisiting) useful socket family for generic queries
	 */
	if(inet_sock!=-1)
		return inet_sock;
	if(ipx_sock!=-1)
		return ipx_sock;
	if(ax25_sock!=-1)
		return ax25_sock;
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
get_range_info(int		skfd,
	       char *		ifname,
	       iwrange *	range)
{
  struct iwreq		wrq;

  strcpy(wrq.ifr_name, ifname);
  wrq.u.data.pointer = (caddr_t) range;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCGIWRANGE, &wrq) < 0)
    return(-1);

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Get information about what private ioctls are supported by the driver
 */
int
get_priv_info(int		skfd,
	      char *		ifname,
	      iwprivargs *	priv)
{
  struct iwreq		wrq;

  /* Ask the driver */
  strcpy(wrq.ifr_name, ifname);
  wrq.u.data.pointer = (caddr_t) priv;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCGIWPRIV, &wrq) < 0)
    return(-1);

  /* Return the number of ioctls */
  return(wrq.u.data.length);
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
float2freq(double	in,
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
freq2float(iwfreq *	in)
{
  return ((double) in->m) * pow(10,in->e);
}


/*********************** ADDRESS SUBROUTINES ************************/
/*
 * This section is mostly a cut & past from net-tools-1.2.0
 * manage address display and input...
 */

/*------------------------------------------------------------------*/
/*
 * Check if interface support the right address types...
 */
int
check_addr_type(int	skfd,
		char *	ifname)
{
  struct ifreq		ifr;

  /* Get the type of interface address */
  strcpy(ifr.ifr_name, ifname);
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

  /* Get the type of hardware address */
  strcpy(ifr.ifr_name, ifname);
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
 * Display an Ethernet address in readable format.
 */
char *
pr_ether(unsigned char *ptr)
{
  static char buff[64];

  sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X",
	(ptr[0] & 0xFF), (ptr[1] & 0xFF), (ptr[2] & 0xFF),
	(ptr[3] & 0xFF), (ptr[4] & 0xFF), (ptr[5] & 0xFF)
  );
  return(buff);
}

/*------------------------------------------------------------------*/
/*
 * Input an Ethernet address and convert to binary.
 */
int
in_ether(char *bufp, struct sockaddr *sap)
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
in_inet(char *bufp, struct sockaddr *sap)
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
in_addr(int		skfd,
	char *		ifname,
	char *		bufp,
	struct sockaddr *sap)
{
  /* Check if it is a hardware or IP address */
  if(index(bufp, ':') == NULL)
    {
      struct sockaddr	if_address;
      struct arpreq	arp_query;

      /* Read interface address */
      if(in_inet(bufp, &if_address) < 0)
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
      strcpy(arp_query.arp_dev, ifname);
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
    /* Get the hardware address */
    if(in_ether(bufp, sap) < 0)
      {
	fprintf(stderr, "Invalid hardware address %s\n", bufp);
	return(-1);
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
byte_size(int	args)
{
  int	ret = args & IW_PRIV_SIZE_MASK;

  if(((args & IW_PRIV_TYPE_MASK) == IW_PRIV_TYPE_INT) ||
     ((args & IW_PRIV_TYPE_MASK) == IW_PRIV_TYPE_FLOAT))
    ret <<= 2;

  if((args & IW_PRIV_TYPE_MASK) == IW_PRIV_TYPE_NONE)
    return 0;

  return ret;
}

