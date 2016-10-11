/*
 * Warning : this program need wireless extensions...
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
byte_size(args)
{
  int	ret = args & IW_PRIV_SIZE_MASK;

  if(((args & IW_PRIV_TYPE_MASK) == IW_PRIV_TYPE_INT) ||
     ((args & IW_PRIV_TYPE_MASK) == IW_PRIV_TYPE_FLOAT))
    ret <<= 2;

  if((args & IW_PRIV_TYPE_MASK) == IW_PRIV_TYPE_NONE)
    return 0;

  return ret;
}

int
main(int	argc,
     char **	argv)
{
  struct iwreq		wrq;
  char *		ifname = argv[1];
  struct iw_priv_args	priv[16];
  int			k;

  if(argc < 2)
    exit(0);

  /* Create a channel to the NET kernel. */
  if((skfd = sockets_open()) < 0)
    {
      perror("socket");
      exit(-1);
    }

  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  wrq.u.data.pointer = (caddr_t) priv;
  wrq.u.data.length = 0;
  wrq.u.data.flags = 0;
  if(ioctl(skfd, SIOCGIWPRIV, &wrq) < 0)
    {
      fprintf(stderr, "Interface doesn't provide private interface info...\n");
      fprintf(stderr, "SIOCGIWPRIV: %s\n", strerror(errno));
      return(-1);
    }

  /* If no args... */
  if(argc < 3)
    {
      char *	argtype[] = { "    ", "byte", "char", "", "int", "float" };

      printf("Available private ioctl :\n");
      for(k = 0; k < wrq.u.data.length; k++)
	printf("%s (%lX) : set %3d %s & get %3d %s\n",
	       priv[k].name, priv[k].cmd,
	       priv[k].set_args & IW_PRIV_SIZE_MASK,
	       argtype[(priv[k].set_args & IW_PRIV_TYPE_MASK) >> 12],
	       priv[k].get_args & IW_PRIV_SIZE_MASK,
	       argtype[(priv[k].get_args & IW_PRIV_TYPE_MASK) >> 12]);
    }
  else
    {
      u_char	buffer[1024];

      /* Seach the correct ioctl */
      k = -1;
      while((++k < wrq.u.data.length) && strcmp(priv[k].name, argv[2]))
	;
      /* If not found... */
      if(k == wrq.u.data.length)
	fprintf(stderr, "Invalid argument : %s\n", argv[2]);

      /* If we have to set some data */
      if((priv[k].set_args & IW_PRIV_TYPE_MASK) &&
	 (priv[k].set_args & IW_PRIV_SIZE_MASK))
	{
	  int	i;

	  /* Warning : we may have no args to set... */

	  switch(priv[k].set_args & IW_PRIV_TYPE_MASK)
	    {
	    case IW_PRIV_TYPE_BYTE:
	      /* Number of args to fetch */
	      wrq.u.data.length = argc - 3;
	      if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
		wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

	      /* Fetch args */
	      for(i = 0; i < wrq.u.data.length; i++)
		sscanf(argv[i + 3], "%d", buffer + i);
	      break;

	    case IW_PRIV_TYPE_INT:
	      /* Number of args to fetch */
	      wrq.u.data.length = argc - 3;
	      if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
		wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

	      /* Fetch args */
	      for(i = 0; i < wrq.u.data.length; i++)
		sscanf(argv[i + 3], "%d", ((u_int *) buffer) + i);
	      break;

	    default:
	      fprintf(stderr, "Not yet implemented...\n");
	      return(-1);
	    }

	  if((priv[k].set_args & IW_PRIV_SIZE_FIXED) &&
	     (wrq.u.data.length != (priv[k].set_args & IW_PRIV_SIZE_MASK)))
	    {
	      printf("The command %s need exactly %d argument...\n",
		     priv[k].name, priv[k].set_args & IW_PRIV_SIZE_MASK);
	      return(-1);
	    }
	}	/* if args to set */
      else
	{
	  wrq.u.data.length = 0L;
	}

      strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

      if((priv[k].set_args & IW_PRIV_SIZE_FIXED) &&
	 (byte_size(priv[k].set_args) < IFNAMSIZ))
	memcpy(wrq.u.name, buffer, IFNAMSIZ);
      else
	{
	  wrq.u.data.pointer = (caddr_t) buffer;
	  wrq.u.data.flags = 0;
	}

      if(ioctl(skfd, priv[k].cmd, &wrq) < 0)
	{
	  fprintf(stderr, "Interface doesn't accept private ioctl...\n");
	  fprintf(stderr, "%X: %s\n", priv[k].cmd, strerror(errno));
	  return(-1);
	}

      /* If we have to get some data */
      if((priv[k].get_args & IW_PRIV_TYPE_MASK) &&
	 (priv[k].get_args & IW_PRIV_SIZE_MASK))
	{
	  int	i;
	  int	n;		/* number of args */

	  if((priv[k].get_args & IW_PRIV_SIZE_FIXED) &&
	     (byte_size(priv[k].get_args) < IFNAMSIZ))
	    {
	      memcpy(buffer, wrq.u.name, IFNAMSIZ);
	      n = priv[k].get_args & IW_PRIV_SIZE_MASK;
	    }
	  else
	    n = wrq.u.data.length;

	  switch(priv[k].get_args & IW_PRIV_TYPE_MASK)
	    {
	    case IW_PRIV_TYPE_BYTE:
	      /* Display args */
	      for(i = 0; i < n; i++)
		printf("%d  ", buffer[i]);
	      printf("\n");
	      break;

	    case IW_PRIV_TYPE_INT:
	      /* Display args */
	      for(i = 0; i < n; i++)
		printf("%d  ", ((u_int *) buffer)[i]);
	      printf("\n");
	      break;

	    default:
	      fprintf(stderr, "Not yet implemented...\n");
	      return(-1);
	    }

	}	/* if args to set */

    }	/* if ioctl list else ioctl exec */

  /* Close the socket. */
  close(skfd);

  return(1);
}
