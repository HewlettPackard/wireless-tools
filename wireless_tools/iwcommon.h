/*
 *	Wireless Tools
 *
 *		Jean II - HPLB '99
 *
 * Common header for the wireless tools...
 */

#ifndef IWCOMMON_H
#define IWCOMMON_H

/************************** DOCUMENTATION **************************/
/*
 * None ? Todo...
 */

/* --------------------------- HISTORY --------------------------- */
/*
 * wireless 16 :		(Jean Tourrilhes)
 * -----------
 *	o iwconfig, iwpriv & iwspy
 *
 * wireless 17 :		(Justin Seger)
 * -----------
 *	o Compile under glibc fix
 *	o merge iwpriv in iwconfig
 *	o Add Wavelan roaming support
 *	o Update man page of iwconfig
 *
 * wireless 18 :
 * -----------
 *		(From Andreas Neuhaus <andy@fasta.fh-dortmund.de>)
 *	o Many fix to remove "core dumps" in iwconfig
 *	o Remove useless headers in iwconfig
 *	o CHAR wide private ioctl
 *		(From Jean Tourrilhes)
 *	o Create iwcommon.h and iwcommon.c
 *	o Separate iwpriv again for user interface issues
 *	  The folllowing didn't make sense and crashed :
 *		iwconfig eth0 priv sethisto 12 15 nwid 100
 *	o iwspy no longer depend on net-tools-1.2.0
 *	o Reorganisation of the code, cleanup
 *	o Add ESSID stuff in iwconfig
 *	o Add display of level & noise in dBm (stats in iwconfig)
 *	o Update man page of iwconfig and iwpriv
 *	o Add xwireless (didn't check if it compiles)
 *		(From Dean W. Gehnert <deang@tpi.com>)
 *	o Minor fixes
 *		(Jan Rafaj <rafaj@cedric.vabo.cz>)
 *	o Cosmetic changes (sensitivity relative, freq list)
 *	o Frequency computation on double
 *	o Compile clean on libc5
 *		(From Jean Tourrilhes)
 *	o Move listing of frequencies to iwspy
 *	o Add AP address stuff in iwconfig
 *	o Add AP list stuff in iwspy
 *
 * wireless 19 :
 * -----------
 *		(From Jean Tourrilhes)
 *	o Allow for sensitivity in dBm (if < 0) [iwconfig]
 *	o Formatting changes in displaying ap address in [iwconfig]
 *	o Slightly improved man pages and usage display
 *	o Add channel number for each frequency in list [iwspy]
 *	o Add nickname... [iwconfig]
 *	o Add "port" private ioctl shortcut [iwpriv]
 *	o If signal level = 0, no range or dBms [iwconfig]
 *	o I think I now got set/get char strings right in [iwpriv]
 *		(From Thomas Ekstrom <tomeck@thelogic.com>)
 *	o Fix a very obscure bug in [iwspy]
 */

/* ----------------------------- TODO ----------------------------- */
/*
 * One day, maybe...
 *
 * iwconfig :
 * --------
 *	Use new 802.11 parameters (rate, rts, frag)...
 *
 * iwpriv :
 * ------
 *	?
 *
 * iwspy :
 * -----
 *	?
 *
 * Doc & man pages :
 * ---------------
 *	?
 */

/***************************** INCLUDES *****************************/

/* Standard headers */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>		/* gethostbyname, getnetbyname */

/* This is our header selection. Try to hide the mess and the misery :-(
 * Please choose only one of the define...
 */
#define KER2_2_HEADERS		/* Kernel 2.2.X + Glibc - ok for most people */
#undef LINUX_HEADERS		/* Kernel 2.0.X + Glibc - Debian 2.0, RH5 */
#undef LIBC5_HEADERS		/* Kernel 2.0.X + libc5 - old systems */
#undef PRIVATE_HEADERS		/* Ugly last resort case */

#ifdef KER2_2_HEADERS
#include <socketbits.h>
#include <linux/if_arp.h>	/* For ARPHRD_ETHER */
#include <linux/socket.h>	/* For AF_INET & struct sockaddr */
#include <linux/in.h>		/* For struct sockaddr_in */

/* Wireless extensions */
#include <linux/wireless.h>

#endif	/* KER2_2_HEADERS */

