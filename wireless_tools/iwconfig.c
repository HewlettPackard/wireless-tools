/*
 * Hack my way... Jean II
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/ipx.h>
#include <linux/wireless.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Some usefull constants */
#define KILO	1e3
#define MEGA	1e6
#define GIGA	1e9

typedef struct iw_statistics	iwstats;
typedef struct iw_range		iwrange;

/* Structure for storing all wireless information for each device */
struct wireless_info
{
  char		dev[IFNAMSIZ];		/* Interface name (device) */
  char		name[12];		/* Wireless name */
  int		has_nwid;
  int		nwid_on;
  u_long	nwid;			/* Network ID */
  int		has_freq;
  u_long	freq;			/* Frequency/channel */

  /* Stats */
  iwstats	stats;
  int		has_stats;
  iwrange	range;
  int		has_range;
};

int skfd = -1;				/* generic raw socket desc.	*/
int ipx_sock = -1;			/* IPX socket			*/
int ax25_sock = -1;			/* AX.25 socket			*/
int inet_sock = -1;			/* INET socket			*/
int ddp_sock = -1;			/* Appletalk DDP socket		*/


static void
usage(void)
{
  fprintf(stderr, "Usage: iwconfig interface\n");
  fprintf(stderr, "                [nwid NN]\n");
  fprintf(stderr, "                [freq N.NN (add k, M or G) ]\n");
  fprintf(stderr, "                [channel N]\n");
  exit(1);
}


static void
print_info(struct wireless_info *	info)
{
  /* Display device name */
  printf("%-8.8s  ", info->dev);

  /* Display wireless name */
  printf("%s  ", info->name);

  /* Display Network ID */
  if(info->has_nwid)
    if(info->nwid_on)
      printf("NWID:%lX  ", info->nwid);
    else
      printf("NWID:off  ");

  /* Display frequency / channel */
  if(info->has_freq)
    if(info->freq < KILO)
      printf("Channel:%g  ", (double) info->freq);
    else
      if(info->freq > GIGA)
	printf("Frequency:%gGHz  ", info->freq / GIGA);
      else
	if(info->freq > MEGA)
	  printf("Frequency:%gMHz  ", info->freq / MEGA);
	else
	  printf("Frequency:%gkHz  ", info->freq / KILO);

  printf("\n");

  if(info->has_stats)
    {
      if(info->has_range)
	printf("          Link quality:%d/%d  Signal level:%d/%d  Noise level:%d/%d\n",
	       info->stats.qual.qual, info->range.max_qual.qual,
	       info->stats.qual.level, info->range.max_qual.level,
	       info->stats.qual.noise, info->range.max_qual.noise);
      else
	printf("          Link quality:%d  Signal level:%d  Noise level:%d\n",
	       info->stats.qual.qual,
	       info->stats.qual.level,
	       info->stats.qual.noise);

      printf("          Rx invalid nwid:%d  invalid crypt:%d  invalid misc:%d\n",
	     info->stats.discard.nwid,
	     info->stats.discard.crypt,
	     info->stats.discard.misc);
    }

  printf("\n");
}

static int if_getstats(char *ifname, iwstats *	stats)
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
 		sscanf(bp, "%X", &stats->status);
		bp = strtok(NULL, " .");
 		sscanf(bp, "%d", &stats->qual.qual);
		bp = strtok(NULL, " .");
		sscanf(bp, "%d", &stats->qual.level);
		bp = strtok(NULL, " .");
		sscanf(bp, "%d", &stats->qual.noise);
		bp = strtok(NULL, " .");
		sscanf(bp, "%d", &stats->discard.nwid);
		bp = strtok(NULL, " .");
		sscanf(bp, "%d", &stats->discard.crypt);
		bp = strtok(NULL, " .");
		sscanf(bp, "%d", &stats->discard.misc);
 		fclose(f);
 		return;
  	}
  }
  fclose(f);
  return 0;
}

/* Get wireless informations & config from the device driver */
static int
get_info(char *			ifname,
	 struct wireless_info *	info)
{
  struct iwreq		wrq;

  memset((char *) info, 0, sizeof(struct wireless_info));

  /* Get device name */
  strcpy(info->dev, ifname);

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
      info->freq = wrq.u.freq;
    }

  /* Get stats */
  if(if_getstats(ifname, &(info->stats)) == 0)
    {
      info->has_stats = 1;
    }

  /* Get ranges */
  strcpy(wrq.ifr_name, ifname);
  wrq.u.data.pointer = (caddr_t) &(info->range);
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCGIWRANGE, &wrq) >= 0)
    {
      info->has_range = 1;
    }

  return(0);
}


