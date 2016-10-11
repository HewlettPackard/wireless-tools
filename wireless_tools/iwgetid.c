/*
 *	Wireless Tools
 *
 *		Jean II - HPL '01
 *
 * Just print the ESSID or NWID...
 *
 * This file is released under the GPL license.
 *     Copyright (c) 1997-2002 Jean Tourrilhes <jt@hpl.hp.com>
 */

#include "iwlib.h"		/* Header */

#include <getopt.h>

#define FORMAT_DEFAULT	0	/* Nice looking display for the user */
#define FORMAT_SCHEME	1	/* To be used as a Pcmcia Scheme */
#define WTYPE_ESSID	0	/* Display ESSID or NWID */
#define WTYPE_AP	1	/* Display AP/Cell Address */
#define WTYPE_FREQ	2	/* Display frequency/channel */
#define WTYPE_MODE	3	/* Display mode */
#define WTYPE_PROTO	4	/* Display protocol name */

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

/*************************** SUBROUTINES ***************************/
/*
 * Just for the heck of it, let's try to not link with iwlib.
 * This will keep the binary small and tiny...
 *
 * Note : maybe it's time to admit that we have lost the battle
 * and we start using iwlib ? Maybe we should default to dynamic
 * lib first...
 */

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

/*------------------------------------------------------------------*/
/*
 * Display an Ethernet address in readable format.
 */
void
iw_ether_ntop(const struct ether_addr* eth, char* buf)
{
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
	  eth->ether_addr_octet[0], eth->ether_addr_octet[1],
	  eth->ether_addr_octet[2], eth->ether_addr_octet[3],
	  eth->ether_addr_octet[4], eth->ether_addr_octet[5]);
}

/*------------------------------------------------------------------*/
/*
 * Convert our internal representation of frequencies to a floating point.
 */
double
iw_freq2float(iwfreq *	in)
{
#ifdef WE_NOLIBM
  /* Version without libm : slower */
  int		i;
  double	res = (double) in->m;
  for(i = 0; i < in->e; i++)
    res *= 10;
  return(res);
#else	/* WE_NOLIBM */
  /* Version with libm : faster */
  return ((double) in->m) * pow(10,in->e);
#endif	/* WE_NOLIBM */
}

/*------------------------------------------------------------------*/
/*
 * Output a frequency with proper scaling
 */
void
iw_print_freq(char *	buffer,
	      double	freq)
{
  if(freq < KILO)
    sprintf(buffer, "Channel:%g", freq);
  else
    {
      if(freq >= GIGA)
	sprintf(buffer, "Frequency:%gGHz", freq / GIGA);
      else
	{
	  if(freq >= MEGA)
	    sprintf(buffer, "Frequency:%gMHz", freq / MEGA);
	  else
	    sprintf(buffer, "Frequency:%gkHz", freq / KILO);
	}
    }
}

/*------------------------------------------------------------------*/
const char * const iw_operation_mode[] = { "Auto",
					"Ad-Hoc",
					"Managed",
					"Master",
					"Repeater",
					"Secondary",
					"Monitor" };

/************************ DISPLAY ESSID/NWID ************************/

/*------------------------------------------------------------------*/
/*
 * Display the ESSID if possible
 */