#ifdef LINUX_HEADERS
#include <linux/if_arp.h>	/* For ARPHRD_ETHER */
#include <linux/socket.h>	/* For AF_INET & struct sockaddr */
#include <linux/in.h>		/* For struct sockaddr_in */

/* Wireless extensions */
#include <linux/wireless.h>

#endif	/* LINUX_HEADERS */

#ifdef LIBC5_HEADERS
#include <sys/socket.h>		/* For AF_INET & struct sockaddr & socket() */
#include <linux/if_arp.h>	/* For ARPHRD_ETHER */
#include <linux/in.h>		/* For struct sockaddr_in */

/* Wireless extensions */
#include <linux/wireless.h>

#endif	/* LIBC5_HEADERS */

#ifdef PRIVATE_HEADERS
#include <sys/types.h>
#include <sys/socket.h>
#include <socketbits.h>
#include "compat.h"		/* Don't ask ! */

/* Wireless extensions */
#include "wireless.h"

#endif	/* PRIVATE_HEADERS */


/****************************** DEBUG ******************************/


/************************ CONSTANTS & MACROS ************************/

/* Some usefull constants */
#define KILO	1e3
#define MEGA	1e6
#define GIGA	1e9

/* Some hack */
#ifndef IW_ESSID_MAX_SIZE
#define IW_ESSID_MAX_SIZE	32	/* Should be in wireless.h */
#endif
#ifndef IW_MAX_AP
#define IW_MAX_AP		8	/* Should be in wireless.h */
#endif

/****************************** TYPES ******************************/

/* Shortcuts */
typedef struct iw_statistics	iwstats;
typedef struct iw_range		iwrange;
typedef struct iw_freq		iwfreq;
typedef struct iw_priv_args	iwprivargs;
typedef struct sockaddr		sockaddr;

/* Structure for storing all wireless information for each device */
typedef struct wireless_info
{
  char		name[IFNAMSIZ];		/* Wireless/protocol name */
  int		has_nwid;
  int		nwid_on;
  u_long	nwid;			/* Network ID */
  int		has_freq;
  float		freq;			/* Frequency/channel */
  int		has_sens;
  int		sens;			/* sensitivity */
  int		has_enc;
  int		enc_method;		/* encoding method or off */
  long long	enc_key;		/* key used */
  int		has_essid;
  int		essid_on;
  char		essid[IW_ESSID_MAX_SIZE + 1];	/* ESSID (extended network) */
  int		has_nickname;
  char		nickname[IW_ESSID_MAX_SIZE + 1]; /* NickName */
  int		has_ap_addr;
  sockaddr	ap_addr;		/* Access point address */
  int		has_bitrate;
  long		bitrate;		/* Bit rate in bps */
  int		bitrate_fixed;		/* Fixed or auto */
  int		has_rts;
  long		rts;			/* RTS threshold in bytes */
  int		rts_fixed;		/* Fixed or auto */
  int		has_frag;
  long		frag;			/* Fragmentation threshold in bytes */
  int		frag_fixed;		/* Fixed or auto */

  /* Stats */
  iwstats	stats;
  int		has_stats;
  iwrange	range;
  int		has_range;
} wireless_info;

/**************************** PROTOTYPES ****************************/
/*
 * All the functions in iwcommon.c
 */
/* ---------------------- SOCKET SUBROUTINES -----------------------*/
int
	sockets_open(void);
/* --------------------- WIRELESS SUBROUTINES ----------------------*/
int
	get_range_info(int		skfd,
		       char *		ifname,
		       iwrange *	range);
int
	get_priv_info(int		skfd,
		      char *		ifname,
		      iwprivargs *	priv);
/* -------------------- FREQUENCY SUBROUTINES --------------------- */
void
	float2freq(double	in,
		   iwfreq *	out);
double
	freq2float(iwfreq *	in);
/* --------------------- ADDRESS SUBROUTINES ---------------------- */
int
	check_addr_type(int	skfd,
			char *	ifname);
char *
	pr_ether(unsigned char *ptr);
int
	in_ether(char *bufp, struct sockaddr *sap);
int
	in_inet(char *bufp, struct sockaddr *sap);
int
	in_addr(int		skfd,
		char *		ifname,
		char *		bufp,
		struct sockaddr *sap);
/* ----------------------- MISC SUBROUTINES ------------------------ */
int
	byte_size(int		args);

/**************************** VARIABLES ****************************/

#endif	/* IWCOMMON_H */
