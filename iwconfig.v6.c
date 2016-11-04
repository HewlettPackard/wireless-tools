/*
 * Hack my way... Jean II
 *
 * ifconfig	This file contains an implementation of the command
 *		that either displays or sets the characteristics of
 *		one or more of the system's networking interfaces.
 *
 * Usage:	ifconfig [-a] [-i] [-v] interface
 *			[inet address]
 *			[hw ether|ax25|ppp address]
 *			[metric NN] [mtu NN]
 *			[trailers] [-trailers]
 *			[arp] [-arp]
 *			[netmask aa.bb.cc.dd]
 *			[dstaddr aa.bb.cc.dd]
 *			[mem_start NN] [io_addr NN] [irq NN]
 *			[[-] broadcast [aa.bb.cc.dd]]
 *			[[-]pointopoint [aa.bb.cc.dd]]
 *			[up] [down] ...
 *
 * Version:	@(#)ifconfig.c	2.30	10/07/93
 *
 * Author:	Fred N. van Kempen, <waltje@uwalt.nl.mugnet.org>
 *		Copyright 1993 MicroWalt Corporation
 *
 *		This program is free software; you can redistribute it
 *		and/or  modify it under  the terms of  the GNU General
 *		Public  License as  published  by  the  Free  Software
 *		Foundation;  either  version 2 of the License, or  (at
 *		your option) any later version.
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
};

struct interface {
  char			name[IFNAMSIZ];		/* interface name	 */
  short			type;			/* if type		 */
  short			flags;			/* various flags	 */
  int			metric;			/* routing metric	 */
  int			mtu;			/* MTU value		 */
  struct ifmap		map;			/* hardware setup	 */
  struct sockaddr	addr;			/* IP address		 */
  struct sockaddr	dstaddr;		/* P-P IP address	 */
  struct sockaddr	broadaddr;		/* IP broadcast address	 */
  struct sockaddr	netmask;		/* IP network mask	 */
  struct sockaddr	ipxaddr_bb;		/* IPX network address   */
  struct sockaddr	ipxaddr_sn;		/* IPX network address   */
  struct sockaddr	ipxaddr_e3;		/* IPX network address   */
  struct sockaddr	ipxaddr_e2;		/* IPX network address   */
  struct sockaddr	ddpaddr;		/* Appletalk DDP address */
  int			has_ip;
  int			has_ipx_bb;
  int			has_ipx_sn;
  int			has_ipx_e3;
  int			has_ipx_e2;
  int			has_ax25;
  int			has_ddp;
  char			hwaddr[32];		/* HW address		 */
  struct enet_statistics stats;			/* statistics		 */
};

  
int skfd = -1;				/* generic raw socket desc.	*/
int ipx_sock = -1;			/* IPX socket			*/
int ax25_sock = -1;			/* AX.25 socket			*/
int inet_sock = -1;			/* INET socket			*/
int ddp_sock = -1;			/* Appletalk DDP socket		*/


static void
usage(void)
{
  fprintf(stderr, "Usage: ifconfig [-a] [-i] [-v] interface\n");
  fprintf(stderr, "                [inet address]\n");
  fprintf(stderr, "                [hw] [ax25 address]\n");
  fprintf(stderr, "                [metric NN] [mtu NN]\n");
  fprintf(stderr, "                [trailers] [-trailers]\n");
  fprintf(stderr, "                [arp] [-arp]\n");
  fprintf(stderr, "                [netmask aa.bb.cc.dd]\n");
  fprintf(stderr, "                [dstaddr aa.bb.cc.dd]\n");
  fprintf(stderr, "                [mem_start NN] [io_addr NN] [irq NN]\n");
  fprintf(stderr, "                [[-] broadcast [aa.bb.cc.dd]]\n");
  fprintf(stderr, "                [[-]pointopoint [aa.bb.cc.dd]]\n");
  fprintf(stderr, "                [up] [down] ...\n");
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

  printf("\n\n");
}

/*
static void if_getstats(char *ifname, struct interface *ife)
{
  FILE *f=fopen("/proc/net/dev","r");
  char buf[256];
  char *bp;
  if(f==NULL)
  	return;
  while(fgets(buf,255,f))
  {
  	bp=buf;
  	while(*bp&&isspace(*bp))
  		bp++;
  	if(strncmp(bp,ifname,strlen(ifname))==0 && bp[strlen(ifname)]==':')
  	{
 		bp=strchr(bp,':');
 		bp++;
 		sscanf(bp,"%d %d %d %d %d %d %d %d %d %d %d",
 			&ife->stats.rx_packets,
 			&ife->stats.rx_errors,
 			&ife->stats.rx_dropped,
 			&ife->stats.rx_fifo_errors,
 			&ife->stats.rx_frame_errors,
 			
 			&ife->stats.tx_packets,
 			&ife->stats.tx_errors,
 			&ife->stats.tx_dropped,
 			&ife->stats.tx_fifo_errors,
 			&ife->stats.collisions,
 			
 			&ife->stats.tx_carrier_errors
 		);
 		fclose(f);
 		return;
  	}
  }
  fclose(f);
}
*/

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
	  if((++i >= count) ||
	     (sscanf(args[i], "%g", &(wrq.u.freq)) != 1))
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
	fprintf(stderr, "%s: no wireless extensions.\n",
		ifname);
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
