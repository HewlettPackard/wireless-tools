/*
 *	Wireless Tools
 *
 *		Jean II - HPLB 97->99 - HPL 99->01
 *
 * Common header for the Wireless Extension library...
 *
 * This file is released under the GPL license.
 */

#ifndef IWLIB_H
#define IWLIB_H

/*#include "CHANGELOG.h"*/

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
 * The selection has been moved in the Makefile, here we have only
 * the ugly part. Don't look, you would go blind ;-) */

#ifdef KLUDGE_HEADERS
#include <socketbits.h>
#endif	/* KLUDGE_HEADERS */

#if defined(KLUDGE_HEADERS) || defined(GLIBC_HEADERS)
#include <linux/if_arp.h>	/* For ARPHRD_ETHER */
#include <linux/socket.h>	/* For AF_INET & struct sockaddr */
#include <linux/in.h>		/* For struct sockaddr_in */
#endif	/* KLUDGE_HEADERS || GLIBC_HEADERS */

#ifdef GLIBC22_HEADERS 
/* Added by Ross G. Miller <Ross_Miller@baylor.edu>, 3/28/01 */
#include <linux/if_arp.h> 	/* For ARPHRD_ETHER */
#include <linux/socket.h>	/* For AF_INET & struct sockaddr */
#include <sys/socket.h>
#endif /* GLIBC22_HEADERS */    

#ifdef LIBC5_HEADERS
#include <sys/socket.h>		/* For AF_INET & struct sockaddr & socket() */
#include <linux/if_arp.h>	/* For ARPHRD_ETHER */
#include <linux/in.h>		/* For struct sockaddr_in */
#endif	/* LIBC5_HEADERS */

#ifdef PRIVATE_WE_HEADER
/* Private copy of Wireless extensions */
#include "wireless.h"
#else	/* PRIVATE_WE_HEADER */
/* System wide Wireless extensions */
#include <linux/wireless.h>
#endif	/* PRIVATE_WE_HEADER */

#if WIRELESS_EXT < 9
#error "Wireless Extension v9 or newer required :-(\
Use Wireless Tools v19 or update your kernel headers"
#endif
#if WIRELESS_EXT < 11
#warning "Wireless Extension v11 recommended...\
You may update your kernel and/or system headers to get the new features..."
#endif

/****************************** DEBUG ******************************/


/************************ CONSTANTS & MACROS ************************/

/* Some usefull constants */
#define KILO	1e3
#define MEGA	1e6
#define GIGA	1e9

/* Backward compatibility for Wireless Extension 9 */
#ifndef IW_POWER_MODIFIER
#define IW_POWER_MODIFIER	0x000F	/* Modify a parameter */
#define IW_POWER_MIN		0x0001	/* Value is a minimum  */
#define IW_POWER_MAX		0x0002	/* Value is a maximum */
#define IW_POWER_RELATIVE	0x0004	/* Value is not in seconds/ms/us */
#endif IW_POWER_MODIFIER

#ifndef IW_ENCODE_NOKEY
#define IW_ENCODE_NOKEY         0x0800  /* Key is write only, so not here */
#define IW_ENCODE_MODE		0xF000	/* Modes defined below */
#endif IW_ENCODE_NOKEY

/****************************** TYPES ******************************/

/* Shortcuts */
typedef struct iw_statistics	iwstats;
typedef struct iw_range		iwrange;
typedef struct iw_param		iwparam;
typedef struct iw_freq		iwfreq;
typedef struct iw_quality	iwqual;
typedef struct iw_priv_args	iwprivargs;
typedef struct sockaddr		sockaddr;

/* Structure for storing all wireless information for each device
 * This is pretty exhaustive... */
typedef struct wireless_info
{
  char		name[IFNAMSIZ];		/* Wireless/protocol name */
  int		has_nwid;
  iwparam	nwid;			/* Network ID */
  int		has_freq;
  float		freq;			/* Frequency/channel */
  int		has_sens;
  iwparam	sens;			/* sensitivity */
  int		has_key;
  unsigned char	key[IW_ENCODING_TOKEN_MAX];	/* Encoding key used */
  int		key_size;		/* Number of bytes */
  int		key_flags;		/* Various flags */
  int		has_essid;
  int		essid_on;
  char		essid[IW_ESSID_MAX_SIZE + 1];	/* ESSID (extended network) */
  int		has_nickname;
  char		nickname[IW_ESSID_MAX_SIZE + 1]; /* NickName */
  int		has_ap_addr;
  sockaddr	ap_addr;		/* Access point address */
  int		has_bitrate;
  iwparam	bitrate;		/* Bit rate in bps */
  int		has_rts;
  iwparam	rts;			/* RTS threshold in bytes */
  int		has_frag;
  iwparam	frag;			/* Fragmentation threshold in bytes */
  int		has_mode;
  int		mode;			/* Operation mode */
  int		has_power;
  iwparam	power;			/* Power management parameters */
  int		has_txpower;
  iwparam	txpower;		/* Transmit Power in dBm */
  int		has_retry;
  iwparam	retry;			/* Retry limit or lifetime */

  /* Stats */
  iwstats	stats;
  int		has_stats;
  iwrange	range;
  int		has_range;
} wireless_info;

