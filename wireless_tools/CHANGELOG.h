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
 *	o Document the use of /etc/pcmcia/wireless.opts [PCMCIA]
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
 *
 * wireless 24 :
 * -----------
 *	o Added common function to display frequency [iwlib]
 *	o Added handler to parse Wireless Events [iwlib]
 *	o Added tool to display Wireless Events [iwevent]
 *	o Pass command line to subroutines [iwlist]
 *	o Scanning support through SIOCSIWSCAN [iwlist]
 *	---
 *	o Added common function to display bitrate [iwlib]
 *	o Add bitrate/encoding scanning support [iwlist]
 *	o Allow to read scan results for non-root users [iwlist]
 *	o Set 5s timeout on waiting for scan results [iwlist]
 *	o Cleanup iwgetid & support ap+scheme display [iwgetid]
 *	o iwevent man page [iwevent]
 *		(From Guus Sliepen <guus@warande3094.warande.uu.nl>)
 *	o iwgetid man page [iwgetid]
 *	---
 *	o Add "#define WIRELESS_EXT > 13" around event code [iwlib]
 *	o Move iw_enum_devices() from iwlist.c to iwlib.c [iwlib]
 *	o Use iw_enum_devices() everywhere [iwconfig/iwspy/iwpriv]
 *		(From Pavel Roskin <proski@gnu.org>, rewrite by me)
 *	o Return proper error message on non existent interfaces [iwconfig]
 *	o Read interface list in /proc/net/wireless and not SIOCGIFCONF [iwlib]
 *	---
 *		(From Pavel Roskin <proski@gnu.org> - again !!!)
 *	o Don't loose flags when setting encryption key [iwconfig]
 *	o Add <time.h> [iwevent]
 *	---
 *		(From Casey Carter <Casey@Carter.net>)
 *	o Improved compilations directives, stricter warnings [Makefile]
 *	o Fix strict warnings (static func, unused args...) [various]
 *	o New routines to display/input Ethernet MAC addresses [iwlib]
 *	o Correct my english & spelling [various]
 *	o Get macaddr to compile [macaddr]
 *	o Fix range checking in max number of args [iwlist]
 *	---
 *	o Display time when we receive event [iwevent]
 *	---
 *	o Display time before event, easier to read [iwevent]
 *		(From "Dr. Michael Rietz" <rietz@mail.amps.de>)
 *	o Use a generic set of header, may end header mess [iwlib]
 *		(From Casey Carter <Casey@Carter.net>)
 *	o Zillions cleanups, small fixes and code reorg [all over]
 *	o Proper usage/help printout [iwevent, iwgetid, ...]
 *	---
 *	o Send broadcast address for iwconfig ethX ap auto/any [iwconfig]
 *	---
 *	o Send NULL address for iwconfig ethX ap off [iwconfig]
 *	o Add iw_in_key() helper (and use it) [iwlib]
 *	o Create symbolink link libiw.so to libiw.so.XX [Makefile]
 *		(From Javier Achirica <achirica@ttd.net>)
 *	o Always send TxPower flags to the driver [iwconfig]
 *		(From John M. Choi <johnchoi@its.caltech.edu>)
 *	o Header definition for Slackware (kernel 2.2/glibc 2.2) [iwlib]
 *
 * wireless 25 :
 * -----------
 *	o Remove library symbolic link before creating it [Makefile]
 *	o Display error and exit if WE < 14 [iwevent]
 *		(From Sander Jonkers <sander@grachtzicht.cjb.net>)
 *	o Fix iwconfig usage display to show "enc off" [iwconfig]
 *		(From Pavel Roskin <proski@gnu.org>)
 *	o Formating : add spaces after cell/ap addr [iwconfig]
 *	---
 *	o Do driver WE source version verification [iwlib]
 *		(From Pavel Roskin <proski@gnu.org>)
 *	o Cleanup user configurable options [Makefile]
 *	o add FORCE_WEXT_VERSION [Makefile]
 *	o Add uninstall directived [Makefile]
 *	o Cleanup version warnings [iwlib]
 *	o Fix iwconfig usage display to show "mode MODE" [iwconfig]
 *	o Replace "rm -f + ln -s" with "ln -sfn" in install [Makefile]
 *	---
 *	o Add various documentation in source code of [iwpriv]
 *	o Allow to get more than 16 private ioctl description [iwlib]
 *	o Ignore ioctl descriptions with null name [iwpriv]
 *	o Implement sub-ioctls (simple/iw_point) [iwpriv]
 *	---
 *	o Add DISTRIBUTIONS file with call for help [README]
 *	o Change iw_byte_size in iw_get_priv_size [iwlib]
 *	o Document various bugs of new driver API with priv ioctls [iwpriv]
 *	o Implement float/addr priv data types [iwpriv]
 *	o Fix off-by-one bug (priv_size <= IFNAMSIZ) [iwpriv]
 *	o Reformat/beautify ioctl list display [iwpriv]
 *	o Add "-a" command line to dump all read-only priv ioctls [iwpriv]
 *	o Add a sample showing new priv features [sample_priv_addr.c]
 *	o Update other samples with new driver API [sample_enc.c/sample_pm.c]
 *	---
 *	o Fix "iwpriv -a" to not call ioctls not returning anything [iwpriv]
 *	o Use IW_MAX_GET_SPY in increase number of addresses read [iwspy]
 *	o Finish fixing the mess of off-by-one on IW_ESSID_MAX_SIZE [iwconfig]
 *	o Do interface enumeration using /proc/net/dev [iwlib]
 *	---
 *	o Display various --version information [iwlib, iwconfig, iwlist]
 *	o Filled in Debian 2.3 & Red-Hat 7.3 sections in [DISTRIBUTIONS]
 *	o Filled in Red-Hat 7.2, Mandrake 8.2 and SuSE 8.0 in [DISTRIBUTIONS]
 *	o Display current freq/channel after the iwrange list [iwlist]
 *	o Display current rate after the iwrange list [iwlist]
 *	o Display current txpower after the iwrange list [iwlist]
 *	o Add BUILD_NOLIBM to build without libm [Makefile]
 *	o Fix infinite loop on unknown events/scan elements [iwlib]
 *	o Add IWEVCUSTOM support [iwevent, iwlist]
 *	o Add IWEVREGISTERED & IWEVEXPIRED support [iwevent]
 *		(From Pavel Roskin <proski@gnu.org>)
 *	o Make $(DYNAMIC_LINK) relative (and not absolute) [Makefile]
 *	---
 *	o Replace all float occurence with double [iwlib, iwlist]
 *	o Implement iwgetid --mode [iwgetid]
 *	o Convert frequency to channel [iwlist, iwlib]
 *		(Suggested by Pavel Roskin <proski@gnu.org> - always him !)
 *	o Implement --version across the board [iwspy, iwevent, iwpriv]
 *	o Implement iwgetid --freq [iwgetid]
 *	o Display "Access Point/Cell" [iwgetid]
 *	---
 *	o New manpage about configuration (placeholder) [wireless.7]
 *	o Catch properly invalid arg to "iwconfig ethX key" [iwconfig]
 *	o Put placeholder for Passphrase to key conversion [iwlib]
 *	o Allow args of "iwconfig ethX key" in any order [iwconfig]
 *	o Implement token index for private commands [iwpriv]
 *	o Add IW_MODE_MONITOR for passive monitoring [iwlib]
 *		I wonder why nobody bothered to ask for it before ;-)
 *	o Mention distribution specific document in [PCMCIA]
 *	o Create directories before installing stuff in it [Makefile]
 *	---
 *	o Add Debian 3.0 and PCMCIA in [wireless.7]
 *	o Add iw_protocol_compare() in [iwlib]
 *	---
 *	o Complain about version mistmatch at runtime only once [iwlib]
 *	o Fix IWNAME null termination [iwconfig, iwlib]
 *	o "iwgetid -p" to display protocol name and check WE support [iwgetid]
 *
 * wireless 26 :
 * -----------
 *	o #define IFLA_WIRELESS if needed [iwlib]
 *	o Update man page with SuSE intruction (see below) [wireless.7]
 *		(From Alexander Pevzner <pzz@pzz.msk.ru>)
 *	o Allow to display all 8 bit rates instead of 7 [iwlist]
 *	o Fix retry lifetime to not set IW_RETRY_LIMIT flag [iwconfig]
 *		(From Christian Zoz <zoz@suse.de>)
 *	o Update SuSE configuration instructions [DISTRIBUTIONS]
 *	---
 *	o Update man page with regards to /proc/net/wireless [iwconfig.8]
 *	o Add NOLIBM version of iw_dbm2mwatt()/iw_mwatt2dbm() [iwlib]
 *	---
 *	o Fix "iwconfig ethX enc on" on WE-15 : set buffer size [iwconfig]
 *	o Display /proc/net/wireless before "typical data" [iwspy]
 *		(From Pavel Roskin <proski@gnu.org>)
 *	o Fix uninstall [Makefile]
 *	o Change "Encryption mode" to "Security mode" [iwconfig/iwlist]
 *	---
 *	o Add kernel headers that will be removed from wireless.h [iwlib]
 *	o Remove IW_MAX_GET_SPY, people should use AP-List [iwspy]
 *	o Re-word List of "Access Points" to "Peers/Access-Points" [iwlist]
 *	o Add support for SIOCGIWTHRSPY event [iwevent/iwlib]
 *	o Add support for SIOCSIWTHRSPY/SIOCGIWTHRSPY ioctls [iwspy]
 *	---
 *	o Add SIOCGIWNAME/Protocol event display [iwlist scan/iwevent]
 *	o Add temporary encoding flag setting [iwconfig]
 *	o Add login encoding setting [iwlib/iwconfig]
 *	---
 *	o Fix corruption of encryption key setting when followed by another
 *	  setting starting with a valid hex char ('essid' -> 'E') [iwlib]
 *	o Fix iw_in_key() so that it parses every char and not bundle of
 *	  two so that 'enc' is not the valid key '0E0C' [iwlib]
 *	o Fix parsing of odd keys '123' is '0123' instead of '1203' [iwlib]
 *	---
 *	o Implement WE version tool redirector (need WE-16) [iwredir]
 *	o Add "make vinstall" to use redirector [Makefile]
 *	o Fix compilation warning in WE < 16 [iwlib, iwspy]
 *	o Allow to specify PREFIX on make command line [Makefile]
 *	---
 *	o Update wireless.h (more frequencies) [wireless.h]
 *	o Allow to escape ESSID="any" using "essid - any" [iwconfig]
 *	o Updated Red-Hat 9 wireless config instructions [DISTRIBUTIONS]
 *		(From Pavel Roskin <proski@gnu.org>)
 *	o Replace all %d into %i so we can input hex/oct [iwlib, iwpriv]
 *	---
 *	o If >= WE-16, display kernel version in "iwconfig --version" [iwlib]
 *		(From Antonio Vilei <francesco.sigona@unile.it>)
 *	o Fix "wrq.u.bitrate.value = power;" => txpower [iwconfig]
 *		(From Casey Carter <Casey@Carter.net>)
 *	o Make iwlib.h header C++ friendly. [iwlib]
 *	---
 *		(From Pavel Roskin <proski@gnu.org>)
 *	o Make sure that KERNEL_SRC point to a valid directory [Makefile]
 *	o Print message if card support WE but has no version info [iwlib]
 *		(From Simon Kelley <simon@thekelleys.org.uk>)
 *	o If power has no mode, don't print garbage [iwlib]
 *	---
 *		(Bug reported by Guus Sliepen <guus@sliepen.eu.org>)
 *	o Don't cast "int power" to unsigned long in sscanf [iwconfig]
 *		(From Pavel Roskin <proski@gnu.org>)
 *	o Add $(LDFLAGS) for final linking [Makefile]
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
 *	Add an "auto" flag to have the driver cache the last n results
 *
 * iwlist :
 * ------
 *	Add scanning command line modifiers
 *	More scan types support
 *
 * iwevent :
 * -------
 *	Make it non root-only
 *
 * Doc & man pages :
 * ---------------
 *	Update main doc.
 *
 * Other :
 * -----
 *	What about some graphical tools ?
 */
