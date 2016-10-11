/*
 *	Wireless Tools
 *
 *		Jean II - HPLB 97->99 - HPL 99->02
 *
 * The changelog...
 *
 * This files is released under the GPL license.
 *     Copyright (c) 1997-2002 Jean Tourrilhes <jt@hpl.hp.com>
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
 *
 * wireless 22 :
 * -----------
 *		(From Jim Kaba <jkaba@sarnoff.com>)
 *	o Fix socket_open to not open all types of sockets [iwcommon]
 *		(From Michael Tokarev <mjt@tls.msk.ru>)
 *	o Rewrite main (top level) + command line parsing of [iwlist]
 *		(From Jean Tourrilhes)
 *	o Set commands should return proper success flag [iwspy/iwpriv]
 *	  requested by Michael Tokarev
 *	---
 *		(From Torgeir Hansen <torgeir@trenger.ro>)
 *	o Replace "strcpy(wrq.ifr_name," with strncpy to avoid buffer
 *	  overflows. This is OK because the kernel use strncmp...
 *	---
 *	o Move operation_mode in iwcommon and add NUM_OPER_MODE [iwconfig]
 *	o print_stats, print_key, ... use char * instead if FILE * [iwcommon]
 *	o Add `iw_' prefix to avoid namespace pollution [iwcommon]
 *	o Add iw_get_basic_config() and iw_set_basic_config() [iwcommon]
 *	o Move iw_getstats from iwconfig to iwcommon [iwcommon]
 *	o Move changelog to CHANGELOG.h [iwcommon]
 *	o Rename iwcommon.* into iwlib.* [iwcommon->iwlib]
 *	o Compile iwlib. as a dynamic or static library [Makefile]
 *	o Allow the tools to be compiled with the dynamic library [Makefile]
 *	--- Update to Wireless Extension 12 ---
 *	o Show typical/average quality in iwspy [iwspy]
 *	o Get Wireless Stats through ioctl instead of /proc [iwlib]
 *
 * wireless 23 :
 * -----------
 *	o Split iw_check_addr_type() into two functions mac/if [iwlib]
 *	o iw_in_addr() does appropriate iw_check_xxx itself  [iwlib]
 *	o Allow iwspy on MAC address even if IP doesn't check [iwspy]
 *	o Allow iwconfig ap on MAC address even if IP doesn't check [iwconfig]
 *	---
 *	o Fix iwlist man page about extra commands [iwlist]
 *	---
 *	o Fix Makefile rules for library compile (more generic) [Makefile]
 *	---
 *	o Set max length for all GET request with a iw_point [various]
 *	o Fix set IW_PRIV_TYPE_BYTE to be endian/align clean [iwpriv]
 *	---
 *		(From Kernel Jake <kerneljake@hotmail.com>)
 *	o Add '/' at the end of directories to create them [Makefile]
 *		(From Pavel Roskin <proski@gnu.org>)
 *	o Replace "cp" with "install" to get permissions proper [Makefile]
 *	o Install Man-Pages at the proper location [Makefile]
 *	o Add automatic header selection based on libc/kernel [iwlib.h]
 *	---
 *	o Add "commit" to force parameters on the card [iwconfig]
 *	o Wrap ioctl() in iw_set/get_ext() wrappers [all]
 *	o Beautify set request error messages [iwconfig]
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