/* Structure for storing all wireless information for each device
 * This is a cut down version of the one above, containing only
 * the things *truly* needed to configure a card.
 * Don't add other junk, I'll remove it... */
typedef struct wireless_config
{
  char		name[IFNAMSIZ];		/* Wireless/protocol name */
  int		has_nwid;
  iwparam	nwid;			/* Network ID */
  int		has_freq;
  float		freq;			/* Frequency/channel */
  int		has_key;
  unsigned char	key[IW_ENCODING_TOKEN_MAX];	/* Encoding key used */
  int		key_size;		/* Number of bytes */
  int		key_flags;		/* Various flags */
  int		has_essid;
  int		essid_on;
  char		essid[IW_ESSID_MAX_SIZE + 1];	/* ESSID (extended network) */
  int		has_mode;
  int		mode;			/* Operation mode */
} wireless_config;

/**************************** PROTOTYPES ****************************/
/*
 * All the functions in iwcommon.c
 */
/* ---------------------- SOCKET SUBROUTINES -----------------------*/
int
	iw_sockets_open(void);
/* --------------------- WIRELESS SUBROUTINES ----------------------*/
int
	iw_get_range_info(int		skfd,
			  char *	ifname,
			  iwrange *	range);
int
	iw_get_priv_info(int		skfd,
			 char *		ifname,
			 iwprivargs *	priv);
int
	iw_get_basic_config(int			skfd,
			    char *		ifname,
			    wireless_config *	info);
int
	iw_set_basic_config(int			skfd,
			    char *		ifname,
			    wireless_config *	info);
/* -------------------- FREQUENCY SUBROUTINES --------------------- */
void
	iw_float2freq(double	in,
		   iwfreq *	out);
double
	iw_freq2float(iwfreq *	in);
/* ---------------------- POWER SUBROUTINES ----------------------- */
int
	iw_dbm2mwatt(int	in);
int
	iw_mwatt2dbm(int	in);
/* -------------------- STATISTICS SUBROUTINES -------------------- */
int
	iw_get_stats(int	skfd,
		     char *	ifname,
		     iwstats *	stats);
void
	iw_print_stats(char *		buffer,
		       iwqual *		qual,
		       iwrange *	range,
		       int		has_range);
/* --------------------- ENCODING SUBROUTINES --------------------- */
void
	iw_print_key(char *		buffer,
		     unsigned char *	key,
		     int		key_size,
		     int		key_flags);
/* ----------------- POWER MANAGEMENT SUBROUTINES ----------------- */
void
	iw_print_pm_value(char *	buffer,
			  int		value,
			  int		flags);
void
	iw_print_pm_mode(char *		buffer,
			 int		flags);
/* --------------- RETRY LIMIT/LIFETIME SUBROUTINES --------------- */
#if WIRELESS_EXT > 10
void
	iw_print_retry_value(char *	buffer,
			     int	value,
			     int	flags);
#endif
/* --------------------- ADDRESS SUBROUTINES ---------------------- */
int
	iw_check_addr_type(int	skfd,
			char *	ifname);
char *
	iw_pr_ether(char *buffer, unsigned char *ptr);
int
	iw_in_ether(char *bufp, struct sockaddr *sap);
int
	iw_in_inet(char *bufp, struct sockaddr *sap);
int
	iw_in_addr(int			skfd,
		   char *		ifname,
		   char *		bufp,
		   struct sockaddr *	sap);
/* ----------------------- MISC SUBROUTINES ------------------------ */
int
	iw_byte_size(int		args);

/**************************** VARIABLES ****************************/

extern const char *	iw_operation_mode[];
#define IW_NUM_OPER_MODE	6

#endif	/* IWLIB_H */
