/*
 * Warning : this program need to be link against libsupport.a
 * in net-tools-1.2.0
 * This program need also wireless extensions...
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
#include "support.h"

int skfd = -1;				/* generic raw socket desc.	*/
int ipx_sock = -1;			/* IPX socket			*/
int ax25_sock = -1;			/* AX.25 socket			*/
int inet_sock = -1;			/* INET socket			*/
int ddp_sock = -1;			/* Appletalk DDP socket		*/

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
  struct ifreq		ifr;
  struct iwreq		wrq;
  struct aftype *ap;
  struct hwtype *hw;
  char *	ifname = argv[1];

  if(argc < 2)
    exit(0);

  /* Create a channel to the NET kernel. */
  if((skfd = sockets_open()) < 0)
    {
      perror("socket");
      exit(-1);
    }

  /* Get the type of interface address */
  strcpy(ifr.ifr_name, ifname);
  if((ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0) ||
     ((ap = get_afntype(ifr.ifr_addr.sa_family)) == NULL))
    {
      /* Deep trouble... */
      fprintf(stderr, "Interface %s unavailable\n", ifname);
      exit(0);
    }

#ifdef DEBUG
  printf("Interface : %d - %s - %s\n", ifr.ifr_addr.sa_family,
	 ap->name, ap->sprint(&ifr.ifr_addr, 1));
#endif

  /* Get the type of hardware address */
  strcpy(ifr.ifr_name, ifname);
  if((ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) ||
     ((hw = get_hwntype(ifr.ifr_hwaddr.sa_family)) == NULL))
    {
      /* Deep trouble... */
      fprintf(stderr, "Unable to get hardware address of the interface %s\n",
	     ifname);
      exit(0);
    }

#ifdef DEBUG
  printf("Hardware : %d - %s - %s\n", ifr.ifr_hwaddr.sa_family,
	 hw->name, hw->print(ifr.ifr_hwaddr.sa_data));
#endif

  /* Is there any arguments ? */
  if(argc < 3)
    {	/* Nope : read out from kernel */
      char		buffer[(sizeof(struct iw_quality) +
				sizeof(struct sockaddr)) * IW_MAX_SPY];
      struct sockaddr *	hwa;
      struct iw_quality *qual;
      int		n;
      int		i;

      strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
      wrq.u.data.pointer = (caddr_t) buffer;
      wrq.u.data.length = 0;
      wrq.u.data.flags = 0;
      if(ioctl(skfd, SIOCGIWSPY, &wrq) < 0)
	{
	  fprintf(stderr, "Interface doesn't accept reading addresses...\n");
	  fprintf(stderr, "SIOCGIWSPY: %s\n", strerror(errno));
	  return(-1);
	}

      n = wrq.u.data.length;

      hwa = (struct sockaddr *) buffer;
      qual = (struct iw_quality *) (buffer + (sizeof(struct sockaddr) * n));

      for(i = 0; i < n; i++)
	printf("%s : Quality %d ; Signal %d ; Noise %d %s\n",
	       hw->print(hwa[i].sa_data),
	       qual[i].qual, qual[i].level, qual[i].noise,
	       qual[i].updated & 0x7 ? "(updated)" : "");
    }
  else
    {	/* Send addresses to kernel */
      int		i;
      int		nbr;		/* Number of valid addresses */
      struct sockaddr	hw_address[IW_MAX_SPY];
      struct sockaddr	if_address;
      struct arpreq	arp_query;

      /* Read command line */
      i = 2;	/* first arg to read */
      nbr = 0;	/* Number of args readen so far */

      /* "off" : disable functionality (set 0 addresses) */
      if(!strcmp(argv[2], "off"))
	i = argc;	/* hack */

      /* "+" : add all addresses already in the driver */
      if(!strcmp(argv[2], "+"))
	{
	  char		buffer[(sizeof(struct iw_quality) +
				sizeof(struct sockaddr)) * IW_MAX_SPY];

	  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	  wrq.u.data.pointer = (caddr_t) buffer;
	  wrq.u.data.length = 0;
	  wrq.u.data.flags = 0;
	  if(ioctl(skfd, SIOCGIWSPY, &wrq) < 0)
	    {
	      fprintf(stderr, "Interface doesn't accept reading addresses...\n");
	      fprintf(stderr, "SIOCGIWSPY: %s\n", strerror(errno));
	      return(-1);
	    }

	  /* Copy old addresses */
	  nbr = wrq.u.data.length;
	  memcpy(hw_address, buffer, nbr * sizeof(struct sockaddr));

	  i = 3;	/* skip the "+" */
	}

      /* Read other args on command line */
      while((i < argc) && (nbr < IW_MAX_SPY))
	{
	  /* If it is not a hardware address (prefixed by hw)... */
	  if(strcmp(argv[i], "hw"))
	    {
	      /* Read interface address */
	      if(ap->input(argv[i++], &if_address) < 0)
		{
		  fprintf(stderr, "Invalid interface address %s\n", argv[i - 1]);
		  continue;
		}

	      /* Translate IP addresses to MAC addresses */
	      memcpy((char *) &(arp_query.arp_pa),
		     (char *) &if_address,
		     sizeof(struct sockaddr));
	      arp_query.arp_ha.sa_family = 0;
	      arp_query.arp_flags = 0;
	      /* The following restrict the search to the interface only */
	      strcpy(arp_query.arp_dev, ifname);
	      if((ioctl(inet_sock, SIOCGARP, &arp_query) < 0) ||
		 !(arp_query.arp_flags & ATF_COM))
		{
		  fprintf(stderr, "Arp failed for %s... (%d) Try to ping the address before.\n",
			 ap->sprint(&if_address, 1), errno);
		  continue;
		}

	      /* Store new MAC address */
	      memcpy((char *) &(hw_address[nbr++]),
		     (char *) &(arp_query.arp_ha),
		     sizeof(struct sockaddr));

#ifdef DEBUG
	      printf("IP Address %s => Hw Address = %s - %d\n",
		     ap->sprint(&if_address, 1),
		     hw->print(hw_address[nbr - 1].sa_data));
#endif
	    }
	  else	/* If it's an hardware address */
	    {
	      if(++i >= argc)
		{
		  fprintf(stderr, "hw must be followed by an address...\n");
		  continue;
		}

	      /* Get the hardware address */
	      if(hw->input(argv[i++], &(hw_address[nbr])) < 0)
		{
		  fprintf(stderr, "Invalid hardware address %s\n", argv[i - 1]);
		  continue;
		}
	      nbr++;

#ifdef DEBUG
	      printf("Hw Address = %s - %d\n",
		     hw->print(hw_address[nbr - 1].sa_data));
#endif
	    }
	}	/* Loop on all addresses */


      /* Check the number of addresses */
      if((nbr == 0) && strcmp(argv[2], "off"))
	{
	  fprintf(stderr, "No valid addresses found : exiting...\n");
	  exit(0);
	}

      /* Check if there is some remaining arguments */
      if(i < argc)
	{
	  fprintf(stderr, "Got only the first %d addresses, remaining discarded\n", IW_MAX_SPY);
	}

      /* Time to do send addresses to the driver */
      strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
      wrq.u.data.pointer = (caddr_t) hw_address;
      wrq.u.data.length = nbr;
      wrq.u.data.flags = 0;
      if(ioctl(skfd, SIOCSIWSPY, &wrq) < 0)
	{
	  fprintf(stderr, "Interface doesn't accept addresses...\n");
	  fprintf(stderr, "SIOCSIWSPY: %s\n", strerror(errno));
	  return(-1);
	}
    }

  /* Close the socket. */
  close(skfd);

  return(1);
}
