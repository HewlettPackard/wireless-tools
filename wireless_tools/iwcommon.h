/*
 *	Wireless Tools
 *
 *		Jean II - HPLB 97->99 - HPL 99->00
 *
 * Common header for the wireless tools...
 *
 * This file is released under the GPL license.
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
 *
 * wireless 20 :
 * -----------
 *		(From Jean Tourrilhes)
 *	o Remove all #ifdef WIRELESS ugliness, but add a #error :
 *		we require Wireless Extensions 9 or nothing !  [all]
 *	o Switch to new 'nwid' definition (specific -> iw_param) [iwconfig]
 *	o Rewriten totally the encryption support [iwconfig]
 *		- Multiple keys, through key index
 *		- Flexible/multiple key size, and remove 64bits upper limit
 *		- Open/Restricted modes
 *		- Enter keys as ASCII strings
 *	o List key sizes supported and all keys in [iwspy]
 *	o Mode of operation support (ad-hoc, managed...) [iwconfig]
 *	o Use '=' to indicate fixed instead of ugly '(f)' [iwconfig]
 *	o Ability to disable RTS & frag (off), now the right way [iwconfig]
 *	o Auto as an input modifier for bitrate [iwconfig]
 *	o Power Management support [iwconfig]
 *		- set timeout or period and its value
 *		- Reception mode (unicast/multicast/all)
 *	o Updated man pages with all that ;-)
 *
 * wireless 21 :
 * -----------
 *		(from Alan McReynolds <alan_mcreynolds@hpl.hp.com>)
 *	o Use proper macros for compilation directives [Makefile]
 *		(From Jean Tourrilhes)
 *	o Put licensing info everywhere (almost). Yes, it's GPL !
 *	o Document the use of /etc/pcmcia/wireless.opts
 *	o Add min/max modifiers to power management parameters [iwconfig]
 *		-> requested by Lee Keyser-Allen for the Spectrum24 driver
 *	o Optionally output a second power management parameter [iwconfig]
 *	---
 *	o Common subroutines to display stats & power saving info [iwcommon]
 *	o Display all power management info, capability and values [iwspy]
 *	---
 *	o Optional index for ESSID (for Aironet driver) [iwcommon]
 *	o IW_ENCODE_NOKEY for write only keys [iwconfig/iwspy]
 *	o Common subrouting to print encoding keys [iwspy]
 *	---
 *	o Transmit Power stuff (dBm + mW) [iwconfig/iwspy]
 *	o Cleaner formatting algorithm when displaying params [iwconfig]
 *	---
 *	o Fix get_range_info() and use it everywhere - Should fix core dumps.
 *	o Catch WE version differences between tools and driver and
 *	  warn user. Thanks to Tobias Ringstrom for the tip... [iwcommon]
 *	o Add Retry limit and lifetime support. [iwconfig/iwlist]
 *	o Display "Cell:" instead of "Access Point:" in ad-hoc mode [iwconfig]
 *	o Header fix for glibc2.2 by Ross G. Miller <Ross_Miller@baylor.edu>
 *	o Move header selection flags in Makefile [iwcommon/Makefile]
 *	o Spin-off iwlist.c from iwspy.c. iwspy is now much smaller
 *	  After moving this bit of code all over the place, from iwpriv
 *	  to iwconfig to iwspy, it now has a home of its own... [iwspy/iwlist]
 *	o Wrote quick'n'dirty iwgetid.
 *	o Remove output of second power management parameter [iwconfig]
 *	  Please use iwlist, I don't want to bloat iwconfig
 *	---
 *	o Fix bug in display ints - "Allen Miu" <aklmiu@mit.edu> [iwpriv]
 */

/* ----------------------------- TODO ----------------------------- */
/*
 * One day, maybe...
 *
 * iwconfig :
 * --------
 *	Make disable a per encryption key modifier if some hardware
 *	requires it.
 *
 * iwpriv :
 * ------
 *	Remove 'port' and 'roam' cruft now that we have mode in iwconfig
 *
 * iwspy :
 * -----
 *	-
 *
 * Doc & man pages :
 * ---------------
 *	Update main doc.
 *
 * Other :
 * -----
 *	What about some graphical tools ?
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

/* Structure for storing all wireless information for each device */
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
/* ---------------------- POWER SUBROUTINES ----------------------- */
int
	dbm2mwatt(int	in);
int
	mwatt2dbm(int	in);
/* -------------------- STATISTICS SUBROUTINES -------------------- */
void
	print_stats(FILE *	stream,
		    iwqual *	qual,
		    iwrange *	range,
		    int		has_range);
/* --------------------- ENCODING SUBROUTINES --------------------- */
void
	print_key(FILE *		stream,
		  unsigned char	*	key,
		  int			key_size,
		  int			key_flags);
/* ----------------- POWER MANAGEMENT SUBROUTINES ----------------- */
void
	print_pm_value(FILE *	stream,
		       int	value,
		       int	flags);
void
	print_pm_mode(FILE *	stream,
		      int	flags);
/* --------------- RETRY LIMIT/LIFETIME SUBROUTINES --------------- */
#if WIRELESS_EXT > 10
void
	print_retry_value(FILE *	stream,
			  int		value,
			  int		flags);
#endif
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
