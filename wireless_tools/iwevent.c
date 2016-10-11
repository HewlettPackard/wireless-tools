/*
 *	Wireless Tools
 *
 *		Jean II - HPL 99->01
 *
 * Main code for "iwevent". This listent for wireless events on rtnetlink.
 * You need to link this code against "iwcommon.c" and "-lm".
 *
 * Part of this code is from Alexey Kuznetsov, part is from Casey Carter,
 * I've just put the pieces together...
 * By the way, if you know a way to remove the root restrictions, tell me
 * about it...
 *
 * This file is released under the GPL license.
 *     Copyright (c) 1997-2002 Jean Tourrilhes <jt@hpl.hp.com>
 */

/***************************** INCLUDES *****************************/

#include "iwlib.h"		/* Header */

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <getopt.h>
#include <time.h>
#include <sys/time.h>

/* Ugly backward compatibility :-( */
#ifndef IFLA_WIRELESS
#define IFLA_WIRELESS	(IFLA_MASTER + 1)
#endif /* IFLA_WIRELESS */

/************************ RTNETLINK HELPERS ************************/
/*
 * The following code is extracted from :
 * ----------------------------------------------
 * libnetlink.c	RTnetlink service routines.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 * -----------------------------------------------
 */

struct rtnl_handle
{
	int			fd;
	struct sockaddr_nl	local;
	struct sockaddr_nl	peer;
	__u32			seq;
	__u32			dump;
};

static inline void rtnl_close(struct rtnl_handle *rth)
{
	close(rth->fd);
}

static inline int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions)
{
	int addr_len;

	memset(rth, 0, sizeof(rth));

	rth->fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (rth->fd < 0) {
		perror("Cannot open netlink socket");
		return -1;
	}

	memset(&rth->local, 0, sizeof(rth->local));
	rth->local.nl_family = AF_NETLINK;
	rth->local.nl_groups = subscriptions;

	if (bind(rth->fd, (struct sockaddr*)&rth->local, sizeof(rth->local)) < 0) {
		perror("Cannot bind netlink socket");
		return -1;
	}
	addr_len = sizeof(rth->local);
	if (getsockname(rth->fd, (struct sockaddr*)&rth->local, &addr_len) < 0) {
		perror("Cannot getsockname");
		return -1;
	}
	if (addr_len != sizeof(rth->local)) {
		fprintf(stderr, "Wrong address length %d\n", addr_len);
		return -1;
	}
	if (rth->local.nl_family != AF_NETLINK) {
		fprintf(stderr, "Wrong address family %d\n", rth->local.nl_family);
		return -1;
	}
	rth->seq = time(NULL);
	return 0;
}

/********************* WIRELESS EVENT DECODING *********************/
/*
 * This is the bit I wrote...
 */

#if WIRELESS_EXT > 13
/*------------------------------------------------------------------*/
/*
 * Print one element from the scanning results
 */