static int
set_info(char *		args[],		/* Command line args */
	 int		count,		/* Args count */
	 char *		ifname)		/* Dev name */
{
  struct iwreq		wrq;
  int			i;

  /* Set dev name */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

  /* The other args on the line specify options to be set... */
  for(i = 0; i < count; i++)
    {
      /* Set network ID */
      if(!strcmp(args[i], "nwid"))
	{
	  if(++i >= count)
	    usage();
	  if(!strcasecmp(args[i], "off"))
	    wrq.u.nwid.on = 0;
	  else
	    if(sscanf(args[i], "%lX", &(wrq.u.nwid.nwid)) != 1)
	      usage();
	    else
	      wrq.u.nwid.on = 1;

	  if(ioctl(skfd, SIOCSIWNWID, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWNWID: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* Set frequency / channel */
      if((!strncmp(args[i], "freq", 4)) ||
	 (!strcmp(args[i], "channel")))
	{
	  if(++i >= count)
	    {
	      struct iw_range	range;
	      int		k;

	      wrq.u.data.pointer = (caddr_t) &range;
	      wrq.u.data.length = 0;
	      wrq.u.data.flags = 0;
	      if(ioctl(skfd, SIOCGIWRANGE, &wrq) < 0)
		{
		  fprintf(stderr, "SIOCGIWRANGE: %s\n", strerror(errno));
		  return(-1);
		}

	      printf("%d channels ; available frequencies :",
		     range.num_channels);
	      for(k = 0; k < range.num_frequency; k++)
		if(range.freq[k] > GIGA)
		  printf(" %gGHz ", range.freq[k] / GIGA);
		else
		  if(range.freq[k] > MEGA)
		    printf(" %gMHz ", range.freq[k] / MEGA);
		  else
		    printf(" %gkHz ", range.freq[k] / KILO);
	      printf("\n");
	      return;	/* no more arg */
	    }

	  if(sscanf(args[i], "%g", &(wrq.u.freq)) != 1)
	    usage();
	  if(index(args[i], 'G')) wrq.u.freq *= GIGA;
	  if(index(args[i], 'M')) wrq.u.freq *= MEGA;
	  if(index(args[i], 'k')) wrq.u.freq *= KILO;

	  if(ioctl(skfd, SIOCSIWFREQ, &wrq) < 0)
	    {
	      fprintf(stderr, "SIOCSIWFREQ: %s\n", strerror(errno));
	      return(-1);
	    }
	  continue;
	}

      /* Here we have an unrecognised arg... */
      fprintf(stderr, "Invalid argument : %s\n", args[i]);
      usage();
      return(-1);
    }		/* for(index ... */
  return(0);
}

static void
print_devices(char *ifname)
{
  char buff[1024];
  struct wireless_info	info;
  struct ifconf ifc;
  struct ifreq *ifr;
  int i;

  if(ifname == (char *)NULL)
    {
      ifc.ifc_len = sizeof(buff);
      ifc.ifc_buf = buff;
      if(ioctl(skfd, SIOCGIFCONF, &ifc) < 0)
	{
	  fprintf(stderr, "SIOCGIFCONF: %s\n", strerror(errno));
	  return;
	}

      ifr = ifc.ifc_req;
      for(i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; ifr++)
	{
	  if(get_info(ifr->ifr_name, &info) < 0)
	    {
	      /* Could skip this message ? */
	      fprintf(stderr, "%-8.8s  no wireless extensions.\n\n",
		      ifr->ifr_name);
	      continue;
	    }

	  print_info(&info);
	}
    }
  else
    {
      if(get_info(ifname, &info) < 0)
	{
	  fprintf(stderr, "%s: no wireless extensions.\n",
		  ifname);
	  usage();
	}
      else
	print_info(&info);
    }
}


static int sockets_open()
{
	inet_sock=socket(AF_INET, SOCK_DGRAM, 0);
	ipx_sock=socket(AF_IPX, SOCK_DGRAM, 0);
	ax25_sock=socket(AF_AX25, SOCK_DGRAM, 0);
	ddp_sock=socket(AF_APPLETALK, SOCK_DGRAM, 0);
	/*
	 *	Now pick any (exisiting) useful socket family for generic queries
	 */
	if(inet_sock!=-1)
		return inet_sock;
	if(ipx_sock!=-1)
		return ipx_sock;
	if(ax25_sock!=-1)
		return ax25_sock;
	/*
	 *	If this is -1 we have no known network layers and its time to jump.
	 */
	 
	return ddp_sock;
}
	
int
main(int	argc,
     char **	argv)
{
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
      print_devices((char *)NULL);
      close(skfd);
      exit(0);
    }

  /* The device name must be the first argument */
  if(argc == 2)
    {
      print_devices(argv[1]);
      close(skfd);
      exit(0);
    }

  /* The other args on the line specify options to be set... */
  goterr = set_info(argv + 2, argc - 2, argv[1]);

  /* Close the socket. */
  close(skfd);

  return(goterr);
}
