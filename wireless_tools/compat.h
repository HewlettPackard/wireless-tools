/* This whole file is flaky */
/* This is all we need to compile and can't find only in linux headers.
 * So, I did copy past here...
 */
#define __u8	unsigned char
#define __u16	unsigned short
#define __u32	unsigned long		/* Hum, and on Alpha ? */
#define __u64	unsigned long long	/* Unsure about this one */

#define	IFNAMSIZ	16
#define ARPHRD_ETHER 	1		/* Ethernet 10Mbps		*/
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */

/*
 * Interface request structure used for socket
 * ioctl's.  All interface ioctl's must have parameter
 * definitions which begin with ifr_name.  The
 * remainder may be interface specific.
 */
struct ifreq 
{
#define IFHWADDRLEN	6
#define	IFNAMSIZ	16
	union
	{
		char	ifrn_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	} ifr_ifrn;
	
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		struct	sockaddr ifru_netmask;
		struct  sockaddr ifru_hwaddr;
		short	ifru_flags;
		int	ifru_metric;
		int	ifru_mtu;
#if 0
		struct  ifmap ifru_map;
#endif
		char	ifru_slave[IFNAMSIZ];	/* Just fits the size */
		caddr_t	ifru_data;
	} ifr_ifru;
};
#define ifr_name	ifr_ifrn.ifrn_name	/* interface name 	*/
#define ifr_hwaddr	ifr_ifru.ifru_hwaddr	/* MAC address 		*/
#define	ifr_addr	ifr_ifru.ifru_addr	/* address		*/

/*
 * Structure used in SIOCGIFCONF request.
 * Used to retrieve interface configuration
 * for machine (useful for programs which
 * must know all networks accessible).
 */
struct ifconf 
{
	int	ifc_len;			/* size of buffer	*/
	union 
	{
		caddr_t	ifcu_buf;
		struct	ifreq *ifcu_req;
	} ifc_ifcu;
};
#define	ifc_buf	ifc_ifcu.ifcu_buf		/* buffer address	*/
#define	ifc_req	ifc_ifcu.ifcu_req		/* array of structures	*/

/* Internet address. */
struct in_addr {
	__u32	s_addr;
};

/* Address to accept any incoming messages. */
#define	INADDR_ANY		((unsigned long int) 0x00000000)

/* Structure describing an Internet (IP) socket address. */
#define __SOCK_SIZE__	16		/* sizeof(struct sockaddr)	*/
struct sockaddr_in {
  short int		sin_family;	/* Address family		*/
  unsigned short int	sin_port;	/* Port number			*/
  struct in_addr	sin_addr;	/* Internet address		*/

  /* Pad to size of `struct sockaddr'. */
  unsigned char		__pad[__SOCK_SIZE__ - sizeof(short int) -
			sizeof(unsigned short int) - sizeof(struct in_addr)];
};

#define ATF_COM		0x02		/* completed entry (ha valid)	*/
/* ARP ioctl request. */
struct arpreq {
  struct sockaddr	arp_pa;		/* protocol address		*/
  struct sockaddr	arp_ha;		/* hardware address		*/
  int			arp_flags;	/* flags			*/
  struct sockaddr       arp_netmask;    /* netmask (only for proxy arps) */
  char			arp_dev[16];
};