static inline int
print_event_token(struct iw_event *	event,	/* Extracted token */
		  char *		ifname,
		  struct iw_range *	iwrange,	/* Range info */
		  int			has_iwrange)
{
  char		buffer[128];	/* Temporary buffer */

  /* Now, let's decode the event */
  switch(event->cmd)
    {
      /* ----- set events ----- */
      /* Events that result from a "SET XXX" operation by the user */
    case SIOCSIWNWID:
      if(event->u.nwid.disabled)
	printf("NWID:off/any\n");
      else
	printf("NWID:%X\n", event->u.nwid.value);
      break;
    case SIOCSIWFREQ:
      {
	float		freq;			/* Frequency/channel */
	freq = iw_freq2float(&(event->u.freq));
	iw_print_freq(buffer, freq);
	printf("%s\n", buffer);
      }
      break;
    case SIOCSIWMODE:
      printf("Mode:%s\n",
	     iw_operation_mode[event->u.mode]);
      break;
    case SIOCSIWESSID:
      {
	char essid[IW_ESSID_MAX_SIZE+1];
	if((event->u.essid.pointer) && (event->u.essid.length))
	  memcpy(essid, event->u.essid.pointer, event->u.essid.length);
	essid[event->u.essid.length] = '\0';
	if(event->u.essid.flags)
	  {
	    /* Does it have an ESSID index ? */
	    if((event->u.essid.flags & IW_ENCODE_INDEX) > 1)
	      printf("ESSID:\"%s\" [%d]\n", essid,
		     (event->u.essid.flags & IW_ENCODE_INDEX));
	    else
	      printf("ESSID:\"%s\"\n", essid);
	  }
	else
	  printf("ESSID:off/any\n");
      }
      break;
    case SIOCSIWENCODE:
      {
	unsigned char	key[IW_ENCODING_TOKEN_MAX];
	if(event->u.data.pointer)
	  memcpy(key, event->u.essid.pointer, event->u.data.length);
	else
	  event->u.data.flags |= IW_ENCODE_NOKEY;
	printf("Encryption key:");
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
      /* ----- driver events ----- */
      /* Events generated by the driver when something important happens */
    case SIOCGIWAP:
      printf("New Access Point/Cell address:%s\n",
	     iw_pr_ether(buffer, event->u.ap_addr.sa_data));
      break;
    case SIOCGIWSCAN:
      printf("Scan request completed\n");
      break;
    case IWEVTXDROP:
      printf("Tx packet dropped:%s\n",
	     iw_pr_ether(buffer, event->u.addr.sa_data));
      break;
#if WIRELESS_EXT > 14
    case IWEVCUSTOM:
      {
	char custom[IW_CUSTOM_MAX+1];
	if((event->u.data.pointer) && (event->u.data.length))
	  memcpy(custom, event->u.data.pointer, event->u.data.length);
	custom[event->u.data.length] = '\0';
	printf("Custom driver event:%s\n", custom);
      }
      break;
    case IWEVREGISTERED:
      printf("Registered node:%s\n",
	     iw_pr_ether(buffer, event->u.addr.sa_data));
      break;
    case IWEVEXPIRED:
      printf("Expired node:%s\n",
	     iw_pr_ether(buffer, event->u.addr.sa_data));
      break;
#endif /* WIRELESS_EXT > 14 */
#if WIRELESS_EXT > 15
    case SIOCGIWTHRSPY:
      {
	struct iw_thrspy	threshold;
	int			skfd;
	struct iw_range		range;
	int			has_range = 0;
	if((event->u.data.pointer) && (event->u.data.length))
	  {
	    memcpy(&threshold, event->u.data.pointer,
		   sizeof(struct iw_thrspy));
	    if((skfd = iw_sockets_open()) >= 0)
	      {
		has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);
		close(skfd);
	      }
	    printf("Spy threshold crossed on address:%s\n",
		   iw_pr_ether(buffer, threshold.addr.sa_data));
	    threshold.qual.updated = 0x0;	/* Not that reliable, disable */
	    iw_print_stats(buffer, &threshold.qual, &range, has_range);
	    printf("                            Link %s\n", buffer);
	  }
	else
	  printf("Invalid Spy Threshold event\n");
      }
      break;
#else
      /* Avoid "Unused parameter" warning */
      ifname = ifname;
#endif /* WIRELESS_EXT > 15 */
      /* ----- junk ----- */
      /* other junk not currently in use */
    case SIOCGIWRATE:
      iw_print_bitrate(buffer, event->u.bitrate.value);
      printf("Bit Rate:%s\n", buffer);
      break;
    case SIOCGIWNAME:
      printf("Protocol:%-1.16s\n", event->u.name);
      break;
    case IWEVQUAL:
      {
	event->u.qual.updated = 0x0;	/* Not that reliable, disable */
	iw_print_stats(buffer, &event->u.qual, iwrange, has_iwrange);
	printf("Link %s\n", buffer);
	break;
      }
    default:
      printf("(Unknown Wireless event 0x%04X)\n", event->cmd);
    }	/* switch(event->cmd) */

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Print out all Wireless Events part of the RTNetlink message
 * Most often, there will be only one event per message, but
 * just make sure we read everything...
 */
static inline int
print_event_stream(char *	ifname,
		   char *	data,
		   int		len)
{
  struct iw_event	iwe;
  struct stream_descr	stream;
  int			i = 0;
  int			ret;
  char			buffer[64];
  struct timeval	recv_time;
#if 0
  struct iw_range	range;
  int			has_range;
#endif

#if 0
  has_range = (iw_get_range_info(skfd, ifname, &range) < 0);
#endif

  /* In readable form */
  gettimeofday(&recv_time, NULL);
  iw_print_timeval(buffer, &recv_time);

  iw_init_event_stream(&stream, data, len);
  do
    {
      /* Extract an event and print it */
      ret = iw_extract_event_stream(&stream, &iwe);
      if(ret != 0)
	{
	  if(i++ == 0)
	    printf("%s   %-8.8s ", buffer, ifname);
	  else
	    printf("                           ");
	  if(ret > 0)
	    print_event_token(&iwe, ifname, NULL, 0);
	  else
	    printf("(Invalid event)\n");
	}
    }
  while(ret > 0);

  return(0);
}
#endif	/* WIRELESS_EXT > 13 */

/*********************** RTNETLINK EVENT DUMP***********************/
/*
 * Dump the events we receive from rtnetlink
 * This code is mostly from Casey
 */

/*------------------------------------------------------------------*/
/*
 * Respond to a single RTM_NEWLINK event from the rtnetlink socket.
 */
static inline int
index2name(int	index, char *name)
{
  int		skfd = -1;	/* generic raw socket desc.	*/
  struct ifreq	irq;
  int		ret = 0;

  memset(name, 0, IFNAMSIZ + 1);

  /* Create a channel to the NET kernel. */
  if((skfd = iw_sockets_open()) < 0)
    {
      perror("socket");
      exit(-1);
    }

  /* Get interface name */
  irq.ifr_ifindex = index;
  if(ioctl(skfd, SIOCGIFNAME, &irq) < 0)
    ret = -1;
  else
    strncpy(name, irq.ifr_name, IFNAMSIZ);

  close(skfd);
  return(ret);
}


/*------------------------------------------------------------------*/
/*
 * Respond to a single RTM_NEWLINK event from the rtnetlink socket.
 */
static int
LinkCatcher(struct nlmsghdr *nlh)
{
  struct ifinfomsg* ifi;
  char ifname[IFNAMSIZ + 1];

#if 0
  fprintf(stderr, "nlmsg_type = %d.\n", nlh->nlmsg_type);
#endif

  if(nlh->nlmsg_type != RTM_NEWLINK)
    return 0;

  ifi = NLMSG_DATA(nlh);

  /* Get a name... */
  index2name(ifi->ifi_index, ifname);

#if WIRELESS_EXT > 13
  /* Code is ugly, but sort of works - Jean II */

  /* Check for attributes */
  if (nlh->nlmsg_len > NLMSG_ALIGN(sizeof(struct ifinfomsg))) {
      int attrlen = nlh->nlmsg_len - NLMSG_ALIGN(sizeof(struct ifinfomsg));
      struct rtattr *attr = (void*)ifi + NLMSG_ALIGN(sizeof(struct ifinfomsg));

      while (RTA_OK(attr, attrlen)) {
	/* Check if the Wireless kind */
	if(attr->rta_type == IFLA_WIRELESS) {
	  /* Go to display it */
	  print_event_stream(ifname,
			     (void *)attr + RTA_ALIGN(sizeof(struct rtattr)),
			     attr->rta_len - RTA_ALIGN(sizeof(struct rtattr)));
	}
	attr = RTA_NEXT(attr, attrlen);
      }
  }
#endif	/* WIRELESS_EXT > 13 */

  return 0;
}

/* ---------------------------------------------------------------- */
/*
 * We must watch the rtnelink socket for events.
 * This routine handles those events (i.e., call this when rth.fd
 * is ready to read).
 */
static inline void
handle_netlink_events(struct rtnl_handle *	rth)
{
  while(1)
    {
      struct sockaddr_nl sanl;
      socklen_t sanllen;

      struct nlmsghdr *h;
      int amt;
      char buf[8192];

      amt = recvfrom(rth->fd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr*)&sanl, &sanllen);
      if(amt < 0)
	{
	  if(errno != EINTR && errno != EAGAIN)
	    {
	      fprintf(stderr, "%s: error reading netlink: %s.\n",
		      __PRETTY_FUNCTION__, strerror(errno));
	    }
	  return;
	}

      if(amt == 0)
	{
	  fprintf(stderr, "%s: EOF on netlink??\n", __PRETTY_FUNCTION__);
	  return;
	}

      h = (struct nlmsghdr*)buf;
      while(amt >= (int)sizeof(*h))
	{
	  int len = h->nlmsg_len;
	  int l = len - sizeof(*h);

	  if(l < 0 || len > amt)
	    {
	      fprintf(stderr, "%s: malformed netlink message: len=%d\n", __PRETTY_FUNCTION__, len);
	      break;
	    }

	  switch(h->nlmsg_type)
	    {
	    case RTM_NEWLINK:
	      LinkCatcher(h);
	      break;
	    default:
#if 0
	      fprintf(stderr, "%s: got nlmsg of type %#x.\n", __PRETTY_FUNCTION__, h->nlmsg_type);
#endif
	      break;
	    }

	  len = NLMSG_ALIGN(len);
	  amt -= len;
	  h = (struct nlmsghdr*)((char*)h + len);
	}

      if(amt > 0)
	fprintf(stderr, "%s: remnant of size %d on netlink\n", __PRETTY_FUNCTION__, amt);
    }
}

