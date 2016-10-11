/*
 *	Wireless Tools
 *
 *		Jean II - HPL '01
 *
 * Just print the ESSID or NWID...
 *
 * This file is released under the GPL license.
 */

#include "iwcommon.h"		/* Header */

#define FORMAT_DEFAULT	0	/* Nice looking display for the user */
#define FORMAT_SCHEME	1	/* To be used as a Pcmcia Scheme */

/*
 * Note on Pcmcia Schemes :
 * ----------------------
 *	The purpose of this tool is to use the ESSID discovery mechanism
 * to select the appropriate Pcmcia Scheme. The card tell us which
 * ESSID it has found, and we can then select the appropriate Pcmcia
 * Scheme for this ESSID (Wireless config (encrypt keys) and IP config).
 *	The way to do it is as follows :
 *			cardctl scheme "essidany"
 *			delay 100
 *			$scheme = iwgetid --scheme
 *			cardctl scheme $scheme
 *	Of course, you need to add a scheme called "essidany" with the
 * following setting :
 *			essidany,*,*,*)
 *				ESSID="any"
 *				IPADDR="10.0.0.1"
 *
 *	This can also be integrated int he Pcmcia scripts.
 *	Some drivers don't activate the card up to "ifconfig up".
 * Therefore, they wont scan ESSID up to this point, so we can't
 * read it reliably in Pcmcia scripts.
 *	I guess the proper way to write the network script is as follows :
 *			if($scheme == "iwgetid") {
 *				iwconfig $name essid any
 *				iwconfig $name nwid any
 *				ifconfig $name up
 *				delay 100
 *				$scheme = iwgetid $name --scheme
 *				ifconfig $name down
 *			}
 *
 *	This is pseudo code, but you get an idea...
 *	The "ifconfig up" activate the card.
 *	The "delay" is necessary to let time for the card scan the
 * frequencies and associate with the AP.
 *	The "ifconfig down" is necessary to allow the driver to optimise
 * the wireless parameters setting (minimise number of card resets).
 *
 *	Another cute idea is to have a list of Pcmcia Schemes to try
 * and to keep the first one that associate (AP address != 0). This
 * would be necessary for closed networks and cards that can't
 * discover essid...
 *
 * Jean II - 29/3/01
 */

/************************ DISPLAY ESSID/NWID ************************/

/*------------------------------------------------------------------*/
/*
 * Display the ESSID if possible
 */
static int
print_essid(int		skfd,
	    char *	ifname,
	    int		format)
{
  struct iwreq		wrq;
  char			essid[IW_ESSID_MAX_SIZE + 1];	/* ESSID */
  char			pessid[IW_ESSID_MAX_SIZE + 1];	/* Pcmcia format */
  int		i;
  int		j;

  /* Get ESSID */
  strcpy(wrq.ifr_name, ifname);
  wrq.u.essid.pointer = (caddr_t) essid;
  wrq.u.essid.length = 0;
  wrq.u.essid.flags = 0;
  if(ioctl(skfd, SIOCGIWESSID, &wrq) < 0)
    return(-1);

  switch(format)
    {
    case FORMAT_SCHEME:
      /* Stip all white space and stuff */
      j = 0;
      for(i = 0; i < strlen(essid); i++)
	if(isalnum(essid[i]))
	  pessid[j++] = essid[i];
      pessid[j] = '\0';
      if((j == 0) || (j > 32))
	return(-2);
      printf("%s\n", pessid);
      fflush(stdout);
      break;
    default:
      printf("%-8.8s  ESSID:\"%s\"\n", ifname, essid);
      break;
    }

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Display the NWID if possible
 */
static int
print_nwid(int		skfd,
	   char *	ifname,
	   int		format)
{
  struct iwreq		wrq;

  /* Get network ID */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWNWID, &wrq) < 0)
    return(-1);

  switch(format)
    {
    case FORMAT_SCHEME:
      /* Prefix with nwid to avoid name space collisions */
      printf("nwid%X\n", wrq.u.nwid.value);
      fflush(stdout);
      break;
    default:
      printf("%-8.8s  NWID:%X\n", ifname, wrq.u.nwid.value);
      break;
    }

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Try the various devices until one return something we can use
 */