static int
print_essid(int			skfd,
	    const char *	ifname,
	    int			format)
{
  struct iwreq		wrq;
  char			essid[IW_ESSID_MAX_SIZE + 1];	/* ESSID */
  char			pessid[IW_ESSID_MAX_SIZE + 1];	/* Pcmcia format */
  unsigned int		i;
  unsigned int		j;

  /* Get ESSID */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  wrq.u.essid.pointer = (caddr_t) essid;
  wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
  wrq.u.essid.flags = 0;
  if(ioctl(skfd, SIOCGIWESSID, &wrq) < 0)
    return(-1);

  switch(format)
    {
    case FORMAT_SCHEME:
      /* Strip all white space and stuff */
      j = 0;
      for(i = 0; i < strlen(essid); i++)
	if(isalnum(essid[i]))
	  pessid[j++] = essid[i];
      pessid[j] = '\0';
      if((j == 0) || (j > 32))
	return(-2);
      printf("%s\n", pessid);
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
	   const char *	ifname,
	   int		format)
{
  struct iwreq		wrq;

  /* Get network ID */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  if(ioctl(skfd, SIOCGIWNWID, &wrq) < 0)
    return(-1);

  switch(format)
    {
    case FORMAT_SCHEME:
      /* Prefix with nwid to avoid name space collisions */
      printf("nwid%X\n", wrq.u.nwid.value);
      break;
    default:
      printf("%-8.8s  NWID:%X\n", ifname, wrq.u.nwid.value);
      break;
    }

  return(0);
}

/**************************** AP ADDRESS ****************************/

/*------------------------------------------------------------------*/
/*
 * Display the AP Address if possible
 */
static int
print_ap(int		skfd,
	 const char *	ifname,
	 int		format)
{
  struct iwreq		wrq;
  char			buffer[64];

  /* Get AP Address */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  if(ioctl(skfd, SIOCGIWAP, &wrq) < 0)
    return(-1);

  /* Print */
  iw_ether_ntop((const struct ether_addr *) wrq.u.ap_addr.sa_data, buffer);
  switch(format)
    {
    case FORMAT_SCHEME:
      /* I think ':' are not problematic, because Pcmcia scripts
       * seem to handle them properly... */
      printf("%s\n", buffer);
      break;
    default:
      printf("%-8.8s  Access Point/Cell: %s\n", ifname, buffer);
      break;
    }

  return(0);
}

/****************************** OTHER ******************************/

/*------------------------------------------------------------------*/
/*
 * Display the frequency (or channel) if possible
 */
static int
print_freq(int		skfd,
	   const char *	ifname,
	   int		format)
{
  struct iwreq		wrq;
  double		freq;
  char			buffer[64];

  /* Get frequency / channel */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  if(ioctl(skfd, SIOCGIWFREQ, &wrq) < 0)
    return(-1);

  /* Print */
  freq = iw_freq2float(&(wrq.u.freq));
  switch(format)
    {
    case FORMAT_SCHEME:
      printf("%g\n", freq);
      break;
    default:
      iw_print_freq(buffer, freq);
      printf("%-8.8s  %s\n", ifname, buffer);
      break;
    }

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Display the mode if possible
 */
static int
print_mode(int		skfd,
	   const char *	ifname,
	   int		format)
{
  struct iwreq		wrq;

  /* Get frequency / channel */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  if(ioctl(skfd, SIOCGIWMODE, &wrq) < 0)
    return(-1);
  if(wrq.u.mode >= IW_NUM_OPER_MODE)
    return(-2);

  /* Print */
  switch(format)
    {
    case FORMAT_SCHEME:
      printf("%d\n", wrq.u.mode);
      break;
    default:
      printf("%-8.8s  Mode:%s\n", ifname, iw_operation_mode[wrq.u.mode]);
      break;
    }

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Display the ESSID if possible
 */
static int
print_protocol(int		skfd,
	       const char *	ifname,
	       int		format)
{
  struct iwreq		wrq;
  char			proto[IFNAMSIZ + 1];	/* Protocol */
  char			pproto[IFNAMSIZ + 1];	/* Pcmcia format */
  unsigned int		i;
  unsigned int		j;

  /* Get Protocol name */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  if(ioctl(skfd, SIOCGIWNAME, &wrq) < 0)
    return(-1);
  strncpy(proto, wrq.u.name, IFNAMSIZ);
  proto[IFNAMSIZ] = '\0';

  switch(format)
    {
    case FORMAT_SCHEME:
      /* Strip all white space and stuff */
      j = 0;
      for(i = 0; i < strlen(proto); i++)
	if(isalnum(proto[i]))
	  pproto[j++] = proto[i];
      pproto[j] = '\0';
      if((j == 0) || (j > 32))
	return(-2);
      printf("%s\n", pproto);
      break;
    default:
      printf("%-8.8s  Protocol Name:\"%s\"\n", ifname, proto);
      break;
    }

  return(0);
}

/******************************* MAIN ********************************/

/*------------------------------------------------------------------*/
/*
 * Check options and call the proper handler
 */
static int
print_one_device(int		skfd,
		 int		format,
		 int		wtype,
		 const char*	ifname)
{
  int ret;

  /* Check wtype */
  switch(wtype)
    {
    case WTYPE_AP:
      /* Try to print an AP */
      ret = print_ap(skfd, ifname, format);
      break;

    case WTYPE_FREQ:
      /* Try to print frequency */
      ret = print_freq(skfd, ifname, format);
      break;

    case WTYPE_MODE:
      /* Try to print the mode */
      ret = print_mode(skfd, ifname, format);
      break;

    case WTYPE_PROTO:
      /* Try to print the protocol */
      ret = print_protocol(skfd, ifname, format);
      break;

    default:
      /* Try to print an ESSID */
      ret = print_essid(skfd, ifname, format);
      if(ret < 0)
	{
	  /* Try to print a nwid */
	  ret = print_nwid(skfd, ifname, format);
	}
    }

  return(ret);
}

/*------------------------------------------------------------------*/
/*
 * Try the various devices until one return something we can use
 */
static int
scan_devices(int		skfd,
	     int		format,
	     int		wtype)
{
  char		buff[1024];
  struct ifconf ifc;
  struct ifreq *ifr;
  int		i;

  /* Get list of active devices */
  ifc.ifc_len = sizeof(buff);
  ifc.ifc_buf = buff;
  if(ioctl(skfd, SIOCGIFCONF, &ifc) < 0)
    {
      perror("SIOCGIFCONF");
      return(-1);
    }
  ifr = ifc.ifc_req;

  /* Print the first match */
  for(i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; ifr++)
    {
      if(print_one_device(skfd, format, wtype, ifr->ifr_name) >= 0)
	return 0;
    }
  return(-1);
}

/*------------------------------------------------------------------*/
/*
 * helper
 */
static void
iw_usage(int status)
{
  fputs("Usage iwgetid [OPTIONS] [ifname]\n"
	"  Options are:\n"
	"    -a,--ap       Print the access point address\n"
	"    -f,--freq     Print the current frequency\n"
	"    -m,--mode     Print the current mode\n"
	"    -p,--protocol Print the protocol name\n"
	"    -s,--scheme   Format the output as a PCMCIA scheme identifier\n"
	"    -h,--help     Print this message\n",
	status ? stderr : stdout);
  exit(status);
}

static const struct option long_opts[] = {
  { "ap", no_argument, NULL, 'a' },
  { "freq", no_argument, NULL, 'f' },
  { "mode", no_argument, NULL, 'm' },
  { "protocol", no_argument, NULL, 'p' },
  { "help", no_argument, NULL, 'h' },
  { "scheme", no_argument, NULL, 's' },
  { NULL, 0, NULL, 0 }
};

/*------------------------------------------------------------------*/
/*
 * The main !
 */
int
main(int	argc,
     char **	argv)
{
  int	skfd;			/* generic raw socket desc.	*/
  int	format = FORMAT_DEFAULT;
  int	wtype = WTYPE_ESSID;
  int	opt;
  int	ret = -1;

  /* Check command line arguments */
  while((opt = getopt_long(argc, argv, "afhmps", long_opts, NULL)) > 0)
    {
      switch(opt)
	{
	case 'a':
	  /* User wants AP/Cell Address */
	  wtype = WTYPE_AP;
	  break;

	case 'f':
	  /* User wants frequency/channel */
	  wtype = WTYPE_FREQ;
	  break;

	case 'm':
	  /* User wants the mode */
	  wtype = WTYPE_MODE;
	  break;

	case 'p':
	  /* User wants the protocol */
	  wtype = WTYPE_PROTO;
	  break;

	case 'h':
	  iw_usage(0);
	  break;

	case 's':
	  /* User wants a Scheme format */
	  format = FORMAT_SCHEME;
	  break;

	default:
	  iw_usage(1);
	  break;
	}
    }
  if(optind + 1 < argc) {
    fputs("Too many arguments.\n", stderr);
    iw_usage(1);
  }

  /* Create a channel to the NET kernel. */
  if((skfd = iw_sockets_open()) < 0)
    {
      perror("socket");
      return(-1);
    }

  /* Check if first argument is a device name */
  if(optind < argc)
    {
      /* Yes : query only this device */
      ret = print_one_device(skfd, format, wtype, argv[optind]);
    }
  else
    {
      /* No : query all devices and print first found */
      ret = scan_devices(skfd, format, wtype);
    }

  fflush(stdout);
  close(skfd);
  return(ret);
}