/**************************** MAIN LOOP ****************************/

/* ---------------------------------------------------------------- */
/*
 * Wait until we get an event
 */
static inline int
wait_for_event(struct rtnl_handle *	rth)
{
#if 0
  struct timeval	tv;	/* Select timeout */
#endif

  /* Forever */
  while(1)
    {
      fd_set		rfds;		/* File descriptors for select */
      int		last_fd;	/* Last fd */
      int		ret;

      /* Guess what ? We must re-generate rfds each time */
      FD_ZERO(&rfds);
      FD_SET(rth->fd, &rfds);
      last_fd = rth->fd;

      /* Wait until something happens */
      ret = select(last_fd + 1, &rfds, NULL, NULL, NULL);

      /* Check if there was an error */
      if(ret < 0)
	{
	  if(errno == EAGAIN || errno == EINTR)
	    continue;
	  fprintf(stderr, "Unhandled signal - exiting...\n");
	  break;
	}

      /* Check if there was a timeout */
      if(ret == 0)
	{
	  continue;
	}

      /* Check for interface discovery events. */
      if(FD_ISSET(rth->fd, &rfds))
	handle_netlink_events(rth);
    }

  return(0);
}

/******************************* MAIN *******************************/

/* ---------------------------------------------------------------- */
/*
 * helper ;-)
 */