static int
scan_devices(int		skfd,
	     int		format)
{
  char		buff[1024];
  struct ifconf ifc;
  struct ifreq *ifr;
  int		i;
  int		ret;

  /* Get list of active devices */
  ifc.ifc_len = sizeof(buff);
  ifc.ifc_buf = buff;
  if(ioctl(skfd, SIOCGIFCONF, &ifc) < 0)
    {
      fprintf(stderr, "SIOCGIFCONF: %s\n", strerror(errno));
      return(-1);
    }
  ifr = ifc.ifc_req;

  /* Print the first match */
  for(i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; ifr++)
    {
      /* Try to print an ESSID */
      ret = print_essid(skfd, ifr->ifr_name, format);
      if(ret == 0)
	return(0);	/* Success */

      /* Try to print a nwid */
      ret = print_nwid(skfd, ifr->ifr_name, format);
      if(ret == 0)
	return(0);	/* Success */
    }
  return(-1);
}

/*************************** SUBROUTINES ***************************/

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

/**************************** AP ADDRESS ****************************/

/*------------------------------------------------------------------*/
/*
 * Display the NWID if possible
 */
static int
print_ap(int		skfd,
	 char *		ifname,
	 int		format)
{
  struct iwreq		wrq;

  /* Get network ID */
  strcpy(wrq.ifr_name, ifname);
  if(ioctl(skfd, SIOCGIWAP, &wrq) < 0)
    return(-1);

  /* Print */
  printf("%-8.8s  Access Point: %s\n", ifname,
	 pr_ether(wrq.u.ap_addr.sa_data));

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Try the various devices until one return something we can use
 */
static inline int
scan_ap(int		skfd,
	int		format)
{
  char		buff[1024];
  struct ifconf ifc;
  struct ifreq *ifr;
  int		i;
  int		ret;

  /* Get list of active devices */
  ifc.ifc_len = sizeof(buff);
  ifc.ifc_buf = buff;
  if(ioctl(skfd, SIOCGIFCONF, &ifc) < 0)
    {
      fprintf(stderr, "SIOCGIFCONF: %s\n", strerror(errno));
      return(-1);
    }
  ifr = ifc.ifc_req;

  /* Print the first match */
  for(i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; ifr++)
    {
      /* Try to print an ESSID */
      ret = print_ap(skfd, ifr->ifr_name, format);
      if(ret == 0)
	return(0);	/* Success */
    }
  return(-1);
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
  int	skfd = -1;		/* generic raw socket desc.	*/
  int	format = FORMAT_DEFAULT;
  int	ret = -1;

  /* Create a channel to the NET kernel. */
  if((skfd = sockets_open()) < 0)
    {
      perror("socket");
      return(-1);
    }

  /* No argument */
  if(argc == 1)
    {
      /* Look on all devices */
      ret = scan_devices(skfd, format);
      close(skfd);
      return(ret);
    }

  /* Only ask for first AP address */
  if((!strcmp(argv[1], "--ap")) || (!strcmp(argv[1], "-a")))
    {
      /* Look on all devices */
      ret = scan_ap(skfd, format);
      close(skfd);
      return(ret);
    }

  /* Only the format, no interface name */
  if((!strncmp(argv[1], "--scheme", 4)) || (!strcmp(argv[1], "-s")))
    {
      /* Look on all devices */
      format = FORMAT_SCHEME;
      ret = scan_devices(skfd, format);
      close(skfd);
      return(ret);
    }

  /* Help */
  if((argc > 3) ||
     (!strncmp(argv[1], "-h", 9)) || (!strcmp(argv[1], "--help")))
    {
      fprintf(stderr, "Usage: iwgetid [interface]\n");
      fprintf(stderr, "               [interface] --scheme\n");
      return(-1);
    }

  /* If at least a device name */
  if(argc > 1)
    {
      /* Check extra format argument */
      if(argc > 2)
	{
	  /* Only ask for first AP address */
	  if((!strcmp(argv[2], "--ap")) || (!strcmp(argv[2], "-a")))
	    {
	      ret = print_ap(skfd, argv[1], format);
	      close(skfd);
	      return(ret);
	    }

	  /* Want scheme format */
	  if((!strncmp(argv[2], "--scheme", 4)) || (!strcmp(argv[2], "-s")))
	    format = FORMAT_SCHEME;
	}

      /* Try to print an ESSID */
      ret = print_essid(skfd, argv[1], format);

      if(ret == -1)
	{
	  /* Try to print a nwid */
	  ret = print_nwid(skfd, argv[1], format);
	}
    }

  /* Close the socket. */
  close(skfd);

  return(ret);
}