static void
iw_usage(int status)
{
  fputs("Usage: iwevent [OPTIONS]\n"
	"   Monitors and displays Wireless Events.\n"
	"   Options are:\n"
	"     -h,--help     Print this message.\n"
	"     -v,--version  Show version of this program.\n",
	status ? stderr : stdout);
  exit(status);
}
/* Command line options */
static const struct option long_opts[] = {
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { NULL, 0, NULL, 0 }
};

/* ---------------------------------------------------------------- */
/*
 * main body of the program
 */
int
main(int	argc,
     char *	argv[])
{
  struct rtnl_handle	rth;
  int opt;

  /* Check command line options */
  while((opt = getopt_long(argc, argv, "hv", long_opts, NULL)) > 0)
    {
      switch(opt)
	{
	case 'h':
	  iw_usage(0);
	  break;

	case 'v':
	  return(iw_print_version_info("iwevent"));
	  break;

	default:
	  iw_usage(1);
	  break;
	}
    }
  if(optind < argc)
    {
      fputs("Too many arguments.\n", stderr);
      iw_usage(1);
    }

  /* Open netlink channel */
  if(rtnl_open(&rth, RTMGRP_LINK) < 0)
    {
      perror("Can't initialize rtnetlink socket");
      return(1);
    }

#if WIRELESS_EXT > 13
  fprintf(stderr, "Waiting for Wireless Events...\n");
#else	/* WIRELESS_EXT > 13 */
  fprintf(stderr, "Unsupported in Wireless Extensions <= 14 :-(\n");
  return(-1);
#endif	/* WIRELESS_EXT > 13 */

  /* Do what we have to do */
  wait_for_event(&rth);

  /* Cleanup - only if you are pedantic */
  rtnl_close(&rth);

  return(0);
}
