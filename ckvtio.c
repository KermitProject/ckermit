#ifndef VMS
# error -- CKVTIO.C is used only on the VMS(tm) or OpenVMS(tm) Operating System
#endif /* VMS */

/* REMINDER: "help system $blah" gives help about sys$blah() */

#if defined(__x86_64)
# define CKVTIO_OS_ARCH_STRING " OpenVMS(tm) x86_64"
#else /* defined(__x86_64) */
# if defined(__ia64) || defined(__ia64__)
#  define CKVTIO_OS_ARCH_STRING " OpenVMS(tm) IA64"
# else /* defined(__ia64) || defined(__ia64__) */
#  if defined(__ALPHA) || defined(__alpha)
#   define CKVTIO_OS_ARCH_STRING " OpenVMS(tm) Alpha(tm)"
         /* do nothing */
#  else /* defined(__ALPHA) || defined(__alpha) */
#   ifdef VAX
#    define CKVTIO_OS_ARCH_STRING " OpenVMS(tm) VAX(tm)"
#   else /* def VAX */
#    ifdef __GNUC__
#     define CKVTIO_OS_ARCH_STRING " OpenVMS(tm) VAX(tm) (GCC)"
#    else /* def __GNUC__ */
#     ERROR -- CKVTIO.C unknown architecture - Not Alpha, IA64, VAX, or x86_64
#    endif /* def __GNUC__ */
#   endif /* def VAX */
#  endif /* defined(__ALPHA) || defined(__alpha) */
# endif /* defined(__ia64) || defined(__ia64__) */
#endif /* defined(__x86_64) */

/*
  Module version number and date.
  Also update the module number above accordingly.
*/
char *ckxv = "Communications I/O 10.0.125, 4 October 2022";

/*
  This is the default architecture and operating system herald string.
  It is redetermined dynamically in sysinit() below.
*/
char *ckxsys = CKVTIO_OS_ARCH_STRING;

/*  C K V T I O  --  Terminal and Interrupt Functions for VAX/VMS  */

/* C-Kermit interrupt, terminal control & i/o functions for VMS systems */

/*
  Author: Frank da Cruz <fdc@columbia.edu>
  The Kermit Project, Columbia University, New York City.

  Copyright (C) 1985, 2022,
    Trustees of Columbia University in the City of New York.
    All rights reserved.  See the C-Kermit COPYING.TXT file or the
    copyright text in the ckcmai.c module for disclaimer and permissions.
*/

/* Edit History
 *
 * Originally adapted to VMS by:
 * Stew Rubenstein, Harvard University Chemical Labs, 1985.
 *
 * Cast of characters
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * sms  Steven M Schweda
 * cdh  Charles Don Hall  The Wollongong Group
 * cf   Carl Friedberg    Comet & Co., NYC
 * dbs  David B Sneddon
 * DS   Dan Schullman     Digital
 * fdc  Frank da Cruz     Columbia U
 * HG   Hunter Goatley    Western Kentucky University and Process Software
 * jea  Jeffrey Altman    Columbia U
 * jh   James Harvey      Indiana / Purdue University
 * js   James Sturdevant  (thru 1994)
 * js   John Santos       (1996-...)
 * lh   Lucas Hart        Oregon State University
 * LT   Lee Tibbert       Digital
 * mab  Mark Buda         Digital
 * mlo  Mike O'Malley     Digital
 * MM   Martin Minow      Digital (died 21 Dec 2000)
 * mpjz Martin PJ Zinzer  Gesellschaft fuer Schwerionenforschung GSI Darmstadt
 * tmk  Terry Kennedy     Saint Peters College and tmk.com
 * ttj  Tarjei T. Jensen  Norwegian Hydrographic Service
 * wb   William Bader     Lehigh University
 * wjm  Wolfgang J Moeller DECUS Germany
 * mv   Mortin Vorlaender PDV-Systeme Goslar Germany
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 006  8-May-85 MM   Got rid of "typeahead buffer" code as it didn't
 *                    solve the problem of data overruns at 4800 Baud.
 *                    Added vms "read a char" routine that checks for
 *                    CTRL/C, CTRL/Z, etc.
 * 007 16-May-85 fdc  Changed calling convention of ttopen(), make it
 *                    set value of its argument "lcl", to tell whether
 *                    C-Kermit is in local or remote mode.
 * 008 11 Jun 85 MM   Fix definition of CTTNAM
 * 009 18 Jun 85 fdc  Move def of CTTNAM to ckcdeb.h so it can be shared.
 * 010 25 Jun 85 MM   Added sysinit() to open console.
 * 011  5 Jul 85 DS   Treat hangup of closed line as success.
 * 012 11 Jul 85 fdc  Add gtimer(), rtimer() for timing statistics.
 * 013 14 Sep 87 fdc  Add parity strip to ttinl(), add syscleanup().
 * 014 14 Feb 89 mab  Make break REALLY work.  Add IOSB's to all QIO's.
 * 015 26 Feb 89 mab  Add dcl exit handler and minor cleanup
 * 016 23-Mar-89 mab  Add IO$M_BREAKTHRU to IO$_WRITEVBLK.
 *                    Add zchkspd() function to check for valid speed.
 * 017 04-Apr-89 mab  Fix some minor bugs to local/remote code
 * 018 23-Apr-89 mab  Add some of Valerie Mates' parity changes.
 * 019 25-Apr-89 mab  Change baud to 110 for V4.x break routine as
 *                    50 baud is not supported on most Muxes by VMS.
 * 020 13-Jun-89 mab  Fix on exquota problem on qiow(readvblk)
 * 021 08-Jul-89 mab  Add ^C/^Y abort server mode code.
 * 022 11-May-90 mab  Add V5A code
 * 023 20-Jul-90 wb   Add support for old VAX C and VMS versions
 * 024 22-Sep-90 wb   Fixes to cps/bps confusion
 * 025 26-Sep-90 tmk  Fix the ztime() function
 * 026 29-Sep-90 tmk  Edit 024 cause a server command to give some blither-
 *                    ings about unsupported line speeds.  Added a simple hack
 *                    to exit quietly if the passed speed is 0.  Adventurous
 *                    maintainers may want to look in ttpkt(), where ttsspd()
 *                    is being called with a value of -1/10.
 * 027 11-Oct-90 fdc  Made ttol return number of chars successfully written.
 *                    Made ztime() use ctime() in all cases.
 *                    Another fix (from tmk) for bps/cps confusion.
 *                    Wrapped source lines longer than 80 characters.
 * 028 18-Oct-90 fdc  Added comments to concb() and vms_getchar() to show
 *                    how to make Ctrl-C trap work.  Didn't actually do it,
 *                    though, because Ctrl-Y apparently still can't be caught.
 *                    Also, more minor reformatting.  Adjust time() declare.
 *                    Added support for automatic parity sense in ttinl(),
 *                    within #ifdef PARSENSE conditionals.  Built with PARSENSE
 *                    defined, works ok.
 * 029  5-Apr-91 fdc  Extensive reworking of many routines to allow for
 *                    network connections, addition of TGV MultiNet support.
 * 030 31-Aug-91 tmk  Fix problem with INPUT statements not timing out due to
 *                    extraneous rtimer() inside gtimer().
 * 032  6-Nov-91 fdc  Correct parity problem.
 * 032  6-Nov-91 fdc  Cosmetic cleanup in sysinit().
 * 033 14-Jan-91 fdc  Fix to get_qio_maxbuf_size to prevent crashes:
 *                    remove "&" from "!test_qio(ttychn,max,&tmpbuf)",
 *                    from John Schultz at 3M.
 * 034  8-Feb-92 fdc  Don't change EIGHTBIT setting in ttvt, concb, or conbin.
 *                    Set EIGHTBIT in ttpkt only if parity is not NONE.  From
 *                    Alan Robiette, Oxford U, UK.
 * 035 10-Jun-92 fdc  Added code from Ray Hunter of The Wollongong Group to
 *                    support both WIN/TCP and TGV Multinet.  Network section
 *                    of contti() redone to eliminate polling loop.  It's
 *                    infinitely faster.
 * 036 11-Jun-92 tmk  Fixed up edit 034 so 8-bit characters could be passed
 *                    in connect mode.
 * 037 19-Jun-92 fdc  Totally rewrote all the serial input and mode-setting
 *                    routines in this module to use nonblocking, fully
 *                    buffered input and to share a common buffer.  All
 *                    serial-line input is localized to a single routine,
 *                    txbufr(), which, in turn is known only to ttinc().  The
 *                    other input routines, ttxin() and ttinl(), simply call
 *                    ttinc().  ttchk() and ttflui() are totally cognizant of
 *                    the buffer.  ttinl() now recognizes packets with
 *                    printable start characters and/or lacking terminators,
 *                    so VMS C-Kermit can now engage in "Doomsday Kermit"
 *                    protocol.  ttvt() and ttpkt() were merged into a single
 *                    new (static) routine, ttbin(), which no longer puts the
 *                    device into PASALL mode (which defeats flow control).
 *                    Added ttsndlb() to send a Long BREAK.  Much fine-tuning,
 *                    testing, and filling-in remains to be done, including
 *                    (a) make ttopen() and ttclos() aware of LAT devices; (b)
 *                    check remaining BYTLM quota before issuing a read, (c)
 *                    integrate network and serial buffers, and much more.
 *                    Anyway, this code seems to run faster than ever before,
 *                    and for the first time I can actually use sliding
 *                    windows AND long packets on my 8-year old MicroVAX-II.
 * 038 28-Jun-92 tmk  Additional work on edit 37, general cleanup of old defs.
 * 039  1-Jul-92 wb   Changes for VMS 4.4.
 * 040  4-Jul-92 tmk  Add modem signal support (ttgmdm routine).
 * 041  4-Jul-92 tmk  Add tgetent(), worker routine for el-cheapo curses.
 * 042  4-Jul-92 jh   Enable typeahead in ttbin().
 * 043 21-Aug-92 fdc  Make default flow control be KEEP instead of Xon/Xoff.
 * 044  6-Sep-92 fdc  Put default flow back to Xon/Xoff, but allow KEEP to be
 *                    used to restore device's original flow-control setting.
 * 045 23-Sep-92 fdc  Add sleep(1) to tthang().  Seems to fix HANGUP command.
 *                    Suggested by Lee Tibbert.  Change ttbin() to use global
 *                    flow variable rather than its flow parameter for setting
 *                    flow control, to ensure the desired type of flow control
 *                    is used during DIAL operations.
 * 046 26-Sep-92 fdc  Change sleep(1) in tthang() to sleep(3).  Annoying but
 *                    necessary.  IO$M_HANGUP takes about 3 seconds, but the
 *                    sys$qiow() function returns immediately instead of
 *                    waiting for the action to complete.
 * 047 08-Oct-92 HG   Add call to sys$alloc in ttopen() to prevent user with
 *                    SHARE from getting port in use.  Some add'l cleanup.
 * 048 12-Oct-92 LT   Minor changes to support DEC TCP/IP (nee UCX).
 * 049 25-Oct-92 fdc  Adapt (ck_)cancio() to DEC TCP/IP.
 *                    Remove superfluous ttflui() call from ttpkt().
 *                    Add code from Lee Tibbert to sysinit() to figure out OS
 *                    and architecture name at runtime.
 * 050 18-Nov-92 fdc  Read from comm device in 1024-byte chunks, rather than
 *                    trusting the qio_maxbuf_size.  This should reduce BYTLM
 *                    quota-exceeded errors.  Suggested by tmk as a temporary
 *                    workaround.
 * 051 10-May-93 fdc  Add support for SET TRANSFER CANCELLATION.
 * 052 16-May-93 fdc  Change VMSTCPIP to TCPIPLIB to agree with new CKCNET.H.
 * 053 16-May-93 fdc  ANSIfication for GNU CC, from James Sturdevant.
 * 054 08-Jun-83 fdc  Add TT$M_LOCALECHO and TT$M_ESCAPE to the terminal modes
 *                    we handle, to prevent "getty babble" with modems, VAX
 *                    PSI, etc.
 * 055 16-Jun-93 fdc  Edit 054 only affected ttbin().  This edit does the same
 *                    for conbin() and concb().  Fixes double echoing in
 *                    command mode when coming in via VAX PSI.
 * 056  8-Aug-93 fdc  Add types to all function declarations.
 * 057 17-Aug-93 fdc  Add GET_SDC macro as in CKVIOC.C, accounting for GCC.
 *                    From Tarjei T. Jensen <tarjeij@extern.uio.no>.
 * 058 27-Sep-93 HG   Fix for real the SHARE issue when allocating terminal
 *                    by dropping SHARE before trying to assign the channel.
 * 059  7-Oct-93 mlo  Added support for CMU-OpenVMS/IP ("CMU/Tek").  Requires
 *                    CMU-OpenVMS/IP sockets library, also by mlo.
 * 060  9-Oct-93 HG   Fix improper call to vms_assign_channel in edit 058,
 *                    noticed by Fritz@GEMS.VCU.EDU.
 * 061  9-Oct-93 fdc  For some reason, conbin() was turning off flow control.
 *                    This caused massive data loss during CONNECT mode when
 *                    running C-Kermit from a low-speed connection through
 *                    a DECserver.  Now conbin() leaves the console flow
 *                    control setting alone.
 * 062 10-Oct-93 ttj  Change parameters to time() and ctime() from long to
 *                    time_t (pointers).  Made prototype for vms_assign_channel
 *                    and caught an erroneous function call.
 * 063  9-Nov-93 fdc  In sysinit(), don't run sys$getdviw() on the "console"
 *                    if the console is not a terminal (from tcwkw@sf.msc.edu).
 *                    And (blame this one on me) _never_ set the backgrd flag.
 * 064 25-Nov-93 fdc  Fixed coninc(n) to return -1 on timeout.
 * 065  9-Dec-93 fdc  Fix transfer cancellation to account for parity, and
 *                    allow it only in remote mode.
 * 066 13-Dec-93 fdc  Make debug logging in ttol() show why packet writes fail.
 * 067 15-Dec-93 fdc  New, MAXBUF-proof ttol() recovers from failures by
 *                    writing the packet in chunks, whose size is computed
 *                    dynamically.  This seems to cause no noticable slowdown
 *                    in the transfer rate.
 * 068 31-Dec-93 fdc  Fix bug in parity-detection code in ttinl().
 * 069 14-Dec-93 fdc  Add ttgwsiz() routine, code from John Berryman.
 * 070 14-MAR-94 mlo  CMU_TCPIP modifications: contti - return error (-1)
 *                    if number of characters read is zero; (ck_)cancio - pass
 *                    correct i/o channel for ttyfd to sys$cancel.
 * 071 15-MAR-94 mlo  ttol() - add #ifdef DEBUG for compiles with NODEBUG
 * 072 27-Mar-94 fdc  Straighten out some ttsspd()/ttgspd() confusion.
 * 073 11-AUG-94 fdc  Make conoll() return a proper return code.
 * 074 12-AUG-94 fdc  Make syscleanup() handle conres() error.  Make
 * 075 20-AUG-94 js   Make congm() get terminal type, etc, to remove annoying
 *                    "Sorry, terminal type not supported" messages when
 *                    running from a .COM file, etc.
 * 076 02-Sep-94 fdc  Call con_cancel() in syscleanup() to cancel any pending
 *                    console i/o, hopefully eliminating zombies after
 *                    after disconnection, etc.
 * 077 22-Sep-94 mlo  Don't call sys$cancel() in (ck_k)cancio() on CMU/Tek
 *                    network connections - it breaks the connection.
 * 078 24-Feb-95 mpjz Fix for DECC on VAX.
 * 079 08-Feb-96 fdc  Add symbols for higher serial speeds.
 * 080 30-May-96 fdc  Fix netopen() to work with Rlogin.
 * 081 25-Aug-96 mpjz More DECC/VAXC fixups.
 * 082 05-Sep-96 fdc  Remove #module, change (ck_)cancio() declaration.
 * 083 06-Sep-96 fdc  Fix loss of parity setting on network connections.
 * 084 06-Sep-96 cf   Add missing speeds to ttspeeds[] (Carl Freidberg)
 * 085 06-Sep-96 fdc  Add conspd() routine (for use by RLOGIN).
 * 086 06-Sep-96 fdc  Try to fix parity confusion again.
 * 087 06-Sep-96 fdc  And again: culprit is DECC sign-extension: see ttinc, etc
 * 088 06-Sep-96 fdc  Unravel the network-file-descriptor reusability tangle
 * 089 23-May-97 wjm  CMUIP & related fixes from Wolfgang Moeller
 * 090 23-May-97 cdh  Patches for Wollongong/Attachmate Pathway
 * 091 12-Jul-97 fdc  Try to avoid or at least identify terrible things that
                      happen when (a) we have made a serial connection and the
                      other side hangs it up (and so C-Kermit hangs in ttclos),
                      and (b) when running in a DECwindow and user closes the
                      DECwindow and so any attempt access to the console gets
                      a DEVOFFLINE error, which in turn causes an error msg,
                      which in turn gets another DEVOFFLINE error, etc, until
                      the system grinds to halt.
 * 092  6-Sep-97 fdc  Add ttspdlist() and startupdir[].
 * 093 20-Sep-97 js   CMU/IP fixes (applied by fdc, hopefully correctly).
 * 094 11-Dec-97 fdc  In ttgmdm(), omit controller-type test for Alpha.
 *                    In txbufr(), treat hangup synchronously.
 * 095 22-Dec-97 fdc  Add rftimer() and gftimer(), with help from James Puzzo.
 *                    Fix ttclos() not to set network == 0, which makes
 *                    subsequent ttchk() take the wrong path.
 * 096  1-Jan-98 fdc  Add ok_to_share variable for ttopen() to get around
 *                    an otherwise insuperable problem with DECIntact.
 *                    Turn off broadcasts in conbin().  Set program name.
 * 097 15-Jan-98 fdc  Don't set program name after all.  Use sys$getjpiw()
 *                    instead of sys$getdviw("sys$login:") to get batch /
 *                    interactive status.  More corrections to txbufr() carrier
 *                    treatment.  Ditto for ttchk().
 * 098  1-Feb-98 fdc  Fix contti() to not return a spurious character when
 *                    CARRIER-WATCH is OFF and and sys$qiow() gives SS$_HANGUP
 *                    and no character; return special code -2 and set up to
 *                    queue a new read.
 * 099  8-Apr-98 fdc  Add uname info in sysinit().  Commented out #includes
 *                    for gftimer() in OLD_VMS (from lh).
 * 100  3-May-98 fdc  Fix out-of-bounds tt_fulldevnam[]array reference.
 * 101  7-May-98 lh   Improvements in hardware-name getting.
 * 102  5-Jul-98 fdc  Illegal to take address of constant in GCC, reworked in
 *                    gftimer() (from Kevin Handy).
 * 103 25-Nov-98 fdc  Add support for CLOSE-ON-DISCONNECT.
 * 104  9-Dec-98 fdc  Add support for NETLEBUF (see ckcnet.h).
 * 105  5-Mar-99 fdc  Parameterize device name length TTNAMLEN.
 *                    Fix ttchk() to avoid spurious "OK to exit?" after
 *                    remote has hung up on us.
 * 106 27-Apr-99 lh   Fix GCC warnings in gftimer().
 * 107 22-Jul-99 fdc  Add code to sysinit() to get username.
 * 108 23-Sep-99 fdc  Fix batch echoing & DCL image data execution.
 * 109 18-Oct-99 fdc  Add setting of wasclosed variable.
 * 110  3-Dec-00 fdc  Add Telnet Com Port Option.
 * 111  7-Jun-01 lh   Fix off-by one ckstrncpy() into unm_mch[] in sysinit().
 * 112 21-Jul-01 hg   Improved congm() detects better if console is terminal.
 * 113  2-Nov-01 fdc  Allow REDIAL Telnet terminal server in ttopen().
 * 114 10-Nov-01 lh   Fix bad pointer reference at end of ztime().
 * 115 31-Dec-01 lh   Fix checks for batch/background again.
 * 116  1-Jan-02 fdc  Check for -z and -B command-line option in congm().
 * 117  3-Jan-02 dbs  Changed testing of batch to use itsatty.  There seemed
 *                    to be some inconsistency about the testing of batch and
 *                    itsatty which both appear to be indicating the same
 *                    thing.  Changed the intial setting of itsatty to 1 since
 *                    the initial setting of batch was 0.  Also modify the
 *                    setting of backgrd to match batch - not quite sure why
 *                    there are three flags to indicate the same thing.
 *                    congm sets the three flags, but on return from congm
 *                    they were reset based on isatty(0) - commented out this
 *                    second attempt.  Commented out the CK_USEGETJPI stuff
 *                    to set interactive determination to use sys$input rather
 *                    than JPI$_TERMINAL.
 * 118  8-Feb-02 fdc  Version string.
 * 119 24-Oct-02 jea  SSL support code (not complete).
 * 120 24-Oct-02 jea  More SSL support code (still not complete).
 * 121-122 ???        (not sure -- probably finishing up SSL support?).
 * 123 15-Jun-03 mv   cmdate2tm() function (for HTTP).
 * 124 05-Apr-04 fdc  Add __ia64 arch_string.
 */

/*
 Variables available to outside world:

   dftty  -- Pointer to default tty name string, like "/dev/tty".
   dfloc  -- 0 if dftty is console(remote), 1 if external line(local).
   dfprty -- Default parity
   dfflow -- Default flow control
   ckxech -- Flag for who echoes console typein:
     1 - The program (system echo is turned off)
     0 - The system (or front end, or terminal).
   functions that want to do their own echoing should check this flag
   before doing so.

   backgrd
     Flag indicating program not executing interactively.
     Used to ignore INT and QUIT signals, suppress messages, etc.
   vms_status
     Status returned by most recent VMS system service,
     can be used for error reporting.
   batch
     Nonzero if running in batch job, 0 otherwise.
   itsatty
     Nonzero if sys$input is the terminal, 0 if batch job or started with @.

 Functions for assigned communication line (either external or console tty):

   ttopen(ttname,local,mdmtyp) -- Open the named tty for exclusive access.
   ttclos()                -- Close & reset the tty, releasing any access lock.
   ttpkt(speed,flow)       -- Put the tty in packet mode and set the speed.
   ttvt(speed,flow)        -- Put the tty in virtual terminal mode.
                                or in DIALING or CONNECTED modem control state.
   ttinl(dest,max,timo)    -- Timed read line from the tty.
   ttinc(timo)             -- Timed read character from tty.
   ttchk()                 -- See how many characters in tty input buffer.
   ttxin(n,buf)            -- Read n characters from tty (untimed).
   ttol(string,length)     -- Write a string to the tty.
   ttoc(c)                 -- Write a character to the tty.
   ttflui()                -- Flush tty input buffer.
   tt_cancel()             -- Cancel any asynch I/O to tty
*/

/*
Functions for console terminal:
   congm()   -- Get console terminal modes.
   concb(esc) -- Put the console in single-character wakeup mode with no echo.
   conbin(esc) -- Put the console in binary (raw) mode.
   conres()  -- Restore the console to mode obtained by congm().
   conoc(c)  -- Unbuffered output, one character to console.
   conol(s)  -- Unbuffered output, null-terminated string to the console.
   conola(s) -- Unbuffered output, array of lines to the console, CRLFs added.
   conxo(n,s) -- Unbuffered output, n characters to the console.
   conchk()  -- Check if characters available at console (bsd 4.2).
                Check if escape char (^\) typed at console (System III/V).
   coninc(timo)  -- Timed get a character from the console.
   conint()  -- Enable terminal interrupts on the console if not background.
   connoi()  -- Disable terminal interrupts on the console if not background.
   contti()  -- Get a character from either console or tty, whichever is first.

Time functions

   msleep(m) -- Millisecond sleep
   ztime(&s) -- Return pointer to date/time string
   rtimer()  -- Reset elapsed time counter
   gtimer()  -- Get elapsed time
*/

/* Includes */
#include "ckcdeb.h"                     /* Formats for debug() */
#include "ckcasc.h"
#include "ckcker.h"

#include "ckvvms.h"
#ifdef CK_SSL
#include "ck_ssl.h"
#endif /* CK_SSL */

#include <stdio.h>                      /* Unix Standard i/o */
#include <jpidef.h>
#include <signal.h>                     /* Interrupts */
#include <setjmp.h>                     /* Longjumps */
#include <iodef.h>
#include <ttdef.h>
#include <tt2def.h>
#include <ssdef.h>
#include <descrip.h>
#include <dvidef.h>
#include <dcdef.h>
#include <devdef.h>

#ifdef CMU_TCPIP          /* in case CMU types.h is in search list, rather */
#include <types.h>        /* than added to SYS$LIBRARY:VAXCDEF.TLB, include */
#endif /* CMU_TCPIP */    /* it before <time.h> includes the TLB types.h */

#include <time.h>
#include <syidef.h>
#include <prvdef.h>

typedef struct {
    int val[2];
} QUAD;

/* lt 1992-10-08 Begin
 */
#ifndef __DECC                          /* (was __ALPHA) */
# ifndef __GNUC__
#  define void int
# endif /* __GNUC__ */
#endif /* __DECC */
/* lt 1992-10-08 End
 */

#ifdef OLD_VMS
#define IO$_TTY_PORT 41
#ifdef COMMENT
/* Now it's tested -- so don't */
#include <libdtdef.h>                   /* For gftimer() - Not tested! */
#include <lib$routines.h>               /* ditto */
#endif /* COMMENT */
#else
#include <starlet.h>
#include <libdtdef.h>                   /* OK in VMS 5.x & above */
#include <lib$routines.h>               /* ditto */
#endif /* OLD_VMS */

/* Macros */

#define xx_inc(timo) (--ttxbn>=0?(unsigned)(ttxbuf[ttxbp++]&0xff):txbufr(timo))

/* Declarations */

#ifndef VMS64BIT
#ifndef MULTINET
    time_t time();
#endif	/* MULTINET */
#endif	/* VMS64BIT */
    char *ctime();                      /* Convert to asctime() string */

    void dcl_exit_h();                  /* Exit handler */
    unsigned short vms_assign_channel(struct dsc$descriptor_s *ttname);
    VOID tt_cancel();
/*
  This is the device name for the console.  When we use TT: it evidently fouls
  us up under Batch, in DCL, SPAWN'd, etc.  When we use SYS$INPUT, it prevents
  CONNECT from working in DCL procedures that do not "$ DEFINE SYS$INPUT
  SYS$COMMAND".  When we use SYS$COMMAND, it allows CONNECT to work in DCL
  procedures, but it forces the command parser to read from the terminal, and
  therefore prevents inclusion of Kermit commands as "image data" in DCL
  procedures.  (Dave Sneddon, January 2002.)
*/
#define CONDEVICE    "SYS$INPUT"
#define CONDEV_COLON "SYS$INPUT:"

/* Note: another approach might be to get JPI$_TERMINAL from SYS$GETJPI */

#ifdef VMSV60				/* i.e. VMS 6.0 or later */
/*
  The #ifdef was added in 8.0.201 (Feb 2002).  The aforementioned change seems
  to work fine in later VMS versions, but in 5.5 (and presumably other pre-6.0
  versions) it totally breaks Kermit, to the point it won't even start.  The
  scheme used here was tested successfully on VMS 5.5 and 7.1 on VAX, and
  VMS 6.2, 7.1, 7.2, and 7.3 on Alpha.
*/
    char *dftty = CTTNAM;
#else
    char *dftty = "TT:";
#endif /* VMSV60 */
    int dfloc = 0;                      /* Default location is local */
    int dfprty = 0;                     /* Parity (0 = none) */
    int ttprty = 0;                     /* Parity in use */
    int ttpflg = 0;                     /* Parity not sensed yet. */
    static int ttpmsk = 0xff;           /* Communication device parity mask */
    int ttmdm = 0;                      /* Modem in use. */
    int dfflow = FLO_XONX;              /* Default flow control is Xon/Xoff */
    int backgrd = 0;                    /* Running in "background" (no tty) */
    int batch = 0;                      /* Assume not batch */
    int itsatty = 1;                    /* isatty(0) result */
    int ttcarr = CAR_AUT;               /* Carrier Handling Mode */
    int tvtflg = 0;                     /* Flag that ttvt has been called */
    long ttspeed = -1;                  /* For saving speed */
    long conspd = -1;                   /* Console terminal speed */
    int ttflow = -9;                    /* For saving flow */
    static int ttdialing = 0;           /* Flag on when dialing */

int ckxech = 0; /* 0 if system normally echoes console characters, else 1 */

static int xhangup = 0;                 /* Nonzero if hangup detected */
int overruns = 0;                       /* Data overruns detected */

unsigned int vms_status = SS$_NORMAL;   /* System service return status */
unsigned int vms_lasterr = SS$_NORMAL;  /* Last error */

/* Structures used within this module */

#ifndef TT$C_BAUD_38400                 /* For VMS versions higher than */
#define TT$C_BAUD_38400 17              /* the one I'm compiling on... */
#endif /* TT$C_BAUD_38400 */

#ifndef TT$C_BAUD_57600
#define TT$C_BAUD_57600 18
#endif /* TT$C_BAUD_57600 */

#ifndef TT$C_BAUD_76800
#define TT$C_BAUD_76800 19
#endif /* TT$C_BAUD_76800 */

#ifndef TT$C_BAUD_115200
#define TT$C_BAUD_115200 20
#endif /* TT$C_BAUD_115200 */

static struct {
    unsigned char dec;
    unsigned short int line;
    } ttspeeds[] = {  /* Comments show TT$C_BAUD_xx symbol values */
        {TT$C_BAUD_50,       5},        /*  1 */
        {TT$C_BAUD_75,       7},        /*  2 */
        {TT$C_BAUD_110,     11},        /*  3 */
        {TT$C_BAUD_134,     13},        /*  4 */
        {TT$C_BAUD_150,     15},        /*  5 */
        {TT$C_BAUD_300,     30},        /*  6 */
        {TT$C_BAUD_600,     60},        /*  7 */
        {TT$C_BAUD_1200,   120},        /*  8 */
        {TT$C_BAUD_1800,   180},        /*  9 */
        {TT$C_BAUD_2000,   200},        /* 10 */
        {TT$C_BAUD_2400,   240},        /* 11 */
        {TT$C_BAUD_3600,   360},        /* 12 */
        {TT$C_BAUD_4800,   480},        /* 13 */
        {TT$C_BAUD_7200,   720},        /* 14 */
        {TT$C_BAUD_9600,   960},        /* 15 */
        {TT$C_BAUD_19200, 1920},        /* 16 */
        {TT$C_BAUD_38400, 3840},        /* 17 */
        {TT$C_BAUD_57600, 5760},        /* 18 */
        {TT$C_BAUD_76800, 7680},        /* 19 */
        {TT$C_BAUD_115200,11520},       /* 20 */
        {0,                   0} };

/* Declarations of variables global within this module */

/* was long... */
#define TIME_T time_t
#ifdef __ALPHA
#ifdef WINTCP
#include <socket_aliases.h>
#undef TIME_T
#define TIME_T long
#endif /* WINTCP */
#endif /* __ALPHA */

static TIME_T tcount = 0;               /* For timing statistics */

static char brkarray[] = {              /* For simulating BREAK */

 '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
 '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
 '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
 '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};

int ttyfd = -1;                         /* TTY file descriptor */

#ifdef COMMENT /* old */
static int conif = 0,                   /* Console interrupts on/off flag */
    conclass = 0,                       /* Console device type */
    cgmf = 0,                           /* Flag that console modes saved */
    xlocal = 0,                         /* Flag for tty local or remote */
    ttychn = 0,                         /* TTY i/o channe; */
    conchn = 0,                         /* Console i/o channel */
    con_queued = 0,                     /* console i/o queued in contti() */
    tt_queued = 0,                      /* tty i/o queued in contti() */
    conch,                              /* console input character buffer  */
    curcarr = 0,                        /* Carrier mode: require/ignore */
    ttch;                               /* tty input character buffer */
#else /* new */
static unsigned short
    ttychn = 0,                         /* TTY i/o channe; */
    conchn = 0,                         /* Console i/o channel */
    con_queued = 0,                     /* console i/o queued in contti() */
    tt_queued = 0;                      /* tty i/o queued in contti() */

static int conif = 0,                   /* Console interrupts on/off flag */
    conclass = 0,                       /* Console device type */
    cgmf = 0,                           /* Flag that console modes saved */
    xlocal = 0,                         /* Flag for tty local or remote */
    conch,                              /* console input character buffer  */
    curcarr = 0,                        /* Carrier mode: require/ignore */
    ttch;                               /* tty input character buffer */
#endif /* COMMENT */

static unsigned char escchr;            /* Escape or attn character */
static char ttnmsv[TTNAMLEN+1];         /* copy of open path for tthang */
char cttnam[TTNAMLEN+1];                /* Controlling terminal name */
static char tt_fulldevnam[TTNAMLEN+1];
static struct dsc$descriptor_s tt_fulldevnam_d; /* Descriptor for line name */

static long int qio_maxbuf_size;        /* Maximum size of QIO to succeed */
static long dclexh_status;              /* Exit status for DCL exit handler */
static struct iosb_struct coniosb, ttiosb, wrk_iosb;
static struct tt_mode
    ttold, ttraw, tttvt,                /* for communication line */
    ccold, ccraw, cccbrk,               /* and for console */
    cctmp;

/* Network support */

#include "ckcnet.h"                     /* Network type symbols */
extern int ttnet;                       /* Network type */
static int network = 0;                 /* 1 if network connection */
#ifdef TCPSOCKET
extern int ttnproto;
#endif /* TCPSOCKET */
extern int xfrcan, xfrchr, xfrnum;      /* Transfer cancellation */
extern int initflg, wasclosed;

/*
  Select proper library function for getting socket device channel.
*/
#ifdef TCPIPLIB
#if defined (__DECC)
# define GET_SDC (short)decc$get_sdc
#elif (defined (VAXC) || defined (__VAXC) || defined (__GNUC__))
# define GET_SDC vaxc$get_sdc
#else
# error unknown compiler, not DECC and not VAXC
#endif /* __DECC */
#endif /* TCPIPLIB */

/* Needed for parity fixes in edit 036 */
extern int parity;                      /* current parity setting */

/*
  New buffered input scheme.
*/
#define TTXBUF

#ifdef TTXBUF
#define TTXBUFL RBSIZ                   /* Internal buffer size */

CHAR    ttxbuf[TTXBUFL+1];              /* The buffer */
int     ttxbp = 0, ttxbn = 0;           /* Buffer pointer and count */
int     ok_to_share = 0;                /* Can be set by user interface */

int get_qio_maxbuf_size(unsigned long int ttychn);
int test_qio(unsigned long int ttychn, long int max, unsigned char *dest);
int ttispd();

/*
  T X B U F R

  Read bytes from communication device into internal buffer ttxbuf[].
  To be called only when input buffer is empty, i.e. when ttxbn == 0.

  Other comm-device reading routines, like ttinc, ttinl, ttxin, should check
  the internal buffer first, and call this routine for a refill if necessary.

  When data is read successfully, the first character is returned and
  the global buffer count, ttxbn, is set to the number of characters remaining
  in ttxbuf after it, and the global buffer offset, ttxbp, is set to 1.

  When data is not read successfully, -1 is returned indicating a timeout,
  or -2 indicating disconnection.
*/
int
txbufr(timo) int timo; {                /* TT Buffer Read */
    int i, count;
    int func;                           /* Read function code */
    int mask;                           /* Event mask */
    int vms_status;                     /* Read QIO return code */
    static int trmmsk[2] = {0,0};       /* Terminal read break mask */
    static int trmlong[8] = {0,0,0,0,0,0,0,0}; /* Break on nothing */
    char tmpbuf[16];

    debug(F101,"txbufr entry, ttxbn","",ttxbn);
    if (ttxbn > 0) {                    /* Should not be called */
        debug(F101,"txbufr called with ttxbn","",ttxbn); /* if ttxbn > 0! */
        ttxbn--;
        return(ttxbuf[ttxbp++] & 0xff);
    }
    ttxbp = ttxbn = 0;                  /* Reset buffer pointer and count */
    ttxbuf[0] = NUL;                    /* and the buffer itself */

    if (timo < 0)                       /* Be safe */
      timo = 0;
    debug(F101,"txbufr timo","",timo);

    func = IO$_READVBLK | IO$M_NOFILTR; /* Read virtual block, no filtering */
    trmmsk[0] = sizeof(trmlong);        /* No terminators */
    trmmsk[1] = (int)&trmlong;          /* Keep all characters */
/*
  We try to scoop up as many as we can in a nonblocking read (timed, but with
  timeout value of 0).  This is supposed to return immediately with up to
  "count" characters placed in our buffer.
*/
    count = TTXBUFL;                    /* Maximum characters to read */

#ifdef COMMENT
/*
  This causes problems because we are not adjusting according to the CURRENT
  BYTLM quota, but rather to the one that was obtained when Kermit started.
  Since the quota is dynamic, it might have been reduced since then.
*/
    if (count > qio_maxbuf_size)
        count = qio_maxbuf_size;
#else
/*
  So for now we use 1024, which tends to be smaller than the value obtained
  above.  Later, this should be changed to find out the remaining BYTLM quota
  and use that instead (unless that is too expensive).  This size can be
  overridden at compile time by defining the symbol...
*/
#ifndef CKV_IO_SIZE
#define CKV_IO_SIZE 1024
#endif /* CKV_IO_SIZE */
    if (count > CKV_IO_SIZE)
        count = CKV_IO_SIZE;
#endif /* COMMENT */

    debug(F101,"txbufr count","",count);

    for (i = 0; i < 2; i++) {           /* Two tries... */
        sprintf(tmpbuf,"txbufr %d",i);
        if (i == 0)
          /* First read is nonblocking, timed, with timeout of 0 */
          vms_status = sys$qiow(QIOW_EFN, ttychn,
                                func|IO$M_TIMED, &wrk_iosb, 0, 0,
                                ttxbuf, count, 0, &trmmsk, 0, 0);
        else
          /* Second read is blocking, for 1 byte, using the timeout given */
          vms_status = sys$qiow(QIOW_EFN, ttychn,
                                func, &wrk_iosb, 0, 0,
                                ttxbuf, 1, timo, &trmmsk, 0, 0);

        if (!(vms_status & 1)) vms_lasterr = vms_status;
        debug(F111,tmpbuf,"sys$qiow status",vms_status);

#ifdef SS$_DATAOVERUN
        if (vms_status == SS$_DATAOVERUN) { /* This shouldn't be fatal */
            overruns++;
            debug(F111,tmpbuf,"data overrun",overruns);
        } else
#endif /* SS$_DATAOVERUN */
        if (vms_status != SS$_NORMAL && vms_status != SS$_HANGUP) {
            debug(F111,tmpbuf,"fatal error",vms_status);
            return(-2);
        }
        if (vms_status == SS$_HANGUP && ttcarr != CAR_OFF) {
            xhangup = 1;                /* Remember for next time */
            debug(F111,tmpbuf,"sys$qiow SS$_HANGUP",SS$_HANGUP);
            debug(F111,tmpbuf,"sets xhangup",xhangup);
        }
        debug(F011,"txbufr ttxbuf",ttxbuf,wrk_iosb.size);
        debug(F101,"txbufr iosb.size","",wrk_iosb.size);
        debug(F101,"txbufr iosb.status","",wrk_iosb.status);
/*
  Did we get some data?  Even if SS$_HANGUP was indicated, we might also have
  received some data at the same time (like a NO CARRIER message); if we did,
  return it now and catch the HANGUP next time around.
*/
        if (wrk_iosb.size > 0) {
            ttxbn = wrk_iosb.size;      /* Set buffer count. */
            ttxbn--;                    /* Less one for the one we return */
            return(ttxbuf[ttxbp++] & 0xff); /* Return it, bump offset */
        }
/*
  Check for hangup only if no data was obtained.  Reports indicate that
  that SS$_HANGUP is not indicated more than once, hence the flag.
*/
        if (xhangup > 0) {              /* Hangup detected last time */
            debug(F110,tmpbuf,"xhangup return",0);
            return(-2);
        } else if (wrk_iosb.status == SS$_HANGUP) { /* Or detected this time */
            debug(F111,tmpbuf,"SS$_HANGUP: ttcarr",ttcarr);
            debug(F111,tmpbuf,"SS$_HANGUP: ttdialing",ttdialing);
            if (ttcarr != CAR_OFF && !ttdialing) {
                /* Don't set xhangup here because apparently VMS returns */
                /* this error even when the connection has not hung up. */
                /* Experimentation shows that setting xhangup here prevents */
                /* file transfer from working on dialout connections. */
                /* (VMS 7.1, Alpha PWX 433/au) */
                /* xhangup = 1; */
                debug(F110,tmpbuf,"fatal: hangup and CARRIER-WATCH not OFF",0);
                return(-2);
            }
        } else if (wrk_iosb.status != SS$_TIMEOUT) {
            debug(F111,tmpbuf,"fatal: unexpected iosb status",wrk_iosb.status);
            return(-2);                 /* Call it a hangup */
        }
/*
  No error and no characters either, so next time around try a blocking,
  possibly timed, read for a single character.  Since this routine will be
  called again very soon, the first read with a zero timeout will have the
  rest of the user data in it.  Thus, this isn't as inefficient as it first
  appears.
*/
        if (timo > 0)
          func |= IO$M_TIMED;
    } /* Go back for second loop iteration */

/* Get here only if second read timed out - i.e. no characters arrived. */

    debug(F101, "txbufr timed out","",timo);
    return(-1);
}

/*  T T I N C  --  Read a character from the communication device  */
/*
  ttinc() maintains an internal buffer to minimize system calls.
  Returns the next character, or -1 if there is a timeout, or -2
  on communications disconnect.  Calls txbufr() to refill its buffer
  when necessary.
*/
int
ttinc(timo) int timo; {
    int x; unsigned char c;

#ifdef NETCONN
    if (network) {
#ifdef COMMENT
        int x;
        x = netinc(timo);
        if ((ttnproto == NP_TELNET) && (x > -1))
          return((unsigned)(x & 0xff));
        else
          return(x < 0 ? x : (unsigned)(x & ttpmsk));
#else
        return(netinc(timo));
#endif /* COMMENT */
    }
#endif /* NETCONN */

    debug(F101,"ttinc ttxbn","",ttxbn);
    if (ttxbn > 0) {                    /* Something in internal buffer? */
        ttxbn--;                        /* Yes, deduct one from count. */
        c = ttxbuf[ttxbp++];            /* Return the next character. */
        debug(F101,"ttinc returns c","",c);
        return((unsigned)(c & 0xff));
    } else if ((x = txbufr(timo)) < 0) { /* No, fill buffer */
        debug(F101,"ttinc txbufr failed","",x); /* Pass along failure. */
        return(x);
    } else {                            /* Success. */
        debug(F101,"ttinc returns x","",x);
        return((unsigned)(x & 0xff));   /* Return the character */
    }
}

/*  T T X I N  --  Get n bytes from tty input buffer  */
/*
  Call with n = number of bytes to get, buf = where to put them.

  This routine assumes that the given number of bytes is available
  and will not return until they are gotten.  You should only call this
  routine after calling ttchk to find out how many bytes are waiting to
  to be read.

  Returns:
  -1 on error, number of chars gotten on success.
*/
int
ttxin(n,buf) int n; CHAR *buf; {
    int i, x;

    debug(F101,"ttxin","",n);

#ifdef NETCONN
    if (network) {
        for (i = 0; i < n; i++) {
            if ((x = ttinc(0)) < 0) return(-1);
            buf[i] = (char) x;
        }
    } else {
#endif /* NETCONN */
/* xx_inc() is a macro */
        for (i = 0; i < n; i++) {
            if ((x = xx_inc(0)) < 0) return(-1);
            buf[i] = (char) x;
        }
#ifdef NETCONN
    }
#endif /* NETCONN */
    buf[i] = NUL;
    return(i);
}

/*  T T F L U I  --  Flush communication device input buffer  */

int
ttflui() {
    int n;
    debug(F100,"ttflui","",0);
#ifdef NETCONN
    if (network)
      return(netflui());
#endif /* NETCONN */

    ttxbn = ttxbp = 0;                  /* Flush internal buffer *FIRST* */
    if ((n = ttchk()) > 0) {
        debug(F101,"ttflui count","",n);
        while ((n--) && xx_inc(2) >= 0) ; /* Ignore Warning - see comments */
    }
    return(0);
    /*
       NOTE: the comparison of xx_inc(2) with -1 reportedly causes problems
       with overzealous compilers because xx_inc() is a macro that can
       return a value that is cast as (unsigned).  But it can also return
       the return value of txbufr(), which is signed.  Changing the comparison
       to >= 0 makes no difference.  It's a warning we'll have to live with.
    */
}

/*  T T C H K  --  Check how many bytes are waiting to be read */
/*
  Returns number of bytes waiting, or < 0 if connection has dropped.
  The number of bytes waiting includes those in our internal buffer plus
  those in VMS's internal input buffer.
*/
int                                     /* Check how many bytes are ready */
ttchk() {                               /* for reading from network */
    static struct {
        unsigned short count;
        unsigned char first;
        unsigned char reserved1;
        long reserved2;
    } ttchk_struct;

#ifdef NETCONN
    if (network) {                      /* Network connection */
        debug(F101,"ttchk network ttyfd","",ttyfd);
        if (ttyfd < 0)
          return(-2);
        else
          return(nettchk());
    }
#endif /* NETCONN */

    debug(F101,"ttchk ttxbn","",ttxbn);
    debug(F101,"ttchk xhangup","",xhangup);

    if (xlocal && xhangup)
      return(-2);

    if (!ttychn) {
        debug(F101,"ttchk called with no ttychn","",0);
        return(0);
    }

    if (
#ifdef COMMENT
/*
  I tried something like this in UNIX and it didn't work at all so I'm
  leaving it as a comment here until it can be tested.  - fdc, Jan 98.
*/
        ttxbn < 1 &&
#endif /* COMMENT */
        xlocal && ttcarr != CAR_OFF) { /* No data in buffer */
        int x;                  /* Serial connection */
        extern int clsondisc;
        x = ttgmdm();           /* with carrier checking */
        debug(F101,"ttchk ttgmdm","",x);
        if (x > -1) {           /* Then we better have carrier */
            if (!(x & BM_DCD)) {
                debug(F101,"ttchk carrier lost","",x);
                if (clsondisc) {        /* If "close-on-disconnect" */
                    debug(F100,"ttchk close-on-disconnect","",0);
                    ttclos(0);  /* close device & release it. */
                }
                return(-2);
            }
        }
    }
    vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_SENSEMODE|IO$M_TYPEAHDCNT,
                          &wrk_iosb, 0, 0, &ttchk_struct,
                          sizeof(ttchk_struct), 0, 0, 0, 0
                          );
    if (!(vms_status & 1)) vms_lasterr = vms_status;
#ifdef DEBUG
    if (deblog) {
        debug(F101,"ttchk sys$qiow status","",vms_status);
        debug(F101,"ttchk count","",(int)ttchk_struct.count);
        if (ttchk_struct.count)
          debug(F101,"ttchk first","",(int)ttchk_struct.first);
    }
#endif /* DEBUG */
    return(vms_status & 1 ? ttchk_struct.count + ttxbn : ttxbn);
}

#ifdef CTRLC
#undef CTRLC
#endif /* CTRLC */
#define CTRLC '\03'

#ifndef NOXFER
/*  T T I N L  --  Read a packet from the communications connection.  */
/*
  Reads up to "max" characters from the communication line, terminating on:

    (a) the packet length field if the "turn" argument is zero, or
    (b) on the turnaround character (turn) if the "turn" argument is nonzero
    (c) n repetitions of the interruption character (3 ^C's by default)

  and returns the number of characters read upon success, or if "max" was
  exceeded or the timeout interval expired before (a) or (b), returns -1.

  The characters that were input are copied into "dest" with their parity bits
  stripped if parity was selected.  Returns the number of characters read.
  Characters after end of packet are available upon the next call to this
  function.
*/
int
#ifdef CK_ANSIC
ttinl(CHAR *dest, int max, int timo, CHAR eol, CHAR start, int turn)
#else
ttinl(dest,max,timo,eol,start,turn) int max,timo,turn; CHAR *dest,eol,start;
#endif /* CK_ANSIC */
/* ttinl */ {
    int x, y, c, i, j;
    int ccn = 0;                /* Control C counter */
    int flag;
    int cc;
    unsigned char *cp;
    int pktlen = -1;
    int lplen = 0;
    int havelen = 0;

    debug(F101,"ttinl start","",start);
    debug(F101,"ttinl turn","",turn);
    debug(F101,"ttinl timo","",timo);
    i = j = flag = 0;
#ifdef COMMENT
    ttpmsk = (ttprty) ? 0177 : 0377;    /* Set parity stripping mask. */
#endif /* COMMENT */
    debug(F101,"ttinl loop entry, network","",network);
    while (i < max) {
        cc = network ? netinc(timo) : xx_inc(timo); /* Read a byte */
        if (cc < 0) {
            debug(F101,"ttinl cc","",cc);
            if (cc == -1 || cc == -2) {
                return(cc);
            } else {                    /* I hate C ... */
                debug(F100,
                      "ttinl: SIGN EXTENSION BOTCH - FIX SOURCE CODE","",0
                      );
                /* This doesn't really help because 255 becomes -1, etc. */
                cc &= 0xff;
            }
        }

        /* Check for cancellation */
        if (!xlocal && xfrcan && ((cc & ttpmsk) == xfrchr)) {
            if (++ccn >= xfrnum) {      /* If xfrnum in a row, bail out. */
                fprintf(stderr,"^C...\r\n"); /* Echo Ctrl-C */
                return(-2);
            }
        } else ccn = 0;                 /* No cancel, so reset counter, */

        if ((flag == 0) && ((cc & 0x7f) == start)) {
            debug(F100,"ttinl got start","",0);
            flag = 1;                   /* Got packet start. */
        }
        if (flag) {                     /* If we're in a packet... */
            dest[i++] = cc & ttpmsk;
            if ((cc & 0x7f) == eol) { /* Stop at eol. */
                debug(F101,"ttinl got eol, i","",i);
                break;
            }
        }
/*
  If we have not been instructed to wait for a turnaround character, we
  can go by the packet length field.  If turn != 0, we must wait for the
  end of line (eol) character before returning.
*/
#ifndef xunchar
#define xunchar(ch) (((ch) - 32 ) & 0xFF )      /* Character to number */
#endif /* xunchar */

        if (i == 2) {
            pktlen = xunchar(dest[1] & 0x7f);
            havelen = (pktlen > 1);
            debug(F101,"ttinl length","",pktlen);
        } else if (i == 5 && pktlen == 0) {
            lplen = xunchar(dest[4] & 0x7f);
        } else if (i == 6 && pktlen == 0) {
            pktlen = lplen * 95 + xunchar(dest[5] & 0x7f) + 5;
            havelen = 1;
            debug(F101,"ttinl length","",pktlen);
        }
        if (havelen && !turn && (i > pktlen+1)) { /* Use length field */
            debug(F101,"ttinl break on length","",i);
            break;
        }
    }
    dest[i] = '\0';                     /* Terminate the string */
    debug(F101,"ttinl loop done, i","",i);
    debug(F101,"ttinl max","",max);
    debug(F101,"ttinl dest[i-1]","",dest[i-1]);
    debug(F101,"ttinl eol","",eol);

    if (i >= max) {
        debug(F100,"ttinl buffer overflow","",0);
        return(-1);     /* Overflowed dest buffer without getting eol */
    }
    x = i;                              /* Size. */
    dest[x] = '\0';                     /* Terminate with null */

    debug(F110,"ttinl packet",dest,0);
    debug(F101,"ttinl size","",x);      /* Log the size */
    debug(F101,"ttinl ttprty 1","",ttprty);

    if (ttpflg++ == 0 && ttprty == 0) { /* Check and adjust the parity. */
        if ((ttprty = parchk(dest,start,x)) > 0) {
            debug(F000,"ttinl parchk senses parity","",ttprty);
            ttpmsk = 0x7f;
            for (i = 0; i < x; i++)     /* Strip parity from this packet */
              dest[i] &= 0x7f;
        }
        if (ttprty < 0) ttprty = 0;     /* Restore if parchk error */
        debug(F101,"ttinl ttprty 2","",ttprty);
    }
    return(x);                          /* Return length */
}
#endif /* NOXFER */
#endif /* TTXBUF */

sig_t saval;               /* For saving alarm handler */

VOID
ttimoff() {                             /* Turn off any timer interrupts */
    alarm(0);
    if (saval) {
        signal(SIGALRM,saval);
        saval = NULL;
    } else {
        signal(SIGALRM,SIG_IGN);        /* (was SIG_DFL) */
    }
}

/*  P R I N T _ M S G  --  Log an error message from VMS  */

int
print_msg(s) char *s; {
    long int blen = 0;
    char buf[PMSG_BUF_SIZE], msg[PMSG_MSG_SIZE];
    struct dsc$descriptor_s b = {
        PMSG_BUF_SIZE-1,
        DSC$K_DTYPE_T,
        DSC$K_CLASS_S,NULL
    };

    b.dsc$a_pointer = (char *)&buf;
    vms_status = sys$getmsg(vms_status, &blen, &b, 0, 0);
    if (!(vms_status & 1)) {
        vms_lasterr = vms_status;
        fprintf(stderr,"print_msg; sys$getmsg\n");
        return(-1);
    }
    buf[blen] = '\0';
    sprintf(msg, "%s: %s\n", s, buf);
    debug(F100,s,"",0);
    ermsg(msg);
    return(0);
}

/*  S Y S I N I T  --  System-dependent program initialization.  */

#ifndef DVI$_FULLDEVNAM
#define DVI$_FULLDEVNAM 232
#endif /* DVI$_FULLDEVNAM */

#ifndef DVI$_STS
#define DVI$_STS 226
#endif /* DVI$_STS */

int
ttgwsiz() {                             /* Get console window (screen) size */
    int x = -1;
    extern int tt_rows, tt_cols;
    typedef struct {                    /* define an item list struct */
        short length;                   /* length of buffer */
        short code;                     /* item code */
        void *ptr;                      /* ptr to buffer */
        void *retlen;                   /* ptr to return length */
    } item_list;
    int status, iosb[2];
    item_list tt_dvi[] = {              /* Item list for GETDVI */
        {4,DVI$_DEVBUFSIZ,&tt_cols,0},
        {4,DVI$_TT_PAGE,&tt_rows,0},
        {0,0,0,0}
    };
    $DESCRIPTOR(sysin,CONDEVICE);
    status = sys$getdviw(0,0,&sysin,&tt_dvi,&iosb,0,0,0);
    if (!(status & 1)) {
        vms_lasterr = status;
        return(-1);
    }
    if (tt_rows < 1 || tt_cols < 1) return(0);
    return(1);
}

#define CK_SYSNMLN 31
char unm_mch[CK_SYSNMLN+1] = { '\0', '\0' };
char unm_mod[CK_SYSNMLN+1] = { '\0', '\0' };
char unm_nam[CK_SYSNMLN+1] = { '\0', '\0' };
char unm_rel[CK_SYSNMLN+1] = { '\0', '\0' };
char unm_ver[CK_SYSNMLN+1] = { '\0', '\0' };

int
sysinit() {
    extern int speed;
    extern char ttname[], uidbuf[];
#ifdef CK_ENVIRONMENT
    extern char tn_env_sys[];
#endif /* CK_ENVIRONMENT */
    extern char startupdir[];
    extern char *ckzsys, *ck_s_name, *ck_s_ver;
    int i, x, n;
    char * p;
    struct dsc$descriptor_s prcname;
    char ckname[24];

    static struct desblk {
        long int *fl;                   /* Forward link.  Used by VMS only */
        void (*fncpnt)();               /* Function to call */
        unsigned char argcnt;           /* Only one arg allowed */
        unsigned char filler[3];        /* Filler.  Must be zero */
        long int *sts;                  /* Address of sts (written by VMS) */
    } dclexh_ = {0,dcl_exit_h,1,{0,0,0},&dclexh_status};

#define GETCKXSYS
/*
  Get architecture and operating system name.
*/
#ifdef GETCKXSYS

/* OK, we have a VAX so what is the name of the OS? */

#ifndef SYI$_ARCH_NAME    /* Should be in syidef.h but is not there yet */
# define SYI$_ARCH_NAME 4454
#endif /* SYI$_ARCH_NAME */

    struct iosb_t {
        short int status;               /* System service status */
        short int unused[3];
    } iosb;

    struct itmlst_t {
        short unsigned int buffer_len;  /* Buffer length */
        short unsigned int item_code;   /* Item code */
        char *buffer;                   /* Where to write SYI info */
        long unsigned int *ret_len;     /* Pointer to returned length */
        long unsigned int mbz;          /* Must Be Zero */

    } itmlst;

    struct itmlst dviitm[] = {{64,DVI$_FULLDEVNAM,(char *)&cttnam,0},
                        {sizeof(conclass),DVI$_DEVCLASS,(char *)&conclass,0},
                                {0,0,0,0}};

    long unsigned int ret_len;          /* Returned length */
/*
  $getsyi of "hw_arch" will fail prior to VMS 5.5.  This failure indicates that
  the OS name is "VAX/VMS" (sic).  Use success or failure of $getsyi "hw_arch"
  rather than the more straight forward $getsyi "node_swvers" because latter
  is defined as four (4) characters and will get strange representing VMS
  10.0.
*/

/*  Default */

#ifdef __x86_64
    ckxsys = " OpenVMS x86_64";
#else /* def __x86_64 */
# ifdef __ia64
    ckxsys = " OpenVMS IA64";
# else /* def __ia64 */
#  ifdef __ALPHA
    ckxsys = " OpenVMS Alpha";
#  else /* def __ALPHA */
    ckxsys = " OpenVMS VAX";
#  endif /* def __ALPHA */
# endif /* def __ia64 */
#endif /* def __x86_64 */

    itmlst.buffer_len = CK_SYSNMLN;
    itmlst.item_code = SYI$_ARCH_NAME;
    itmlst.buffer = unm_mch;
    itmlst.ret_len = &ret_len;
    itmlst.mbz = 0;

    if ((sys$getsyiw (0, 0, 0, &itmlst, &iosb, 0, 0) & 1) == 1)
      if ((iosb.status & 1) == 1)
#ifdef __x86_64
        ckxsys = " OpenVMS x86_64";
#else /* def __x86_64 */
# ifdef __ia64
        ckxsys = " OpenVMS IA64";
# else /* def __ia64 */
#  ifdef __ALPHA
        ckxsys = " OpenVMS Alpha";
#  else /* def __ALPHA */
        ckxsys = " OpenVMS VAX";
#  endif /* def __ALPHA */
# endif /* def __ia64 */
#endif /* def __x86_64 */

    if (unm_mch[0] == '\0') {           /* supply a default */
        for (p = ckxsys; *p == ' '; p++) ;
        ckstrncpy(unm_mch,p,4);
    }
    for (p = ckxsys; *p == ' '; p++) ;  /* Strip leading blanks */
    ckstrncpy(unm_nam,p,CK_SYSNMLN);    /* For uname info */
    ckzsys = ckxsys;                    /* Same deal for file module */

#ifdef CK_ENVIRONMENT
    strcpy(tn_env_sys,"VMS");
#ifdef CK_SNDLOC
    {
        extern char * tn_loc;
        char *p;
        if (p = getenv("LOCATION"))
          if (tn_loc = (char *)malloc((int)strlen(p)+1))
            strcpy(tn_loc,p);
    }
#endif /* CK_SNDLOC */
#endif /* CK_ENVIRONMENT */

    itmlst.buffer_len = CK_SYSNMLN;     /* Get hardware model */
#ifdef SYI$_HW_NAME                     /* as of V5.0 ? */
    itmlst.item_code = SYI$_HW_NAME;
#else
#ifdef SYI$_NODE_HWTYPE
    itmlst.item_code = SYI$_NODE_HWTYPE;
#else
    itmlst.item_code = 0;               /* shouldn't happen... */
#endif /* SYI$_NODE_HWTYPE */
#endif /* SYI$_HW_NAME */
    itmlst.buffer = unm_mod;
    itmlst.ret_len = &ret_len;
    itmlst.mbz = 0;
    x = sys$getsyiw (0, 0, 0, &itmlst, &iosb, 0, 0);

    itmlst.buffer_len = CK_SYSNMLN;     /* Get VMS release */
    itmlst.item_code = SYI$_VERSION;
    itmlst.buffer = unm_rel;
    itmlst.ret_len = &ret_len;
    itmlst.mbz = 0;
    x = sys$getsyiw (0, 0, 0, &itmlst, &iosb, 0, 0);

    if ((x & 1) && unm_rel[0]) {        /* Just the major version number */
        for (p = unm_rel; *p && !isdigit(*p); p++) ;
        ckstrncpy(unm_ver,p,CK_SYSNMLN);
        p = unm_ver;
        while (isdigit(*p++)) ;
        *(p-1) = '\0';
    }
#endif /* GETCKXSYS */

/*
 * Set up DCL Exit handler.  This allows us to reset terminal
 * and any other modifications we have done.
 */
    debug(F101,"sysinit ttychn","",ttychn);
    debug(F101,"sysinit conchn","",conchn);
    if (!CHECK_ERR("sysinit: sys$dclexh",
                   sys$dclexh(&dclexh_))) {
        debug(F100,"sysinit failed to declare exit handler","",0);
#ifdef COMMENT
        return(0);
#endif /* COMMENT */
    }

    {                                   /* Get username */
        struct itmlstdef {
            short int buflen;           /* Length of buffer */
            short int itmcod;           /* Function code */
            char *bufaddr;              /* Address of buffer */
            long int *retlen;           /* Address to return actual length */
        };
        int i;
        char userbuf[UIDBUFLEN];        /* The book says 12 is max */
        long buflen = 0;                /* And it's 12 in VMS 7.1  */
        unsigned long user_flags;
        struct itmlstdef gjiitm[] = {
            UIDBUFLEN-1, JPI$_USERNAME, NULL, 0, 0, 0, 0, 0
        };
        gjiitm[0].bufaddr = userbuf;
        gjiitm[0].retlen = &buflen;
        for (i = 0; i < UIDBUFLEN; i++) userbuf[i] = 0;
        vms_status = sys$getjpiw(0, 0, 0, &gjiitm, 0, 0, 0);
        if (!(vms_status & 1)) vms_lasterr = vms_status;
        if (vms_status != SS$_NORMAL) {
            debug(F101,"sysinit sys$getjpiw error","",vms_status);
        } else {
            char c;
            debug(F111,"sysinit sys$getjpiw ok",userbuf,vms_status);
            for (i = 0; i < UIDBUFLEN; i++) {
                c = userbuf[i];
                if (c == ' ') c = '\0';
                uidbuf[i] = c;
                if (!c)
                  break;
            }
            debug(F111,"sysinit sys$getjpiw uidbuf",uidbuf,buflen);
        }
    }
    if (ttychn)                         /* if comms line already opened */
      return(0);                        /* (how could it be???) */

    if (!conchn) {                      /* Get console channel */
#ifdef CMU_TCPIP
      /* need to open console using libcmu routine to enable `select' call on
       * file descriptor zero.
       */
      cmu_stdin_open(dftty);
      conchn = cmu_get_sdc(0);
#else
      struct dsc$descriptor_s devnam =
                {sizeof(dftty)-1,DSC$K_DTYPE_T,DSC$K_CLASS_S,NULL};
      devnam.dsc$a_pointer = dftty;
      conchn = vms_assign_channel(&devnam);
#endif /* CMU_TCPIP */
    }
    congm();                            /* Get and save its modes */
/*
 * Parse console terminal device name.
 */
/*    itsatty = 0;
    if (isatty(0))
      {
      itsatty = 1;
      batch = 0;
      backgrd = 0;
      }
 */
    debug(F101,"sysinit itsatty","",itsatty);

    if (itsatty)
        {
        CHECK_ERR("sysinit: sys$getdviw",
                  sys$getdviw(0, conchn, 0, &dviitm, &wrk_iosb, 0, 0, 0));
        debug(F110,"sysinit cttnam",cttnam,0);

        if (!CHECK_ERR("sysinit: sys$qiow",
                       sys$qiow(QIOW_EFN, conchn, IO$_SENSEMODE,
                                &wrk_iosb, 0, 0,
                                &ccold, sizeof(ccold), 0, 0, 0, 0)))
          return(-1);
        ttspeed = speed = ttispd((unsigned char) wrk_iosb.size);
        conspd = ttspeed;
        debug(F111,"sysinit speed",cttnam,speed);
        ckstrncpy(ttname,cttnam,TTNAMLEN);
    }

    /* Initialize descriptor */
    tt_fulldevnam_d.dsc$b_dtype = DSC$K_DTYPE_T;
    tt_fulldevnam_d.dsc$b_class = DSC$K_CLASS_S;

    strcpy(startupdir, zgtdir());       /* Default directory at startup */

#ifdef CK_VMSSETNAME                    /* (Which is not defined) */
/*
  The problem with this is that it stays there even after Kermit exits.
  If anybody really cares to have "C-Kermit 6.1" show up in SHOW SYSTEM, etc,
  then we'll need to get the current name, save it, and then restore it in
  the exit handler.
*/
    ckname[0] = '\0';                   /* Process name */
    ckstrncpy(ckname,ck_s_name,22);     /* Copy program name from main() */
    strncat(ckname,"_",22);             /* Separator */
    strncat(ckname,ck_s_ver,22);        /* Version number */
    for (n = 0,i = 0; i < 24; i++) {    /* Chop off edit number */
        if (ckname[i] == '.')
          n++;
        if (n == 2) {
            ckname[i] = '\0';
            break;
        }
    }
    ckname[15] = '\0';                  /* Max length is 15 */
    n = strlen(ckname);
    prcname.dsc$w_length = n;           /* Convert to descriptor */
    prcname.dsc$a_pointer = (char *)ckname;
    prcname.dsc$b_class = DSC$K_CLASS_S;
    prcname.dsc$b_dtype = DSC$K_DTYPE_T;
    for (i = 1; i <= 9; i++) {          /* Try to set it */
        x = sys$setprn(&prcname);
        if (!(x & 1)) vms_lasterr = x;
        debug(F111,"sysinit sys$setprn",ckname,x);
        if (x == SS$_NORMAL)            /* Worked, done */
          break;
        if (x != SS$_DUPLNAM)           /* Anything else but duplicate name */
          break;
        if (n > 13)                     /* Add a suffix, 1..9, to name */
          break;
        ckname[n] = '_';
        ckname[n+1] = (char) (i + '0');
        ckname[n+2] = '\0';
        prcname.dsc$w_length = n+2;     /* Let VMS know the new length */
    }
#endif /* CK_VMSSETNAME */
    debug(F100,"sysinit done","",0);
    return(0);
}

char *
whoami() {
    extern char uidbuf[];
    return(uidbuf[0] ? (char *)uidbuf : "UNKNOWN");
}


/*
 * DCL Exit handler.  This is the cleanup handler for program.
 * Any final cleanup (closing channels etc) should be done at this
 * point.
 */
VOID
dcl_exit_h(sts) unsigned long int *sts; {
    syscleanup();
    return;
}

/*  S Y S C L E A N U P -- System-dependent program epilog.  */

int
syscleanup() {
    int x;
    extern int zclosf();
    extern void zrestoredir();
    void con_cancel();
    debug(F101,"syscleanup entry","",ttyfd);
    con_cancel();               /* Cancel pending console i/o. */
    ttclos(ttyfd);              /* Do the cleanup no matter what... */
    zclosf(ZSYSFN);             /* Close various files and kill child procs */
#ifdef COMMENT
    /* This is a bit extreme... */
    /*
       No it isn't.  Scenario: user closes DECwindow with Alt-F4 or whatever.
       The sys$qiow() call in conres() fails with %SYSTEM-F-DEVOFFLINE.
       If we don't exit now, this error will recur infinitely after we return,
       when we try to print any kind of message.  But we still keep this
       commented out because now (July 1997) we test for this in conres().
    */
    if ((x = conres()) < 0) exit(SS$_ABORT);
#else
    conres();
#endif /* COMMENT */
    zrestoredir();                      /* Restore startup directory */
    printf("\r");
    return(0);
}

/*  T T O P E N  --  Open a tty for exclusive access.  */

/*  Returns 0 on success, -1 on failure.  */
/*
  If called with lcl < 0, sets value of lcl as follows:
  0: the terminal named by ttname is the job's controlling terminal.
  1: the terminal named by ttname is not the job's controlling terminal.
  But watch out: if a line is already open, or if requested line can't
  be opened, then lcl remains (and is returned as) -1.
*/
static int ismodem = 0;
static int isremote = 0;
static int isdialup = 0;
static int isdisconnect = 0;
static int issecure = 0;
static int ishangup = 0;
static int ismodhangup = 0;

int
ttopen(ttname,lcl,modem,timo) char *ttname; int *lcl, modem, timo; {
    extern int speed;
    int s, x;
    unsigned long int no_share_priv[2], prev_privs[2];
    unsigned long int devchar, devclass, devsts;
 /* char dvibuf[65]; */
    struct dsc$descriptor_s devnam = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};
    struct itmlst dviitm[] = {{64,DVI$_FULLDEVNAM,(char *)&tt_fulldevnam,0},
                        {sizeof(devchar),DVI$_DEVCHAR,NULL,0},
                        {sizeof(devclass),DVI$_DEVCLASS,NULL,0},
                        {sizeof(devsts),DVI$_STS,NULL,0},
                        {0,0,0,0}};

#ifdef DEBUG
    if (deblog) {
        debug(F110,"ttopen ttname",ttname,0);
        debug(F110,"ttopen ttnmsv",ttnmsv,0);
        debug(F101,"ttopen modem","",modem);
        debug(F101,"ttopen network","",network);
        debug(F101,"ttopen ttychn","",ttychn);
        debug(F101,"ttopen ttyfd","",ttyfd);
    }
#endif /* DEBUG */

    dviitm[1].adr = (char *)&devchar;
    dviitm[2].adr = (char *)&devclass;
    dviitm[3].adr = (char *)&devsts;

    wasclosed = 1;

#ifdef NETCONN
    if (network && ttyfd > -1) {        /* if device already opened */
        if (!strncmp(ttname,ttnmsv,TTNAMLEN-1)) /* new & old names equal? */
          return(0);                    /* Nothing to do. */
        ttnmsv[0] = '\0';               /* No, poke this */
        network = 0;                    /* Undo this */
        ttclos(ttyfd);                  /* Close old connection. */
    }
    /* This is for REDIALing Telnet terminal servers... */

    if (!strcmp(ttname,ttnmsv) &&       /* Old and new names the same? */
        (network > 0) &&                /* Previous connection was network? */
        (ttmdm < 0)) {                  /* And we remember the network type? */
        int rc;
        rc = netopen(ttname, lcl, -ttmdm);
        debug(F111,"ttopen REOPEN netopen",ttname,rc);
        if (rc > -1) {
            xlocal = *lcl = 1;
        } else {
            network = 0;
        }
        return(rc);
    }
    /* General case - open a new network connection */

    if (modem < 0) {                    /* modem < 0 = special code for net */
        int x;
        ttmdm = modem;
        modem = -modem;                 /* Positive network type number */
        debug(F111,"ttopen net",ttname,modem);
        network = 1;                    /* Because rlog_ini() uses ttoc() */
        x = netopen(ttname, lcl, modem);
        if (x > -1) {                   /* Success... */
            ckstrncpy(ttnmsv,ttname,TTNAMLEN);
            ttnet = modem;
        } else                          /* Failed, unset network flag */
          network = 0;
        return(x);
    }
#endif /* NETCONN */

    if (ttychn) return(0);              /* Close channel if open */

/* Now we know we're opening a serial device */

    network = 0;                        /* So set this to zero */

    devnam.dsc$w_length  = strlen(ttname); /* Get real name of device */
    devnam.dsc$a_pointer = ttname;
    sys$getdviw(0, 0, &devnam, &dviitm, &wrk_iosb, 0, 0, 0);
    tt_fulldevnam[TTNAMLEN] = '\0';     /* Make sure name has an end... */

    if (devclass != DC$_TERM) {         /* Is it a terminal? */
        fprintf(stderr,
                "%%CKERMIT-W-NOTTERM, %s is not a terminal\n",ttname);
        return(-1);
    }
    if (!(devchar & DEV$M_AVL)) {       /* Is it available? */
        fprintf(stderr,
                "%%CKERMIT-W-NOTAVAL, %s is not available\n",tt_fulldevnam);
        return(-5);
    }
    if (!(devsts & UCB$M_ONLINE)) {     /* Is it online? */
        fprintf(stderr,
                "%%CKERMIT-W-OFFLINE, %s is not online\n",tt_fulldevnam);
        return(-5);
    }
    ttmdm = modem;                      /* Make this available to other fns */
    xlocal = *lcl;                      /* Make this available to other fns */
    xhangup = 0;                        /* New conneciton - no hangup yet */
    overruns = 0;                       /* Reset overrun counter */
/*
 *  Set up the tt_fulldevnam_d descriptor for use by ttclos() later.
 */
    tt_fulldevnam_d.dsc$w_length  = strlen(tt_fulldevnam);
    tt_fulldevnam_d.dsc$a_pointer = tt_fulldevnam;
/*
  In the old days, we just tried to assign the device, but this could result
  in two people having the same serial device open, which does nobody any
  good.  Starting in 1993, we dropped SHARE privilege before allocating the
  device to prevent this.  But in 1997 we found that this prevented C-Kermit
  from being used in remote mode under DECIntact, which allocates the TT:
  device for itself first, so a /SHARE qualifier was added to SET LINE, which,
  if given, sets ok_to_share to 1, and if not given sets it to 0.  Now we set
  or unset SHARE privilege based on this variable.  -fdc, 31 Dec 1997.
*/
    no_share_priv[0] = PRV$M_SHARE;
    debug(F101,"ttopen ok_to_share","",ok_to_share);
    vms_status = sys$setprv(ok_to_share ? 1 : 0, &no_share_priv,0,&prev_privs);
    if (!(vms_status & 1)) vms_lasterr = vms_status;
    debug(F101,"ttopen sys$setprv 1","",vms_status);
    ttychn = vms_assign_channel(&devnam); /* Get a channel for it. */
    debug(F101,"ttopen ttychn","",ttychn);
    vms_status = sys$setprv (1, &prev_privs, 0, 0); /* Restore old privs */
    if (!(vms_status & 1)) vms_lasterr = vms_status;
    debug(F101,"ttopen sys$setprv 2","",vms_status);
    if (!ttychn) return(-1);            /* Couldn't open, fail */
    wasclosed = 0;
/*
 * Check for maximum size of QIO, so as to not get the dreaded quota exceeded
 * status returned.  When doing a QIO that has a larger buffer than
 * MAXBUF, exceeded quota is returned.
 *
 * Example: MAXBUF = 2048, QIO = 1936, overhead is 112: will succeed.
 *          QIO of 1937 will fail.
 *
 * This can change for different versions of VMS.
 */
    qio_maxbuf_size = get_qio_maxbuf_size(ttychn);

    ckstrncpy(ttname,tt_fulldevnam,TTNAMLEN); /* Copy true name to main() */
    ckstrncpy(ttnmsv,ttname,TTNAMLEN);  /* Keep a copy of the name locally */
    ttxbn = ttxbp = 0;                  /* Initialize input buffer */

/* Caller wants us to figure out if line is controlling tty */

    debug(F111,"ttopen ok",ttname,*lcl);
    if (*lcl < 0) {
        if (conclass == DC$_TERM)
          xlocal = (strncmp(ttname,cttnam,TTNAMLEN) == 0) ? 0 : 1;
        else
          xlocal = 1;                /* If not a term, then we must be local */
        debug(F111,"ttopen controlling terminal",cttnam,xlocal);
    }
    if (!CHECK_ERR("ttopen: sys$qiow",
        sys$qiow(QIOW_EFN, ttychn, IO$_SENSEMODE, &wrk_iosb, 0, 0,
                 &ttold, sizeof(ttold), 0, 0, 0, 0))) return(-1);

    ttspeed = speed = ttispd((unsigned char) wrk_iosb.size);
    debug(F101,"ttopen speed","",speed);

/* Got the line, now set the desired value for local. */

    if (*lcl) *lcl = xlocal;

    tttvt = ttold;
    ttraw = ttold;

#ifdef TT$M_MODEM
    debug(F101,"ttopen TT$M_MODEM","",TT$M_MODEM);
    if (ttold.basic & TT$M_MODEM)
      ismodem = 1;
#endif /* TT$M_MODEM */
    debug(F101,"ttopen ismodem","",ismodem);
#ifdef TT$M_REMOTE
    debug(F101,"ttopen TT$M_REMOTE","",TT$M_REMOTE);
    if (ttold.basic & TT$M_REMOTE)      /* This really means "has carrier" */
      isremote = 1;                     /* if TT$M_MODEM */
#endif /* TT$M_REMOTE */
    debug(F101,"ttopen isremote","",isremote);
#ifdef TT2$M_DIALUP
    debug(F101,"ttopen TT2$M_DIALUP","",TT2$M_DIALUP);
    if (ttold.basic & TT2$M_DIALUP)
      isdialup = 1;
#endif /* TT$2M_DIALUP */
    debug(F101,"ttopen isdialup","",isdialup);
#ifdef TT2$M_DISCONNECT
    debug(F101,"ttopen TT2$M_DISCONNECT","",TT2$M_DISCONNECT);
    if (ttold.basic & TT2$M_DISCONNECT)
      isdisconnect = 1;
#endif /* TT$2M_DISCONNECT */
    debug(F101,"ttopen isdisconnect","",isdisconnect);
#ifdef TT2$M_SECURE
    debug(F101,"ttopen TT2$M_SECURE","",TT2$M_SECURE);
    if (ttold.basic & TT2$M_SECURE)
      issecure = 1;
#endif /* TT$2M_SECURE */
    debug(F101,"ttopen issecure","",issecure);
#ifdef TT2$M_HANGUP
    debug(F101,"ttopen TT2$M_HANGUP","",TT2$M_HANGUP);
    if (ttold.basic & TT2$M_HANGUP)
      ishangup = 1;
#endif /* TT$2M_HANGUP */
    debug(F101,"ttopen ishangup","",ishangup);
#ifdef TT2$M_MODHANGUP
    debug(F101,"ttopen TT2$M_MODHANGUP","",TT2$M_MODHANGUP);
    if (ttold.basic & TT2$M_MODHANGUP)
      ismodhangup = 1;
#endif /* TT$2M_MODHANGUP */
    debug(F101,"ttopen ismodhangup","",ismodhangup);

    debug(F101,"ttopen lcl","",*lcl);
    ttpmsk = 0xff;
    return(0);
}

#ifdef COMMENT
/*
  Old version.
*/
unsigned long int
vms_assign_channel(ttname) char *ttname;  {
    unsigned int channel = 0;
    struct dsc$descriptor_s d = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};

    d.dsc$w_length  = strlen(ttname);
    d.dsc$a_pointer = ttname;
    if (!CHECK_ERR("vms_assign_channel: sys$assign",
        sys$assign(&d, &channel, 0, 0))) return(0);
    return(channel);
}
#else
/*
  New version from Hunter Goatley.
*/
unsigned short int
vms_assign_channel(ttname) struct dsc$descriptor_s *ttname; {
    unsigned short channel = 0;

#ifdef COMMENT
/* what's all this then ... */
    struct dsc$descriptor_s d = {0,DSC$K_DTYPE_T,DSC$K_CLASS_S,0};
    d.dsc$w_length  = strlen(ttname);
    d.dsc$a_pointer = ttname;
#endif /* COMMENT */

    debug(F110,"vms_assign_channel device",ttname,0);

    if (!CHECK_ERR("vms_assign_channel: sys$assign",
        sys$assign(ttname, &channel, 0, 0))) return(0);

#ifdef NEW_STUFF_FROM_BRUCE_DAY
/*
  Which is not presently defined...
  This is supposed to allow C-Kermit to go into protocol mode on an LTA
  device without a prior CONNECT command.
*/
#ifdef IO$M_LT_CONNECT
    vms_status = sys$qiow(QIOW_EFN, channel, IO$_TTY_PORT|IO$M_LT_CONNECT,
                          &wrk_iosb, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!(vms_status & 1)) vms_lasterr = vms_status;
    debug(F101, "vms_assign_channel LAT connect status","",vms_status);
    debug(F101, "vms_assign_channel LAT connect iosb","",wrk_iosb.status);
#endif /* IO$M_LT_CONNECT */
#endif /* NEW_STUFF_FROM_BRUCE_DAY */

    return(channel);
}
#endif /* COMMENT */

/*  T T C L O S  --  Close the communication device.  */

int
ttclos(dummy) int dummy; {
    int channel;
#ifdef DEBUG
    if (deblog) {
        debug(F101,"ttclos 1: ttyfd","",ttyfd);
        debug(F101,"ttclos 1: ttychn","",ttychn);
        debug(F101,"ttclos 1: network","",network);
        debug(F101,"ttclos 1: overruns","",overruns);
    }
#endif /* DEBUG */

#ifdef NETCONN
    if (network) {                      /* Network connection. */
        netclos();                      /* Close it. */
        debug(F101,"ttclos 2: network","",network);
        debug(F101,"ttclos 2: ttychn","",ttychn);
        debug(F101,"ttclos 2: ttyfd","",ttyfd);
        /* network = 0; */
        return(0);
    }
#endif /* NETCONN */

/* The rest is for serial connections, in which ttychn is nonzero. */

    if (!ttychn)
      return(0);
/*
  Observations indicate that it can take 20-30 seconds for DTR to drop
  after closing the device.  Perhaps a call to tthang() should go here.
*/
    debug(F100,"ttclos calling ck_cancio","",0);
    ck_cancio();                        /* Cancel any pending i/o. */
    debug(F100,"ttclos calling ttres","",0);
    ttres();                            /* Reset modes. */
/*
  Assume it's a LAT device and try to do a LAT disconnect on it.
  If it fails, then it's not a LAT device and no harm is done.
*/
#ifdef IO$M_LT_DISCON
    debug(F100,"ttclos calling sys$qiow for LAT disconnect","",0);
    vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_TTY_PORT|IO$M_LT_DISCON,
                          &wrk_iosb, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!(vms_status & 1)) vms_lasterr = vms_status;
    debug(F101, "ttclos LAT disconnect, status", "", vms_status);
    debug(F101, "ttclos LAT disconnect, iosb", "", wrk_iosb.status);
#else
    debug(F100, "ttclos LAT disconnect not supported", "", 0);
#endif /* IO$M_LT_DISCON */
    debug(F100,"ttclos calling sys$dassgn","",0);
    channel = ttychn;
    ttychn = 0;                         /* Mark it as closed */
    wasclosed = 1;
    /* xhangup = 0; */
    if (!CHECK_ERR("ttclos: sys$dassgn", sys$dassgn(channel)))
      return(-1);
    debug(F101,"ttclos done - ttychn","",ttychn);
    return(0);
}

/*  T T R E S  --  Restore terminal to its original modes.  */

int
ttres() {                               /* Restore the tty to normal. */
    ttpmsk = 0xff;
#ifdef NETCONN
    if (network) return (0);            /* Network connection, do nothing */
#endif /* NETCONN */

    if (!ttychn) return(-1);            /* Not open. */

#ifdef COMMENT
/* ck_cancio() does exactly the same thing */
    tt_cancel();                        /* Cancel outstanding I/O */
#endif /* COMMENT */
    msleep(250);                        /* Wait for pending i/o to finish. */
    debug(F101,"ttres, ttychn","",ttychn);
    if (!CHECK_ERR("ttres: sys$qiow",
        sys$qiow(QIOW_EFN, ttychn, IO$_SETMODE, &wrk_iosb, 0, 0,
                 &ttold, sizeof(ttold), 0, 0, 0, 0))) return(-1);
    debug(F100,"ttres still going after sys$qiow","",0);
    return(0);
}

/*  T T B I N  --  Code shared by ttpkt() and ttvt()  */
/*
  Puts communication device in "binary" mode.  In VMS there's no distinction
  between device modes for terminal connection, packet operation, and dialing.
  BUT THERE SHOULD BE!  We need to get SET CARRIER-WATCH working...
*/
static int
ttbin(speed, xflow, xparity) int speed, xflow, xparity; {
    int s;
    extern int flow;                    /* Global flow control variable */

    debug(F101,"ttbin entry xparity","",xparity);
    debug(F101,"ttbin entry xflow","",xflow);
    debug(F101,"ttbin entry flow","",flow);

    if (xparity > -1) {
        ttprty  = xparity;
        ttpflg  = 0;                    /* Parity not sensed yet */
#ifdef COMMENT
/*
  No.  We want the mask applied for file transfer, but not for CONNECT or
  scripts, because then we lose Telnet negotiations.
*/
        ttpmsk  = ttprty ? 0177 : 0377; /* Parity stripping mask */
#endif /* COMMENT */
        debug(F101,"ttbin ttprty","",ttprty);
    }

#ifdef NETCONN
    if (network) return(0);             /* Nothing to do on net connections */
#endif /* NETCONN */

    if (!ttychn) return(-1);            /* Not open. */

/* The following applies only to serial ports - causes errors on network */

    ttdialing = 0;                      /* Not dialing */
    if (xflow != FLO_DIAL && ttflow != FLO_DIAX) {
        ttflow = xflow;                 /* Save for other CKVTIO routines. */
    } else if (xflow == FLO_DIAL) {     /* We're dialing */
        ttdialing = 1;                  /* Remember */
    }
    debug(F101,"ttbin ttdialing","",ttdialing);
    ttspeed = speed;                    /* Keep local copies of arguments */
    if ((s = ttsspd(speed/10)) < 0)     /* Get internal speed code */
      s = 0;

/* Log original terminal settings for debugging purposes */

    ttraw = ttold;                      /* Get a fresh copy of this */
    debug(F101, "ttbin ttraw.basic 1", "", ttraw.basic);
    debug(F101, "ttbin ttraw.extended 1", "", ttraw.extended);
/*
  Settings based on call parameters flow-control and parity...
  NOTE: we are using the GLOBAL copy of flow, not our parameter here.
  This is because the parameter might be FLO_DIAL or FLO_DIALX, which
  is not flow control at all.
*/
    if (flow == FLO_XONX) {                             /* FLOW = XON/XOFF */
        debug(F100,"ttbin FLO_XONX","",0);
        ttraw.basic |=  (TT$M_HOSTSYNC|TT$M_TTSYNC);
    } else if (flow == FLO_NONE) {                      /* FLOW = NONE */
        debug(F100,"ttbin FLO_NONE","",0);
        ttraw.basic &= ~(TT$M_HOSTSYNC|TT$M_TTSYNC);
    } else if (flow == FLO_KEEP) {                      /* FLOW = KEEP */
/*
 * Put flow-control parameters back the way we found them when
 * the device was first opened.
 */
        if (ttold.basic & TT$M_HOSTSYNC) {
            ttraw.basic |= TT$M_HOSTSYNC;
            debug(F100,"ttbin FLO_KEEP HOSTSYNC","",0);
        } else {
            ttraw.basic &= ~TT$M_HOSTSYNC;
            debug(F100,"ttbin FLO_KEEP no HOSTSYNC","",0);
        }
        if (ttold.basic & TT$M_TTSYNC) {
            ttraw.basic |= TT$M_TTSYNC;
            debug(F100,"ttbin FLO_KEEP TTSYNC","",0);
        } else {
            ttraw.basic &= ~TT$M_TTSYNC;
            debug(F100,"ttbin FLO_KEEP no TTSYNC","",0);
        }
/*
  NOTE: VMS 7.0 supports RTS/CTS.  But how can we include that here
  portably -- distinction between compile time & run time, etc...
*/
    }
    if (parity == 0)
      ttraw.basic  |= TT$M_EIGHTBIT;    /* Allow 8-bit data if no parity */
    else                                /* Otherwise */
      ttraw.basic  &= ~TT$M_EIGHTBIT;   /* 7-bit data. */
    ttraw.basic |= TT$M_NOBRDCST;       /* Turn on no-broadcasts */
    ttraw.basic |= TT$M_NOECHO;         /* Turn on no-echo */
    ttraw.basic &= ~TT$M_NOTYPEAHD;     /* Turn off no-type-ahead */
    ttraw.basic &= ~TT$M_ESCAPE;        /* Turn off escape-seq processing */
    ttraw.extended &= ~TT2$M_LOCALECHO; /* Turn off local-echo */
    ttraw.extended |= TT2$M_PASTHRU;    /* Turn on pass-through mode */
    ttraw.extended |= TT2$M_ALTYPEAHD;  /* Turn on big type-ahead buffers */

/* Report what we did so we can check for problems */

    debug(F101, "ttbin ttraw.basic 2", "", ttraw.basic);
    debug(F101, "ttbin ttraw.extended 2", "", ttraw.extended);

    vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_SETMODE, &wrk_iosb, 0, 0,
                         &ttraw, sizeof(ttraw), s, 0, 0, 0);
    if (!(vms_status & 1))
      vms_lasterr = vms_status;
/*
  On some VMS systems, during remote-mode file transfer, C-Kermit still seems
  sensitive to Ctrl-C, Ctrl-X, Ctrl-Y, Ctrl-N, and Ctrl-O.  It has been
  suggested that maybe the following will get rid of at least the ^C and ^Y
  sensitivity (although it is not clear why the code above did not already
  do this):

    vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_SETMODE+IO$M_CTRLCAST,
                          0, 0, 0, <handler:what-goes-here?>, 0, 0, 0, 0, 0);
    vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_SETMODE+IO$M_CTRLYAST,
                          0, 0, 0, <handler:what-goes-here?>, 0, 0, 0, 0, 0);

  Actually, no -- this just sets up Ctrl-C and Ctrl-Y handlers; it doesn't
  prevent VMS from trapping them...  Anyway, it's not a problem most places;
  observed locally only when coming into VMS thru the UCX 2.0 Telnet server.
*/
    debug(F101,"ttbin sy$qiow","",vms_status);
    if (vms_status != SS$_NORMAL) {     /* Error queuing request */
        print_msg("ttbin: sys$qiow");
        return(-1);
    }
    debug(F101,"ttbin iosb status","",wrk_iosb.status);
    if (wrk_iosb.status != SS$_NORMAL) { /* Error executing request */
        vms_status = wrk_iosb.status;
        print_msg("ttbin: sys$qiow(iosb)");
        return(-1);
    }
    debug(F100,"ttbin ok","",0);
    return(0);                          /* All OK */
}

/*  T T P K T  --  Condition the communication device for packets. */

/*  Returns 0 on success, -1 on failure.  */

int
ttpkt(speed,flow,parity) long speed; int flow, parity; {
    int x;
    debug(F101,"ttpkt flow","",flow);
    x = ttbin(speed,flow,parity);       /* Put device in binary mode */
    debug(F101,"ttpkt ttbin","",x);
    ttpmsk = (ttprty) ? 0x7f : 0xff;    /* Parity stripping mask. */
    return(x);
}

/*  T T V T  --  Condition communication device terminal connection. */

int
ttvt(speed,flow) long speed; int flow; {
    int x;
    debug(F101,"ttvt flow","",flow);
    if ((x = ttbin(speed,flow,-1)) > -1) /* Put device in binary mode */
      tvtflg = 1;
    debug(F101,"ttvt ttbin","",x);
    ttpmsk = 0xff;                      /* Let CONNECT module handle it */
    return(x);
}

/* T T I S P D  -- Return binary baud rate for internal coded speed */

int
ttispd(ispeed) unsigned char ispeed; {
    int s;
    long x;

#ifdef NETCONN
    if (network) return(-1);
#endif /* NETCONN */

/* When the line is set, grab the line speed  and save it */

    for (s = 0;  ttspeeds[s].dec &&
        (ttspeeds[s].dec != ispeed);  s++)
                ;

/* If speed is zero, then no match.  Set speed to -1 so it is undefined */

    x = ttspeeds[s].line ? (int) ttspeeds[s].line * 10 : -1;
    debug(F101,"ttispd","",x);
    return(x);
}

#define NSPDLIST 64
static long spdlist[NSPDLIST];

long *
ttspdlist() {
    int i; long n;

    for (i = 0; ttspeeds[i].dec; i++) {
        n  = ttspeeds[i].line * 10;
        if (n == 70L)
          n = 75L;
        else if (n == 130L)
          n = 134L;
        spdlist[i+1] = n;
    }
    spdlist[0] = i;
    return((long *)spdlist);
}

/*  T T S S P D  --  Set speed for serial device */
/*
  In VMS this routine doesn't actually change the speed; it just checks its
  argument and then if OK sets a global variable that is used by ttgspd() to
  say what the current speed is.  The speed is actually changed only by
  ttbin(), which has its own speed argument.  Note that in VMS there is no
  way to read the device's current speed (see ttgspd()).
*/
int
ttsspd(cps) int cps; {
    int s;
    char msg[50];

#ifdef  NETCONN
    if (network) {
#ifdef TN_COMPORT
        if (istncomport())
          return(tnc_set_baud(cps * 10));
        else
#endif /* TN_COMPORT */
          return(0);
    }
#endif  /* NETCONN */

    if (cps <= 0)                       /* 026 Unknown cps fails */
      return (-1);
    for (s = 0;  ttspeeds[s].line && (ttspeeds[s].line != cps);  s++) ;
    if (ttspeeds[s].line) {
        ttspeed = cps * 10L;            /* Make a copy global to this module */
        if (ttspeed == 70L)             /* ... in bits per second, not cps, */
          ttspeed = 75L;                /* because ttgspd() uses it! */
        debug(F101,"ttsspd","",ttspeed);
        return(ttspeeds[s].dec);
    } else {
        sprintf(msg,"Unsupported line speed - %d\n",cps*10);
        debug(F101,"ttsspd fails","",cps*10);
        ermsg(msg);
        ermsg("Current speed not changed\n");
        return(-1);
    }
}

/* Interrupt Functions */

/*  C O N I N T  --  Console Interrupt setter  */

static sig_t cctrap;

VOID
#ifdef CK_ANSIC
conint(SIGTYP (*f)(int), SIGTYP (*s)(int))
#else
conint(f,s) int (*f)(int), (*s)(int);
#endif /* CK_ANSIC */
/* conint */ {                          /* Set an interrupt trap. */
    /* debug(F101,"conint batch","",batch); */

    cctrap = f;                         /* Make a global copy of handler. */
    if (!itsatty)                       /* Ignore signals in batch */
      return;                           /* or if COMMAND INTERRUPT OFF. */
    signal(SIGINT,f);                   /* Not batch - function to trap to. */
    conif = 1;                          /* Flag console interrupts on. */
}

/*  C O N N O I  --  Reset console terminal interrupts */

VOID
connoi() {                              /* Console-no-interrupts */
    signal(SIGINT,SIG_IGN);
    conif = 0;
}

/*  T T O L  --  Write string s, length n, to communication device.  */

#ifndef IO$M_BREAKTHRU
#define IO$M_BREAKTHRU  0x0200
#endif /* IO$M_BREAKTHRU */

#ifndef SS$_EXQUOTA
#define SS$_EXQUOTA 28
#endif /* SS$_EXQUOTA */

int
ttol(s,n) int n; CHAR *s; {
    int
      remaining,                        /* Amount left to write */
      size;                             /* How much to write this time */
    static int max = 0;                 /* Chunk size for writing */

#ifdef NETCONN
    debug(F101,"ttol network","",network);
    if (network)                        /* If SET HOST connection, */
        return(nettol(s,n));            /* call network package. */
#endif /* NETCONN */

/* It's not a SET HOST connection. */

    debug(F101,"ttol ttychn","",ttychn);
    if (!ttychn) return(-1);            /* Not open. */
    debug(F101,"ttol length","",n);
    debug(F101,"ttol max","",max);

/* Have we already calculated a chunk size? */

    if (max == 0) {                     /* No, try to send whole packet. */
        vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_WRITEVBLK|IO$M_BREAKTHRU,
                              &wrk_iosb, 0, 0, s, n, 0, 0, 0, 0);
        if (!(vms_status & 1)) vms_lasterr = vms_status;
        if (vms_status == SS$_NORMAL) {
            debug(F101,"ttol 1 ok","",n);
            return(n);
        }
#ifdef DEBUG
        if (deblog) {                   /* Failed. */
            debug(F101,"ttol 1 error, vms_status","",vms_status);
            debug(F101,"ttol 1 iosb size","",wrk_iosb.size);
            debug(F101,"ttol 1 iosb status","",wrk_iosb.status);
        }
#endif /* DEBUG */
        if (vms_status != SS$_EXQUOTA)  /* "Quota exceeded"? */
          return(-3);                   /* No, something else, give up. */
/*
  Here we should find out what MAXBUF is (not to mention BYTLM, BIOLM, and
  friends), and chop up the packet into pieces accordingly.  But reportedly
  this information, and how to use it (percent overhead, etc), is highly
  VMS-version-dependent.  So instead we just try different numbers.  Our first
  attempt keeps dividing it in half until it works, down to about 70.
*/
        do {
            max = (max == 0) ? n / 2 : max / 2;
            debug(F101,"ttol 2 max","",max);
            vms_status = sys$qiow(QIOW_EFN, ttychn,
                                  IO$_WRITEVBLK|IO$M_BREAKTHRU,
                                  &wrk_iosb, 0, 0, s, max, 0, 0, 0, 0);
            if (!(vms_status & 1)) vms_lasterr = vms_status;
            debug(F101,"ttol 2 vms_status","",vms_status);
            if (vms_status == SS$_NORMAL)
              break;
        } while (max > 70);             /* 70 is the minimum. */

        if (vms_status != SS$_NORMAL)   /* Loop exhausted, fail. */
          return(-3);

    } else {                            /* We already calculated max. */

        size = (max > n) ? n : max;     /* Write 1st chunk, but not too much */
        vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_WRITEVBLK|IO$M_BREAKTHRU,
                              &wrk_iosb, 0, 0, s, size, 0, 0, 0, 0);
        if (!(vms_status & 1)) vms_lasterr = vms_status;
        debug(F101,"ttol 3 vms_status","",vms_status);
        if (vms_status != SS$_NORMAL)
          return(-3);
        if (size == n) {                /* (Not strictly necessary) */
            debug(F101,"ttol 3 done","",n);
            return(n);
        }
    }
/*
  We have written the first chunk successfully, now write the remaining
  max-sized chunks, plus the (usually) less-than-max-sized last chunk.
*/
    remaining = n;
    while (1) {
        s += max;                       /* Where to start */
        remaining -= max;               /* How much left to write */
        size = (remaining < max) ?      /* How much to write this time */
          remaining : max;
        if (size < 1)                   /* Done? */
          break;

        debug(F101,"ttol 4 size","",size);
        vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_WRITEVBLK|IO$M_BREAKTHRU,
                              &wrk_iosb, 0, 0, s, size, 0, 0, 0, 0);
        if (!(vms_status & 1)) vms_lasterr = vms_status;
        debug(F101,"ttol 4 vms_status","",vms_status);
        if (vms_status != SS$_NORMAL)
          return(-3);
    }
    debug(F101,"ttol 5 done","",n);
    return(n);
}

/*  T T O C  --  Output a character to the communication line  */

int
#ifdef CK_ANSIC
ttoc(char c)
#else
ttoc(c) char c;
#endif  /* CK_ANSIC */
/* ttoc */ {
#ifdef NETCONN
    if (network) {
        return(nettoc(c));
    } else {
#endif /* NETCONN */
        debug(F101,"ttoc char","",c);
        if (!ttychn) {
            debug(F100,"ttoc ttychn not open","",0);
            return(-1);                 /* Not open. */
        }
        if (CHECK_ERR("ttoc: sys$qiow",
                      sys$qiow(QIOW_EFN, ttychn, IO$_WRITEVBLK|IO$M_BREAKTHRU,
                               &wrk_iosb, 0, 0, &c, 1, 0, 0, 0, 0)))
          return(0);
#ifdef NETCONN
    }
#endif /* NETCONN */
    return(-1);
}

/*  T T _ C A N C E L  --  Cancel i/o on tty channel if not complete  */

VOID
tt_cancel() {
    int mask;
#ifdef NETCONN
    if (network) return;
#endif /* NETCONN */
    CHECK_ERR("tt_cancel: sys$cancel",sys$cancel(ttychn));
    tt_queued = 0;
}

/*  C O N _ C A N C E L  --  Cancel i/o on console channel if not complete  */

VOID
con_cancel() {
    int mask;

    CHECK_ERR("con_cancel: sys$cancel",sys$cancel(conchn));
    con_queued = 0;
}

/*  S N D B R K  --  Send a BREAK signal of the given length.  */

int
sndbrk(msec) int msec; {
    int long x = 0;
    int brklen;
    struct iosb_struct  tmp_ttiosb;
    struct tt_mode ttchr;
#ifndef TT$M_BREAK                      /* For old VMS with no BREAK... */
/*
  Note: 110 is used instead of 50, because 50 is not supported by all
  VAX serial port controllers.
*/
#define BRKSPD = 110                    /* Speed for simulating BREAK */
#define BRKSYM = TT$C_BAUD_110;         /* VMS symbol for this speed */
#endif /* TT$M_BREAK */

#ifdef NETCONN
    if (network) {                      /* Send network BREAK */
#ifdef TN_COMPORT
        if (istncomport())
          return((tnsndb((long)msec) >= 0) ? 0 : -1);
        else
#endif /* TN_COMPORT */
        return(netbreak());             /* Length doesn't matter */
    }
#endif /* NETCONN */

    if (!ttychn) return(-1);            /* SET LINE not done. */
    debug(F101,"sndbrk msec","",msec);

    tt_cancel();                        /* Cancel I/O */

#ifndef TT$M_BREAK                      /* VMS doesn't have BREAK function */

/* Send the right number of NULs at BREAK-simulation speed... */

    brklen = ( BRKSPD * 1000 ) / ( msec * 10 ); /* Calculate number of chars */
    if (brklen > sizeof(brkarray)) brklen = sizeof(brkarray);
    debug(F101,"sndbrk speed","",BRKSPD);
    debug(F101,"sndbrk brklen","",brklen);
    /* Get current modes */
    if (!CHECK_ERR("ttsndb: SENSEMODE",
        sys$qiow(QIOW_EFN, ttychn, IO$_SENSEMODE, &wrk_iosb, 0, 0,
                &ttchr, sizeof(ttchr), 0, 0, 0, 0))) return(-1);
    /* Set speed */
    if (!CHECK_ERR("ttsndb: SETMODE(1)",
        sys$qiow(QIOW_EFN, ttychn, IO$_SETMODE, &tmp_ttiosb, 0, 0,
                &ttchr, sizeof(ttchr), BRKSYM, 0, 0, 0))) return(-1);
    /* Send NULs */
    if (!CHECK_ERR("ttsndb: writing nulls",
        sys$qiow(QIOW_EFN, ttychn, IO$_WRITEVBLK|IO$M_BREAKTHRU, &tmp_ttiosb,
                 0, 0, (char *) brkarray, brklen, 0, 0, 0, 0))) return(-1);
    /* Restore modes */
    if (!CHECK_ERR("ttsndb: SETMODE(2)",
        sys$qiow(QIOW_EFN, ttychn, IO$_SETMODE, &tmp_ttiosb, 0, 0,
                &ttchr, sizeof(ttchr), wrk_iosb.size, 0, 0, 0))) return(-1);
#else
    if (!CHECK_ERR("ttsndb: SENSEMODE",
        sys$qiow(QIOW_EFN, ttychn, IO$_SENSEMODE, &wrk_iosb, 0, 0,
                &ttchr, sizeof(ttchr), 0, 0, 0, 0))) return(-1);
    x = TT$M_BREAK;                     /* Break signal on */
    if (!CHECK_ERR("ttsndb: SETMODE(1)",
        sys$qiow(QIOW_EFN, ttychn, IO$_SETMODE, &wrk_iosb, 0, 0,
                &ttchr, sizeof(ttchr), 0, 0, x, 0))) return(-1);
    msleep(msec);                       /* Sleep requested amount of time */
    x = 0;                              /* Break signal off */
    if (!CHECK_ERR("ttsndb: SETMODE(2)",
        sys$qiow(QIOW_EFN, ttychn, IO$_SETMODE, &wrk_iosb, 0, 0,
                &ttchr, sizeof(ttchr), 0, 0, x, 0))) return(-1);
#endif /* TT$M_BREAK */
    return(0);
}

/*  T T S N D B  --  Send a BREAK signal  */

int
ttsndb() {
    return(sndbrk(275));
}

/*  T T S N D L B  --  Send a Long BREAK signal  */

int
ttsndlb() {
    return(sndbrk(1500));
}

/*  T T H A N G  --  Hang up the communications line  */
/*
  Warning: As written, this function DOES NOT WORK on terminal server
  ports.  This is a shortcoming of VMS, confirmed by the Digital Diagnostic
  Center (or whatever DDC stands for).  Someone should add code here to test
  if the ttychn device is not a real terminal, and if so to handle it some
  other way, like set the speed to zero for a sec, or close and reopen the
  device.
*/
int
tthang() {
    if (!xlocal) return(0);             /* Only on local connections. */

    debug(F101,"tthang network","",network);
#ifdef NETCONN
    if (network) {                      /* Network connection. */
        int x;
        debug(F101,"tthang istncomport","",istncomport());
#ifdef TN_COMPORT
        if (istncomport()) {
            int rc = tnc_set_dtr_state(0);
            if (rc >= 0) {
                msleep(500);
                rc = tnc_set_dtr_state(1);
            }
            return(rc >= 0 ? 0 : -1);
        } else {
#endif /* TN_COMPORT */
            x = netclos();
            debug(F101,"tthang netclos","",x);
            return((x < 0) ? -1 : 1); /* Just close it. */
#ifdef TN_COMPORT
        }
#endif /* TN_COMPORT */
    }
#endif /* NETCONN */

    if (!ttychn) return(0);             /* Not open. */

    tt_cancel();                        /* Cancel pending i/o. */
/*
  This is NOT listed in the VMS Terminal Driver as one of the functions
  that does NOT work with LAT devices.
*/
    debug(F101,"tthang 1","",gtimer());
    if (!CHECK_ERR("tthang: sys$qiow",
        sys$qiow(QIOW_EFN, ttychn, IO$_SETMODE|IO$M_HANGUP, &wrk_iosb, 0, 0,
                0, 0, 0, 0, 0, 0))) return(-1);
/*
  The following 3-second sleep is required because the sys$qiow() returns
  immediately, about 2.5 seconds before VMS brings DTR back up.  Without this
  sleep(), DIAL does not work at all if DIAL HANGUP is ON, and, worse,
  subsequent operations on the device can hang the Kermit process
  uninterruptibly.
*/
    sleep(3);
    debug(F101,"tthang 2","",gtimer());
    return(1);
}

/*  M S L E E P  --  Millisecond version of sleep().  */

/*
 Handles intervals up to about 7 minutes (2**32 / 10**7 seconds)
*/
int
msleep(m) int m; {

    struct time_struct {
        long int hi, lo;
    } t;

    if (m <= 0) return(0);
    t.hi = -10000 * m;  /*  Time in 100-nanosecond units  */
    t.lo = -1;
    if (!CHECK_ERR("msleep: sys$schdwk",
        sys$schdwk(0, 0, &t, 0))) return(-1);
    sys$hiber();
    debug(F101,"msleep ok","",m);
    return(0);
}

/*  R T I M E R --  Reset elapsed time counter  */

VOID
rtimer() {
    tcount = time( (TIME_T *) 0);
}

/*  G T I M E R --  Get current value of elapsed time counter in seconds  */

int
gtimer() {
    int x;
    x = (int) (time( (TIME_T *) 0 ) - tcount);
    return( (x < 0) ? 0 : x );
}

#ifdef GFTIMER
#ifdef VMSI64
/* The native VMS code is broken in VMS/IA64 8.1. */
/* Luckily, in VMS 8.1 we can use the Unix code. */

static struct timeval tzero;

VOID
rftimer() {
    (VOID) gettimeofday(&tzero, (struct timezone *)0);
}

CKFLOAT
gftimer() {
    struct timeval tnow, tdelta;
    CKFLOAT s;
#ifdef DEBUG
    char fpbuf[64];
#endif /* DEBUG */
    (VOID) gettimeofday(&tnow, (struct timezone *)0);

    tdelta.tv_sec = tnow.tv_sec - tzero.tv_sec;
    tdelta.tv_usec = tnow.tv_usec - tzero.tv_usec;

    if (tdelta.tv_usec < 0) {
	tdelta.tv_sec--;
	tdelta.tv_usec += 1000000;
    }
    s = (CKFLOAT) tdelta.tv_sec + ((CKFLOAT) tdelta.tv_usec / 1000000.0);
    if (s < GFMINTIME)
      s = GFMINTIME;
#ifdef DEBUG
    if (deblog) {
	sprintf(fpbuf,"%f",s);
	debug(F110,"gftimer",fpbuf,0);
    }
#endif /* DEBUG */
    return(s);
}


#else

static QUAD tzero;

VOID
rftimer() {
    int status;
    status = sys$gettim(&tzero);
    if (!(vms_status & 1)) vms_lasterr = vms_status;
    debug(F101,"rftimer status","",status);
}

/* Floating-point (fractional) timer -- granularity in VMS is ~0.00098 sec. */

CKFLOAT
gftimer() {
    float s;                            /* gcc gawks at CKFLOAT */
    QUAD tnow, diff;                    /* 64-bit times */
    int status;
    unsigned long lkd = LIB$K_DELTA_SECONDS_F;
#ifdef DEBUG
    char fpbuf[64];
#endif /* DEBUG */
    status = sys$gettim(&tnow);
    if (!(status & 1)) vms_lasterr = status;
    debug(F101,"gftimer status 1","",status);
    status = lib$sub_times(&tnow, &tzero, &diff );
    if (!(status & 1)) vms_lasterr = status;
    debug(F101,"gftimer status 2","",status);
    status = lib$cvtf_from_internal_time(&lkd,&s,&diff);
    if (!(status & 1)) vms_lasterr = status;
    debug(F101,"gftimer status 3","",status);
#ifdef DEBUG
    if (deblog) {
        sprintf(fpbuf,"%f",s);
        debug(F110,"gftimer s",fpbuf,0);
    }
#endif /* DEBUG */
    return(s > 0.0 ? (CKFLOAT) s : (CKFLOAT) 0.000001);
}
#endif	/* VMSI64 */
#endif /* GFTIMER */

/*  Z T I M E  --  Return date/time string  */

VOID
ztime(s) char **s; {
    static TIME_T clock;
#ifdef COMMENT
#ifdef bogus
    static char time_string[24];
    struct dsc$descriptor_s t =
        {sizeof(time_string)-1,DSC$K_DTYPE_T,DSC$K_CLASS_S,&time_string};

    if (!CHECK_ERR("ztime: sys$asctim",
        sys$asctim(0, &t, 0, 0))) return(-1);
    time_string[t.dsc$w_length] = '\0';
    if (*s)
      *s = &time_string;
#else
    char *asctime();
    struct tm *tp;

    time(&clock);
    tp = localtime(&clock);
    if (*s)
      *s = asctime(tp);
#endif /* bogus */
#else /* not COMMENT */
/*
 Apparently ctime() is available in old C libraries, even though asctime()
 is not.  Let's use the same method for all versions.
*/
    time(&clock);
    if (s)
      *s = ctime(&clock);
#endif /* COMMENT */
}

/*  C O N G M  --  Get console terminal modes.  */

/*
 Saves current console mode, and establishes variables for switching between
 current (presumably normal) mode and other modes.
*/
int
congm() {
    char s[] = CONDEV_COLON;
    extern int bgset;
    struct itmlst dviitm[] = { {4,DVI$_DEVCLASS,NULL,0}, {0,0,0,0}};
#ifdef COMMENT /* old */
    struct dsc$descriptor_s
        r = {sizeof(s),DSC$K_DTYPE_T,DSC$K_CLASS_S,(char *)&s};
#else /* from ttj */
    struct dsc$descriptor_s
        devnam = {sizeof(s)-1,DSC$K_DTYPE_T,DSC$K_CLASS_S,NULL};
    devnam.dsc$a_pointer = s;
#endif /* COMMENT */

    debug(F101,"congm bgset","",bgset); /* Background forced */
    if (bgset > 0)
      {
      batch = 1;
      backgrd = 1;
      itsatty = 0;
      return(-1);               /* e.g. by -B on command line */
      }

    debug(F101,"congm cgmf","",cgmf);
    if (cgmf) return(-1);               /* If called already, then nop */

    debug(F101,"congm batch","",batch);

    dviitm[0].adr = (char *)&dviitm[0].adr;

#ifndef CK_USEGETJPI
#ifdef JPI$_MODE                        /* I don't know how far back */
#ifdef JPI$K_BATCH                      /* these go... */
/* #define CK_USEGETJPI */
#endif /* JPI$K_BATCH */
#endif /* JPI$_MODE */
#endif /* CK_USEGETJPI */

#ifdef CK_USEGETJPI                     /* New way works better I think */
    {                                   /* (fdc, Jan 98) */
        struct itmlstdef {              /* Update by Hunter Goatley Jul 2001 */
            short int buflen;
            short int itmcod;
            char *bufaddr;
            long int *retlen;
        };
        short int terminal_name_size;   /* lh 31 Dec 2001 */
        char *terminal_name[32];
        struct itmlstdef gjiitm[] = { 32, JPI$_TERMINAL, NULL, 0, 0, 0, 0, 0 };

        /* Get the terminal name for the process */

        gjiitm[0].bufaddr = (char *)&terminal_name;
        gjiitm[0].retlen = (long *)&terminal_name_size;
        vms_status = sys$getjpiw(0, 0, 0, &gjiitm, 0, 0, 0);
        if (!(vms_status & 1)) vms_lasterr = vms_status;
        if (vms_status != SS$_NORMAL) {
            debug(F101,"congm sys$getjpiw error","",vms_status);
        } else {
            debug(F101, "congm sys$getdviw: terminal_name_size", "",
                  (unsigned long int) terminal_name_size);

            /* If the process has a terminal, make sure the device really */
            /* is a terminal class device. */

            if (terminal_name_size != 0) {
                devnam.dsc$a_pointer = (char *)&terminal_name;
                devnam.dsc$w_length = terminal_name_size;
                if (!CHECK_ERR("congm: sys$getdviw",
                             sys$getdviw(0,0,&devnam,&dviitm,&wrk_iosb,0,0,0))
                    )
                  return(-1);
                debug(F101, "congm sys$getdviw: devclass", "",
                      (unsigned long int) dviitm[0].adr);
                if ((unsigned long int) dviitm[0].adr != DC$_TERM) {
                    if (bgset < 0) {    /* No "-z" on command line */
                        batch = 1;
                        backgrd = 1;
                        itsatty = 0;
                        debug(F111,"congm batch","devclass",batch);
                    }
                }
            } else {
                if (bgset < 0) {        /* Only if "-z" not given. */
                    batch = 1;
                    backgrd = 1;
                    itsatty = 0;
                    debug(F111,"congm batch","terminal_name_size",batch);
                }

            }
            debug(F101,"congm sys$getjpiw batch","",batch);
        }
    }
#else /* Old way */
    if (!CHECK_ERR("congm: sys$getdviw",
        sys$getdviw(0, 0, &devnam, &dviitm, &wrk_iosb, 0, 0, 0))) return(-1);
    debug(F101, "congm sys$getdviw: devclass", "",
          (unsigned long int) dviitm[0].adr);
    if ((unsigned long int) dviitm[0].adr != DC$_TERM)
      {
      batch = 1;
      backgrd = 1;
      itsatty = 0;
      }
    else
      {
      batch = 0;
      backgrd = 0;
      itsatty = 1;
      }
    debug(F101,"congm sys$getdviw batch","",batch);
#endif /* COMMENT */

#ifdef COMMENT                          /* Let's try it anyway... */
      else {
#endif /* COMMENT */
        /*
           NOTE: Reportedly, when C-Kermit is run from a .COM file
           it complains "Sorry, terminal type not supported: vt300-80".
           Reportedly, the cure is to execute the following code always,
           not just when the if condition above is false; i.e. just get
           rid of {, } else {, and }.  But some of the following system
           calls look like they might be dangerous on non-terminals,
           so widespread testing would be needed.  Better safe than sorry.
        */
        debug(F101, "congm: conchn", "", conchn);
        if (!conchn) {                  /* Get console channel */
            $DESCRIPTOR(sys_input, CONDEV_COLON);
            conchn = vms_assign_channel(&devnam);
        }
        if (!conchn)
          return(-1);
        if (itsatty) {                  /* lh 31 Dec 2001 */
            if (!CHECK_ERR("congm: sys$qiow",
                           sys$qiow(QIOW_EFN, conchn, IO$_SENSEMODE, &wrk_iosb,
                                    0, 0, &ccold, sizeof(ccold), 0, 0, 0, 0)))
              return(-1);
        }
        ccraw = cccbrk = ccold;
#ifdef COMMENT
    }
#endif /* COMMENT */
    cgmf = 1;                           /* Flag that we got them. */
    return(0);
}

/*  C O N C B --  Put console in cbreak mode.  */

/*  Returns 0 if ok, -1 if not  */

int
#ifdef CK_ANSIC
concb(char esc)
#else
concb(esc) char esc;
#endif /* CK_ANSIC */
/* concb */ {
    int x;

    debug(F101,"concb batch","",batch);
    if (!itsatty) {                     /* If we're running in batch */
        extern int bgset;               /* but */
        if (bgset == 0)                 /* the user said SET BACKROUND OFF */
          ckxech = 1;                   /* allow echoing to batch log */
        debug(F101,"concb 1 ckxech","",ckxech);
        return(0);
    }
    if (!cgmf) congm();                 /* Get modes if necessary. */
    escchr = esc;                       /* Make this available to other fns */
    ckxech = 1;                         /* Program can echo characters */
    debug(F101,"concb 2 ckxech","",ckxech);
/*
  Note: PASTHRU / PASSALL is what is preventing the Ctrl-C trap in the
  main program from working.  This business can be removed without any effect
  at all on the command parser -- everything still works: completion, ?-help,
  editing, etc.  The only problem is that Ctrl-Y is not trapped, so the
  program dies and leaves the terminal in no-echo mode.
*/
    cccbrk.extended |= TT2$M_PASTHRU | TT2$M_ALTYPEAHD;
    if (parity)
      cccbrk.basic |= TT$M_NOECHO;
    else
      cccbrk.basic |= TT$M_NOECHO | TT$M_EIGHTBIT;
    cccbrk.basic &= ~TT$M_ESCAPE;       /* Disable escape-seq processing */
    cccbrk.extended &= ~TT2$M_LOCALECHO; /* and local echoing */
    if (!CHECK_ERR("concb: sys$qiow",
        sys$qiow(QIOW_EFN, conchn, IO$_SETMODE, &wrk_iosb, 0, 0,
                 &cccbrk, sizeof(cccbrk), 0, 0, 0, 0))) return(-1);
    debug(F100,"concb ok","",0);
    return(0);
}

/*  C O N B I N  --  Put console in binary mode  */

/*  Returns 0 if ok, -1 if not  */

int
#ifdef CK_ANSIC
conbin(char esc)
#else
conbin(esc) char esc;
#endif /* CK_ANSIC */
/* conbin */ {

    debug(F101,"conbin batch","",batch);
    if (!itsatty) return(0);
    if (!cgmf) congm();                 /* Get modes if necessary. */
    escchr = esc;                       /* Make this available to other fns */
    ckxech = 1;                         /* Program can echo characters */
    debug(F101,"conbin ckxech","",ckxech);
    ccraw.extended |= TT2$M_PASTHRU | TT2$M_ALTYPEAHD;
    ccraw.basic &= ~TT$M_ESCAPE;        /* Disable escape-seq processing */
    ccraw.extended &= ~TT2$M_LOCALECHO; /* and local echoing */
    ccraw.basic |= TT$M_NOBRDCST;       /* Turn on no-broadcasts */
    if (parity)
      ccraw.basic |= TT$M_NOECHO;
    else
      ccraw.basic |= TT$M_NOECHO | TT$M_EIGHTBIT;
#ifdef COMMENT
    ccraw.basic &= ~(TT$M_HOSTSYNC | TT$M_TTSYNC);
#endif /* COMMENT */
    if (!CHECK_ERR("conbin: sys$qiow",
        sys$qiow(QIOW_EFN, conchn, IO$_SETMODE, &wrk_iosb, 0, 0,
                 &ccraw, sizeof(ccraw), 0, 0, 0, 0))) return(-1);
    return(0);
}

/*  C O N R E S  --  Restore the console terminal  */

int
conres() {
    debug(F101,"conres cgmf","",cgmf);
    if (!cgmf) return(0);               /* Do nothing if modes unknown */
    if (!itsatty) return(0);

    msleep(250);
    ckxech = 0;                         /* System should echo chars */
    debug(F101,"conres ckxech","",ckxech);
    debug(F101,"conres calling sys$qiow","",0);
    vms_status = sys$qiow(QIOW_EFN, conchn, IO$_SETMODE, &wrk_iosb, 0, 0,
        &ccold, sizeof(ccold), 0, 0, 0, 0);
    if (!(vms_status & 1)) vms_lasterr = vms_status;
    debug(F101,"conres sys$qiow status","",vms_status);
    if (vms_status == SS$_NORMAL) {
        debug(F100,"conres ok","",0);
        return(0);
    } else if (vms_status == SS$_DEVOFFLINE) {
        debug(F100,"conres DEVOFFLINE","",0);
        exit(SS$_ABORT);
    } else
      return(-1);
}

/*  C O N R E S N E --  Restore the console terminal with No Echo */

int
conresne() {
    debug(F101,"conresne cgmf","",cgmf);
    if (!cgmf) return(0);               /* Don't do anything if modes unk */
    if (!itsatty) return(0);

    msleep(250);
    ckxech = 1;                         /* Program should echo chars */
    debug(F101,"conresne ckxech","",ckxech);
    cctmp = ccold;
    cctmp.basic |= TT$M_NOECHO;
    if (!CHECK_ERR("conres: sys$qiow",
        sys$qiow(QIOW_EFN, conchn, IO$_SETMODE, &wrk_iosb, 0, 0,
        &cctmp, sizeof(cctmp), 0, 0, 0, 0))) return(-1);
    debug(F100,"conresne ok","",0);
    return(0);
}

/*  C O N O C  --  Output a character to the console terminal  */

int
#ifdef CK_ANSIC
conoc(char c)
#else
conoc(c) char c;
#endif /* CK_ANSIC */
/* conoc */ {
    if (!itsatty || !initflg)
      putchar(c);
    else if (!CHECK_ERR("conoc: sys$qiow",
                sys$qiow(QIOW_EFN, conchn, IO$_WRITEVBLK|IO$M_BREAKTHRU,
                     &wrk_iosb, 0, 0, &c, 1, 0, 0, 0, 0)))
      return(-1);
    return(1);
}

/*  C O N X O  --  Write x characters to the console terminal  */

int
conxo(x,s) char *s; int x; {
    if (!itsatty || !initflg) fprintf(stdout, "%.*s", x, s);
    else if (!CHECK_ERR("conxo: sys$qiow",
        sys$qiow(QIOW_EFN, conchn, IO$_WRITEVBLK|IO$M_BREAKTHRU,
                 &wrk_iosb, 0, 0, s, x, 0, 0, 0, 0))) return(-1);
    return(0);
}

/*  C O N O L  --  Write a line to the console terminal  */

int
conol(s) char *s; {
    int len;

    if (!itsatty || !initflg)
      fputs(s, stdout);
    else {
        len = strlen(s);
        if (!CHECK_ERR("conol: sys$qiow",
            sys$qiow(QIOW_EFN, conchn, IO$_WRITEVBLK|IO$M_BREAKTHRU, &wrk_iosb,
                     0, 0, s, len, 0, 0, 0, 0))) return(-1);
    }
    return(1);
}

/*  C O N O L A  --  Write an array of lines to console, with CRLFs added */

int
conola(s) char *s[]; {
    int i;
    char t[100], *cp;

    for (i = 0; *s[i] ; i++) {
        ckstrncpy(t,s[i],100);
        for (cp = t + strlen(t); --cp >= t;) {
            if (*cp != '\n' && *cp != '\r') {
                cp++;
                *cp++ = '\r'; *cp++ = '\n'; *cp++ = '\0';
                break;
            }
        }
        if (conol(t) < 0) return(-1);
    }
    return(0);
}

/*  C O N O L L  --  Output a string followed by CRLF  */

int
conoll(s) char *s; {
    int x;
    x = conol(s);
    if (x > -1)
      x = conol("\r\n");
    return(x);
}

/*  C O N C H K  --  Check if characters available at console  */

int
conchk() {
    struct {
        unsigned short count;
        unsigned char first;
        unsigned char reserved1;
        long reserved2;
        } t;

    if (!itsatty || !initflg)
      return(0);
    return(CHECK_ERR("conchk: sys$qiow",
        sys$qiow(QIOW_EFN, conchn, IO$_SENSEMODE|IO$M_TYPEAHDCNT, &wrk_iosb,
                 0, 0, &t, sizeof(t), 0, 0, 0, 0)) ? t.count : 0);
}

/*  C O N I N C  --  Get a character from the console  */

int
coninc(timo) int timo; {                /* Timo > 0 = timeout in seconds. */
    int n = 0;
    unsigned char ch;
    int func, mask;

    debug(F101,"coninc timo","",timo);
    debug(F101,"coninc con_queued","",con_queued);

    if (!itsatty || !initflg)           /* (was "if (batch || !initflg)" */
      return(getchar());

    mask = 1 << CON_EFN;

    if (con_queued) {           /* If a console read was already posted... */
                                /* e.g. by contti() ... */
        if (timo > 0) {                     /* If a timeout was specified... */
            struct { int hi, lo; } qtime;   /* Set a timer... */
            qtime.hi = -10*1000*1000*timo;  /* in VMS "delta-time" notation. */
/*
  If the timo value is big enough to make the delta-time overflow an integer,
  substitute something useful.
*/
            if (qtime.hi > 0)           /* Did it go positive? */
              qtime.hi = -0x7fffffff;   /* Yes, so fudge it. */
            qtime.lo = -1;
            sys$setimr(TIM_EFN, &qtime, 0, 0, 0); /* Specify event flag. */
            mask |= TIM_EFN;            /* And add it to read mask. */
        }
        sys$wflor(CON_EFN, mask);       /* Wait for SETIMR to complete. */
        sys$readef(CON_EFN, &mask);     /* Read event flags. */
        if (mask & (1 << CON_EFN)) {    /* We got a console event? */
            ch = (unsigned char) conch; /* (see contti() about this...) */
            CHECK_ERR("coninc: coniosb.status", coniosb.status);
            con_queued = 0;
        } else {                        /* We didn't */
            ch = -1;                    /* So indicate that coninc() ... */
            vms_status = SS$_TIMEOUT;   /*  timed out. */
        }
    } else {                            /* Console read not already posted */
        func = IO$_READVBLK | IO$M_NOFILTR;
        if (timo > 0) func |= IO$M_TIMED;
        CHECK_ERR("coninc: sys$qiow",
          sys$qiow(QIOW_EFN, conchn, func, &wrk_iosb,0,0,&ch,1,timo,0,0,0));
    }
    if (vms_status & 1) {
        if (wrk_iosb.status == SS$_TIMEOUT)
          return(-1);
        else return((ch == '\r') ? '\n' : (unsigned)ch);
    } else return(-1);
}

/*  V M S _ G E T C H A R -- get a character from the console (no echo).
 *      Since we use raw reads, we must check for ctrl/c, ctrl/y and
 *      ctrl/z ourselves.  We probably should post a "mailbox" for
 *      ctrl/c and ctrl/y so the poor user can abort a runaway Kermit.
 *      Note: this routine intends for ctrl/z (eof) to be "permanent".
 *      Currently, no kermit routine calls "clearerror".  If this
 *      changes, the following code must be rewritten.
 */

int
vms_getchar() {
    register unsigned int ch;
    static int ateof = 0;

    if (ateof)
      return (EOF);
    ch = coninc(0);
    switch (ch) {
      case ('Y' - 64):
      case ('C' - 64):
#ifndef COMMENT
/*
  Just call the same handler that signal(SIGINT,xxx) would have invoked
  if Ctrl-C had been trapped.  The pointer to the handler was saved in
  cctrap by conint().
*/
        if (cctrap)
          (*cctrap)(SIGINT,0);
#else
        ttclos(ttyfd);                  /* Close down other terminal    */
        conres();                       /* And cleanup console modes    */
        exit(SS$_ABORT);                /* Fatal exit.                  */
#endif /* COMMENT */
      case ('Z' - 64):
        ateof = 1;
        return (EOF);

      default:
        return (ch);
    }
}

/*  C O N T T I  --  Get character from console then from tty  */
/*
  This is used in conect() when NO_FORK is defined.
  src is returned with 1 if the character came from the comm. line,
  0 if it was from the console, and with -1 if there was any error.
*/
#ifdef TCPIPLIB
/*
 * Network/console read posted?
 */
static int      nettty_queued   = 0;
static int      netcon_queued   = 0;
#endif /* TCPIPLIB */

#ifdef CK_SSL
#ifdef TCPIPLIB
int
ssl_contti(c, src) int *c, *src; {

#ifdef CMU_TCPIP
    int s;                                      /* select status */
    fd_set exceptfds;                           /* select exceptions */
    fd_set readfds;                             /* select read */
    static struct timeval timeout;              /* for non-blocking select */
#endif /* CMU_TCPIP */

#define NET_EFN 7                               /* Network event flag */

    int                         mask;           /* Event flag mask */

    static CHAR                 concc;          /* Console and network data */
    static CHAR                 netcc;

    static struct iosb_struct   net_iosb;
    static struct iosb_struct   con_iosb;       /* IO status blocks */

/* Buffered network data, count, next character.  Declared in CKCNET.C ... */

    extern CHAR                 ttibuf[];
    extern int                  ttibn;
    extern int                  ttibp;
#ifdef NETLEBUF
    /* See NETLEBUF in ckcnet.h and ckcnet.c */
    extern int                  ttpush, le_data;
#endif /* NETLEBUF */
    int                         avail;

#ifdef DEBUG
    if (deblog) {
        debug(F101,"ssl_contti: network","",network);
        debug(F101,"ssl_contti: ttyfd","",ttyfd);
        debug(F101,"ssl_contti: ttychn","",ttychn);
    }
#endif /* DEBUG */

    *src = -1;                          /* Assume there was an error */

    if (network && ttyfd < 0)           /* Make sure we're not called */
      return(-1);                       /* out of context... */
    if (!network && !ttychn)
      return(-1);
    if (!(ssl_active_flag || tls_active_flag))
      return(-1);

    debug(F100,"ssl_contti: network","",0);
    debug(F101,"ssl_contti: ttibn","",ttibn);
    debug(F101,"ssl_contti: ttpush","",ttpush);
    debug(F101,"ssl_contti: le_data","",le_data);

    /*
     * Handle the case where data remains in our "internal" buffer.
     * We need to:
     *
     *      -- Handle the console keyboard (is a character ready?)
     *      -- Return one character from the network buffer if not
     *
     * Post a new console read if necessary
     */
    if (!netcon_queued) {
#ifdef CMU_TCPIP
	cmu_stdin_read(IO$_READVBLK, &concc, 1, 0, 0);
#else
	debug(F100,"ssl_contti: sys$qio conchn 1","",0);
	if (!CHECK_ERR("ssl_contti: console sys$qio",
			sys$qio(CON_EFN, conchn, IO$_READVBLK,
				 &con_iosb, 0, 0, &concc,
				 1, 0, 0, 0, 0))) {
	    debug(F100,"ssl_contti: sys$qio conchn 1 fails","",0);
	    return(*c = -1);
	}
	debug(F100,"ssl_contti: sys$qio conchn 2","",0);
#endif /* CMU_TCPIP */
	netcon_queued = 1;
    }

    /* Console char ready? */
#ifdef CMU_TCPIP
    FD_ZERO(&exceptfds);
    FD_SET(0,&exceptfds);
    FD_ZERO(&readfds);
    FD_SET(0,&readfds);
#ifdef CMU_TCPIP_BOGUS_SELECT
    if (select(1, &readfds, 0, &exceptfds, &timeout) == 1)
	if (FD_ISSET(0,&exceptfds))
	    return(*c = -1);
    netcon_queued = 0;
    *c   = (unsigned)(concc & 0xff);
    *src = 0;
    return(1);
#else   /* CMU_TCPIP_BOGUS_SELECT (wjm 02-feb-1997) */
	/* non-blocking select() - zero timeout */
    switch(select(1, &readfds, 0, &exceptfds, &timeout)) {
    case 1:       /* console ready */
	netcon_queued = 0;                  /* QIO completed */
	*src = 0;
	if (!(FD_ISSET(0,&exceptfds))) {    /* o.k. */
	    *c = (unsigned)(concc & 0xff);
	    return(1);
	} else                              /* I/O error */
	    return(*c = -1);

    case 0:       /* timeout (no console input) */
	break;

    default:      /* select() error */
	return(*c = -1);            /* shouldn't get here */
    }
#endif /* CMU_TCPIP_BOGUS_SELECT (wjm 02-feb-1997) */
#else /* CMU_TCPIP */
    (void) sys$readef(CON_EFN, &mask);
    if (mask & (1 << CON_EFN)) {
	netcon_queued = 0;
	if (!CHECK_ERR("ssl_contti: con_iosb.status",
			con_iosb.status)) {
	    debug(F100,"ssl_contti: con_iosb.status fails","",0);
	    return(*c = -1);
	}
	*c = (unsigned)(concc & 0xff);
	*src = 0;
	return(1);
    }
#endif /* CMU_TCPIP */
    /*
     * No console data; return buffered network character
     */

    avail = nettchk();
    debug(F101,"ssl_contti: nettchk","",avail);
    if (avail > 0) {
	*c = netinc(0);              /* See ttbufr() in ckcnet.c */
	return(*src = (*c > -1) ? 1 : 0);
    } else if (avail < 0) {
	return(*src = -1);
    }
    /* all clear up to here - jaltman */

    /*
     * No buffered data; post network and console reads
     */
    debug(F101,"ssl_contti: nettty_queued","",nettty_queued);
    if (!nettty_queued) {
#ifdef CMU_TCPIP

/* Network read is always posted */

#else /* CMU_TCPIP */

/*     -lt.  1992-09-14  begin */
/* All the event flag numbers should be obtained using lib$get_ef().
 * Using hard coded numbers, especially < 31 is tres dangereuse!!!
 * Be careful, one must also change the event flag cluster used by
 * sys$readef. It is *not* just a simple matter of changing a few #defines.
 *
 * At least for DEC TCP/IP Services, socket calls return a proper file
 * descriptor (fd).  OpenVMS system services require a channel
 * (from sys$assign).  The two are *not* the same.  The call vaxc$get_sdc()
 * maps from a DEC TCP/IP fd to a channel.
 *
 * This is "gag me with a spoon" code, but it gets thing up and running.
 *
 */
#ifdef DEC_TCPIP
	{
	    static int last_ttyfd = -1;
	    static short int net_chan = -1;

	    if (ttyfd != last_ttyfd) {
		last_ttyfd = ttyfd;
		net_chan = GET_SDC(last_ttyfd);
	    }
	    if (!CHECK_ERR("ssl_contti: network sys$qio",
			    sys$qio(NET_EFN, net_chan, IO$_READVBLK,
				     &net_iosb, 0, 0,
				     &netcc, 1, 0, INET$C_MSG_PEEK, 0, 0))) {
		debug(F100,"ssl_contti: network sys$qio net_chan fails","",0);
		return(*c = -1);
	    }
	}
#else /* Not DEC_TCPIP */
	if (!CHECK_ERR("ssl_contti: network sys$qio",
			sys$qio(NET_EFN, ttyfd, IO$_READVBLK, &net_iosb,
				 0, 0, &netcc, 0, 0, INET$C_MSG_PEEK, 0, 0))) {
	    debug(F100,"ssl_contti: network sys$qio ttyfd fails","",0);
	    return(*c = -1);
	}
#endif /* DEC_TCPIP */
#endif /* CMU_TCPIP */
	nettty_queued = 1;
    }

    debug(F101,"ssl_contti: netcon_queued","",netcon_queued);
    if (!netcon_queued) {
#ifdef CMU_TCPIP
	cmu_stdin_read(IO$_READVBLK, &concc, 1, 0, 0);
#else
	if (!CHECK_ERR("ssl_contti: console sys$qio",
			sys$qio(CON_EFN, conchn, IO$_READVBLK, &con_iosb,
				 0, 0, &concc, 1, 0, 0, 0, 0))) {
	    debug(F100,"ssl_contti: console sys$qio fails","",0);
	    return(*c = -1);
	}
#endif /* CMU_TCPIP */
	netcon_queued = 1;
    }
    /*
     * Wait for a character
     */
#ifdef CMU_TCPIP
    debug(F100,"CMU_TCPIP","",0);
    FD_ZERO(&exceptfds);
    FD_SET(0,&exceptfds);
    FD_SET(ttyfd,&exceptfds);
    FD_ZERO(&readfds);
    FD_SET(0,&readfds);
    FD_SET(ttyfd,&readfds);
    s = select(ttyfd+1, &readfds, 0, &exceptfds, 0); /*a blocking select*/
    if (s <= 0)
	return(-1);

    if (FD_ISSET(0,&exceptfds))
	return(-1);

    if (FD_ISSET(ttyfd,&exceptfds))
	return(-1);

    if (FD_ISSET(0,&readfds)) {
	*c            = (unsigned)(concc & 0xff);
	*src          = 0;
	netcon_queued = 0;
    } else {
	if (FD_ISSET(ttyfd,&readfds)) {
	    *c = netinc(0);
	    if (*c < 0)
		return(-1);
	    *src      = 1;
	    nettty_queued = 0;
	    return(1);
	}
    }
#else /* CMU_TCPIP */
    debug(F100,"not CMU_TCPIP","",0);
    mask = (1 << CON_EFN) | (1 << NET_EFN);

    if (!CHECK_ERR("ssl_contti: sys$wflor", sys$wflor(CON_EFN, mask))) {
	debug(F100,"ssl_contti: sys$wflor fails", "", 0);
	return(*c = -1);
    }
    if (!CHECK_ERR("ssl_contti: sys$readef",sys$readef(CON_EFN, &mask))) {
	debug(F100,"ssl_contti: sys$readef fails", "", 0);
	return(*c = -1);
    }
    if (mask & (1 << CON_EFN)) {    /* Console */
	if (!CHECK_ERR("ssl_contti: con_iosb.status", con_iosb.status)) {
	    debug(F100,"ssl_contti: con_iosb.status fails","",0);
	    return(-1);
	}
	*c = (unsigned)(concc & 0xff);
	*src = 0;
	netcon_queued = 0;
    }
    else if (mask & (1 << NET_EFN)) { /* Network */
	if (!(net_iosb.status & 1)) { /* Read error */
#ifdef WINTCP
#ifdef OLD_TWG
	    perror("ssl_contti: net_iosb.status");
#else
	    _$set_vaxc_error(SS$_NORMAL, net_iosb.status);
	    win$perror("ssl_contti: net_iosb.status");
#endif /* OLD_TWG */
#endif /* WINTCP */
	    debug(F100,"ssl_contti: network read error", "", 0);
	    return(*c = -1);
	}
	debug(F101,"ssl_contti: net_iosb.size","",net_iosb.size);
	if (net_iosb.size == 0) {   /* Handle reset from remote */
	    debug(F100,"ssl_contti: network reset from remote", "", 0);
	    return(*c = -1);
	}
	/* We ignore the character we peeked at and call the net code */
	debug(F100,"ssl_contti: calling netinc(0)","",0);
	*c = netinc(0);
	*src = 1;
	nettty_queued = 0;
    }
#endif /* CMU_TCPIP */

    debug(F101,"ssl_contti *src","",*src);
    return((*src > -1) ? 1 : 0);
}
#endif /* TCPIPLIB */
#endif /* CK_SSL */

int
contti(c, src) int *c, *src; {

#ifndef TCPIPLIB
    int mask = 1<<CON_EFN | 1<<TTY_EFN;
    int x; unsigned char cc;

#else /* TCPIPLIB */

#ifdef CMU_TCPIP
    int s;                                      /* select status */
    fd_set exceptfds;                           /* select exceptions */
    fd_set readfds;                             /* select read */
    static struct timeval timeout;              /* for non-blocking select */
#endif /* CMU_TCPIP */

#define NET_EFN 7                               /* Network event flag */

    int                         mask;           /* Event flag mask */

    static CHAR                 concc;          /* Console and network data */
    static CHAR                 netcc;

    static struct iosb_struct   net_iosb;
    static struct iosb_struct   con_iosb;       /* IO status blocks */

/* Buffered network data, count, next character.  Declared in CKCNET.C ... */

    extern CHAR                 ttibuf[];
    extern int                  ttibn;
    extern int                  ttibp;
#ifdef NETLEBUF
    /* See NETLEBUF in ckcnet.h and ckcnet.c */
    extern int                  ttpush, le_data;
#endif /* NETLEBUF */
#endif /* TCPIPLIB */

#ifdef DEBUG
    if (deblog) {
        debug(F101,"contti: network","",network);
        debug(F101,"contti: ttyfd","",ttyfd);
        debug(F101,"contti: ttychn","",ttychn);
    }
#endif /* DEBUG */

    *src = -1;                          /* Assume there was an error */

    if (network && ttyfd < 0)           /* Make sure we're not called */
      return(-1);                       /* out of context... */
    if (!network && !ttychn)
      return(-1);

#ifdef TCPIPLIB
    if (network) {                      /* For active network connections */
        debug(F100,"contti: network","",0);
        debug(F101,"contti: ttibn","",ttibn);

        if (ttibn > 0 || ttpush > 0 || le_data > 0) {
                /*
                 * Handle the case where data remains in our "internal" buffer.
                 * We need to:
                 *
                 *      -- Handle the console keyboard (is a character ready?)
                 *      -- Return one character from the network buffer if not
                 *
                 * Post a new console read if necessary
                 */
                if (!netcon_queued) {
#ifdef CMU_TCPIP
                        cmu_stdin_read(IO$_READVBLK, &concc, 1, 0, 0);
#else
                        debug(F100,"contti: sys$qio conchn 1","",0);
                        if (!CHECK_ERR("contti: console sys$qio",
                                       sys$qio(CON_EFN, conchn, IO$_READVBLK,
                                               &con_iosb, 0, 0, &concc,
                                               1, 0, 0, 0, 0))) {
                            debug(F100,"contti: sys$qio conchn 1 fails","",0);
                            return(*c = -1);
                        }
                        debug(F100,"contti: sys$qio conchn 2","",0);
#endif /* CMU_TCPIP */
                        netcon_queued = 1;
                }
                /* Console char ready? */
#ifdef CMU_TCPIP
                FD_ZERO(&exceptfds);
                FD_SET(0,&exceptfds);
                FD_ZERO(&readfds);
                FD_SET(0,&readfds);
#ifdef CMU_TCPIP_BOGUS_SELECT
                if (select(1, &readfds, 0, &exceptfds, &timeout) == 1)
                  if (FD_ISSET(0,&exceptfds))
                    return(*c = -1);
                netcon_queued = 0;
                *c   = (unsigned)(concc & 0xff);
                *src = 0;
                return(1);
#else   /* CMU_TCPIP_BOGUS_SELECT (wjm 02-feb-1997) */
                /* non-blocking select() - zero timeout */
                switch(select(1, &readfds, 0, &exceptfds, &timeout)) {
                  case 1:       /* console ready */
                    netcon_queued = 0;                  /* QIO completed */
                    *src = 0;
                    if (!(FD_ISSET(0,&exceptfds))) {    /* o.k. */
                        *c = (unsigned)(concc & 0xff);
                        return(1);
                    } else                              /* I/O error */
                        return(*c = -1);

                  case 0:       /* timeout (no console input) */
                    break;

                  default:      /* select() error */
                    return(*c = -1);            /* shouldn't get here */
                }
#endif /* CMU_TCPIP_BOGUS_SELECT (wjm 02-feb-1997) */
#else
                (void) sys$readef(CON_EFN, &mask);
                if (mask & (1 << CON_EFN)) {
                    netcon_queued = 0;
                    if (!CHECK_ERR("contti: con_iosb.status",
                                   con_iosb.status)) {
                        debug(F100,"contti: con_iosb.status fails","",0);
                        return(*c = -1);
                    }
                    *c = (unsigned)(concc & 0xff);
                    *src = 0;
                    return(1);
                }
#endif /* CMU_TCPIP */
                /*
                 * No console data; return buffered network character
                 */
#ifdef NETLEBUF
                if (ttpush > 0 || le_data > 0) { /* See NETLEBUF in ckcnet.c */
                    *c = netinc(0);     /* See ttbufr() in ckcnet.c */
                } else {
#endif /* NETLEBUF */
                    ttibn--;
                    *c   = (unsigned)(ttibuf[ttibp++]);
#ifdef NETLEBUF
                }
#endif /* NETLEBUF */
                *src = 1;
                return(1);
        }
        /*
         * No buffered data; post network and console reads
         */
        debug(F101,"contti: nettty_queued","",nettty_queued);
        if (!nettty_queued) {

/* Another attempt by fdc to catch a broken connection. */

            int x;
            x = nettchk();
            debug(F101,"contti: nettchk","",x);
            if (x < 0) {
                return(*src = x);
            }
#ifdef CMU_TCPIP

/* Network read is always posted */

#else

/*     -lt.  1992-09-14  begin */
/* All the event flag numbers should be obtained using lib$get_ef().
 * Using hard coded numbers, especially < 31 is tres dangereuse!!!
 * Be careful, one must also change the event flag cluster used by
 * sys$readef. It is *not* just a simple matter of changing a few #defines.
 *
 * At least for DEC TCP/IP Services, socket calls return a proper file
 * descriptor (fd).  OpenVMS system services require a channel
 * (from sys$assign).  The two are *not* the same.  The call vaxc$get_sdc()
 * maps from a DEC TCP/IP fd to a channel.
 *
 * This is "gag me with a spoon" code, but it gets thing up and running.
 *
 */
#ifdef DEC_TCPIP
            {
                static int last_ttyfd = -1;
                static short int net_chan = -1;

                if (ttyfd != last_ttyfd) {
                    last_ttyfd = ttyfd;
#ifdef COMMENT
#ifdef __DECC
                    net_chan = (short) decc$get_sdc(last_ttyfd);
#else
#ifdef VAXC
                    net_chan = vaxc$get_sdc(last_ttyfd);
#else
# error CALL TO GET_SDC requires DECC or VAXC compiler!
#endif  /* VAXC */
#endif  /* DECC */
#else /* COMMENT */
                    net_chan = GET_SDC(last_ttyfd);
#endif /* COMMENT */
                }
                if (!CHECK_ERR("contti: network sys$qio",
                               sys$qio(NET_EFN, net_chan, IO$_READVBLK,
                                       &net_iosb, 0, 0,
                                       &netcc, 1, 0, 0, 0, 0))) {
                    debug(F100,"contti: network sys$qio net_chan fails","",0);
                    return(*c = -1);
                }
            }

#else /* Not DEC_TCPIP */
            if (!CHECK_ERR("contti: network sys$qio",
                           sys$qio(NET_EFN, ttyfd, IO$_READVBLK, &net_iosb,
                                   0, 0, &netcc, 1, 0, 0, 0, 0))) {
                debug(F100,"contti: network sys$qio ttyfd fails","",0);
                return(*c = -1);
            }
#endif /* DEC_TCPIP */
#endif /* CMU_TCPIP */
            nettty_queued = 1;
        }
        if (!netcon_queued) {
#ifdef CMU_TCPIP
            cmu_stdin_read(IO$_READVBLK, &concc, 1, 0, 0);
#else
            if (!CHECK_ERR("contti: console sys$qio",
                           sys$qio(CON_EFN, conchn, IO$_READVBLK, &con_iosb,
                                   0, 0, &concc, 1, 0, 0, 0, 0))) {
                debug(F100,"contti: console sys$qio fails","",0);
                return(*c = -1);
            }
#endif /* CMU_TCPIP */
            netcon_queued = 1;
        }
        /*
         * Wait for a character
         */
#ifdef CMU_TCPIP
        FD_ZERO(&exceptfds);
        FD_SET(0,&exceptfds);
        FD_SET(ttyfd,&exceptfds);
        FD_ZERO(&readfds);
        FD_SET(0,&readfds);
        FD_SET(ttyfd,&readfds);
        s = select(ttyfd, &readfds, 0, &exceptfds, 0); /*a blocking select*/

        if (FD_ISSET(0,&exceptfds))
            return(-1);

        if (FD_ISSET(ttyfd,&exceptfds))
            return(-1);

        if (FD_ISSET(0,&readfds)) {
            *c            = (unsigned)(concc & 0xff);
            *src          = 0;
            netcon_queued = 0;
        } else {
            if (FD_ISSET(ttyfd,&readfds)) {
                s = cmu_read(ttyfd, &netcc, 1);
                if (s <= 0)
                    return(-1);
                *c        = (unsigned)(netcc & 0xff);
                *src      = 1;
                nettty_queued = 0;
            }
        }
#else
        mask = (1 << CON_EFN) | (1 << NET_EFN);

        if (!CHECK_ERR("contti: sys$wflor", sys$wflor(CON_EFN, mask))) {
            debug(F100,"contti: sys$wflor fails", "", 0);
            return(*c = -1);
        }
        if (!CHECK_ERR("contti: sys$readef",sys$readef(CON_EFN, &mask))) {
            debug(F100,"contti: sys$readef fails", "", 0);
            return(*c = -1);
        }
        if (mask & (1 << CON_EFN)) {    /* Console */
            if (!CHECK_ERR("contti: con_iosb.status", con_iosb.status)) {
                debug(F100,"contti: con_iosb.status fails","",0);
                return(-1);
            }
            *c = (unsigned)(concc & 0xff);
            *src = 0;
            netcon_queued = 0;

        } else if (mask & (1 << NET_EFN)) { /* Network */
            if (!(net_iosb.status & 1)) { /* Read error */
#ifdef WINTCP
#ifdef OLD_TWG
                perror("contti: net_iosb.status");
#else
                _$set_vaxc_error(SS$_NORMAL, net_iosb.status);
                win$perror("contti: net_iosb.status");
#endif /* OLD_TWG */
#else
#ifdef MULTINET
#ifdef COMMENT
/*
  When user hangs up, this prints an unnecessary scary message,
  like "Operation would block."
*/
                socket_perror("contti: net_iosb.status");
#endif /* COMMENT */
#endif /* MULTINET */
#endif /* WINTCP */
                debug(F100,"contti: network read error", "", 0);
                return(*c = -1);
            }
            debug(F101,"contti: net_iosb.size","",net_iosb.size);
            if (net_iosb.size == 0) {   /* Handle reset from remote */
                debug(F100,"contti: network reset from remote", "", 0);
                return(*c = -1);
            }
            *c = (unsigned)(netcc & 0xff);
            *src = 1;
            nettty_queued = 0;
        }
#endif /* CMU_TCPIP */
    } else                              /* Not network */
#endif /* TCPIPLIB */

/*
  Should we worry about a network connection that's running under batch ?
*/
    if (!itsatty) {                     /* Batch? */
        debug(F100,"contti batch","",0);
        if ((*c = getchar()) != EOF) {
            *src = 0;
        } else {
            *src = 1;
            *c = ttinc(0);
        }
    } else {                            /* Interactive but not network */

#ifdef TTXBUF
        if (ttxbn > 0) {                /* Buffered port chars available */

/* Post a read on the console if one is not posted already */

            if (!con_queued) {
                if (!CHECK_ERR("contti: console sys$qio",
                             sys$qio(CON_EFN, conchn, IO$_READVBLK,
                                     &coniosb, 0, 0,
                                     &conch, 1, 0, 0, 0, 0)))
                  return(-1);
                con_queued = 1;
            }

/* See if a console character has been read and if so, return it.  */

            (void) sys$readef(CON_EFN, &mask);
            if (mask & (1 << CON_EFN)) {
                con_queued = 0;
                if (!CHECK_ERR("contti: coniosb.status",
                               coniosb.status))
                  return(-1);
                *c   = (unsigned)(conch & 0xff);
                *src = 0;
                return(1);
            }

/* No console character, so return buffered port character */

            *c = ttinc(0);
            *src = 1;
            return(1);
        }

/* No buffered port data; post both network and console reads... */

#endif /* TTXBUF */

        mask = 1<<CON_EFN | 1<<TTY_EFN; /* Event mask */

        debug(F101,"contti interactive mask","",mask);

        if (!con_queued) {              /* Console read not queued... */
            if (!CHECK_ERR("contti: console sys$qio",
                    sys$qio(CON_EFN, conchn, IO$_READVBLK, &coniosb, 0, 0,
                    &conch, 1, 0, 0, 0, 0))) return(-1);
            con_queued = 1;
            debug(F100,"contti con_queued","",0);
        }
        if (!tt_queued) {               /* Port read not queued */
            ttch = -9;                  /* Impossible value */
            if (!CHECK_ERR("contti: tty sys$qio",
                           sys$qio(TTY_EFN, ttychn, IO$_READVBLK,
                                   &ttiosb, 0, 0, &ttch, 1, 0, 0, 0, 0)))
              return(-1);
            tt_queued = 1;
            debug(F100,"contti tt_queued","",0);
        }

/* Wait for one of the queued reads to complete */

        if (!CHECK_ERR("contti: sys$wflor",
            sys$wflor(CON_EFN, mask))) return(-1);
        debug(F100,"contti sys$wflor ok","",0);

/* Read the event flags to see which read was completed */

        if (!CHECK_ERR("contti: sys$readef",
            sys$readef(CON_EFN, &mask))) return(-1);
        debug(F100,"contti sys$readef ok","",0);

/* Return the character with the appropriate source (src) indicator */

        if (!(*src = ((mask & 1<<CON_EFN) ? 0 : 1))) {
            *c = (unsigned)(conch);
            CHECK_ERR("contti: coniosb.status", coniosb.status);
            con_queued = 0;
        } else {
            *c = (ttprty ? (unsigned)(ttch & 0177) : (unsigned)ttch);
            if (ttiosb.status == SS$_HANGUP) {
                debug(F101,"contti hangup ttch","",ttch);
                if (ttcarr != CAR_OFF) {
                    xhangup = 1;
                    fprintf(stderr,"\n%%CKERMIT-F-HANGUP, data set hang-up");
                    *src = -1;
                    return(1);
                } else if (ttch == -9) {
                    tt_queued = 0;
                    *src = -2;
                    return(1);
                }
            } else if (ttiosb.status != SS$_HANGUP) {
                CHECK_ERR("contti: ttiosb.status", ttiosb.status);
            }
            tt_queued = 0;
        }
        if (!(vms_status & 1)) *src = -1;
    }
    debug(F101,"contti *src","",*src);
    return((*src > -1) ? 1 : 0);
}

/*
  C A N C I O
  Cancel pending I/O requests on console and communication device.
*/

int
ck_cancio() {

    debug(F101,"ck_cancio: ttyfd","",ttyfd);
    if (network && ttyfd < 0)
      return(0);
    debug(F101,"ck_cancio: ttychn","",ttychn);
    if (!network && !ttychn)
      return(0);
#ifdef NETCONN
    if (network) {
#ifdef TCPIPLIB
#ifdef DEC_TCPIP
        short int net_chan = -1;

        net_chan = GET_SDC(ttyfd);
        if (nettty_queued) (void) sys$cancel(net_chan);
        nettty_queued = 0;              /*+wjm*/
#else  /* DEC_TCPIP */
#ifdef CMU_TCPIP
    /* not going to do this when CMU is the network transport.
     * the sys$cancel will cause the channel to shutdown
     *
     * if (nettty_queued) (void) sys$cancel(cmu_get_sdc(ttyfd));
     */
#else  /* !CMU_TCPIP */
       if (nettty_queued) (void) sys$cancel(ttyfd);
        nettty_queued = 0;              /*+wjm*/
#endif /* CMU_TCPIP */
#endif /* DEC_TCPIP */
        if (netcon_queued) (void) sys$cancel(conchn);
        netcon_queued = 0;
        return(0);
#else /* Not TCPIPLIB */
        return(0);
#endif /* TCPIPLIB */
    }
#endif /* NETCONN */

    if (itsatty) {
        CHECK_ERR("ck_cancio: console sys$cancel",
            sys$cancel(conchn));
        CHECK_ERR("ck_cancio: tty sys$cancel",
            sys$cancel(ttychn));
        con_queued = 0;
        tt_queued = 0;
    }
    return(0);
}

/* get_qio_maxbuf_size()
 *
 * Get maximum size of QIO that can occur without getting the dreaded
 * exceeded quota status.
 */

#ifndef SYI$_MAXBUF
#define SYI$_MAXBUF 4175
#endif /* SYI$_MAXBUF */

int
get_qio_maxbuf_size(ttychn) unsigned long int ttychn; {
    unsigned char *tmpbuf;
    int unsigned long max=0;
    struct itmlst syiitm[] = { {2,SYI$_MAXBUF,NULL,0},
                {0,0,0,0}};

    syiitm[0].adr = (char *)&max;

    if (!ttychn) return(-1);

    if (!CHECK_ERR("get_qio_maxbuf_size: sys$getsyiw",
        sys$getsyiw(     0      /* efn */
                        ,0      /* csidadr */
                        ,0      /* nodename */
                        ,&syiitm /* itmlst */
                        ,&wrk_iosb /* iosb */
                        ,0      /* astadr */
                        ,0)))   /* astprm */
                exit(SS$_ABORT);                /* Fatal exit */

    if (!(tmpbuf = malloc(max)))
        return(0);

    for (; max > 0; max -= 16) {
        if (!test_qio(ttychn,max,tmpbuf)) /* (was &tmpbuf, caused crash) */
        {
            free(tmpbuf);
            return(max);
        }
    }

    free(tmpbuf);
    printf("\n%%CKERMIT-F-get_qio_maxbuf_size, Could not get maxbuf size\n");
    exit(SS$_ABORT);            /* Fatal exit */
}

int
test_qio(ttychn,max,dest)
unsigned long int ttychn;
long int max;
unsigned char *dest;
{
    static int trmmsk[2] = {0,0};

/*    trmmsk[1] = 1 << eol; */

    vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_READVBLK|IO$M_TIMED,
                          &wrk_iosb, 0, 0, dest, max, 0, &trmmsk, 0, 0);
    if (!(vms_status & 1)) vms_lasterr = vms_status;
    return( !(vms_status & 1) ||
        (!(wrk_iosb.status & 1)) && wrk_iosb.status != SS$_TIMEOUT);
}


/* Flush communications device output buffer */

int
ttfluo() {

    long n=0;

#ifdef NETCONN
    if (network) return(0);
#endif /* NETCONN */

    if (!ttychn) return(-1);            /* Not open. */

    if (!CHECK_ERR("ttfluo: sys$qiow",
        sys$qiow(QIOW_EFN, ttychn, IO$_READVBLK|IO$M_TIMED|IO$M_PURGE,
                 &wrk_iosb, 0, 0, &n, 0, 0, 0, 0, 0))) {
        perror("flush failed");
        return(-1);
        }
    return(0);
}

/*  T T G M D M  --  Get modem signals  */
/*
 Looks for the modem signals CTS, DSR, and CTS, and returns those that are
 on in as its return value, in a bit mask as described for ttwmdm.  Returns:
 -3 Not implemented
 -2 if the line does not have modem control
 -1 on error.
 >= 0 on success, with a bit mask containing the modem signals that are on.
*/
int
ttgmdm() {
    struct {
        unsigned char type;
        unsigned char spare1;
        unsigned char modem;
        unsigned char spare2;
        unsigned long filler;
    } mdminfo;
    int retval;

#ifdef NETCONN
    if (network) {
#ifdef TN_COMPORT
        if (istncomport())
          return(tngmdm());
        else
#endif /* TN_COMPORT */
          return(-2);
    }
#endif /* NETCONN */
/*
  According to the VMS I/O manual, devices that do not have the TT$M_MODEM
  characteristics can have their modem signals read using IO$M_MAINT.
*/
    /* This does not work on inbound ports or on LAT devices */
    vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_SENSEMODE|IO$M_RD_MODEM,
                          &wrk_iosb, 0, 0, &mdminfo, 0, 0, 0, 0, 0);
    if (!(vms_status & 1)) vms_lasterr = vms_status;
    if (vms_status != SS$_NORMAL) {
        debug(F101,"ttgmdm serious error, status","",vms_status);
        debug(F101,"ttgmdm ttychn","",ttychn);
        return(-1);
    }
    debug(F101,"ttgmdm iosb","",wrk_iosb.status);
    debug(F101,"ttgmdm type","",mdminfo.type);
    debug(F101,"ttgmdm modem","",mdminfo.modem);

    if (wrk_iosb.status != SS$_NORMAL) {
        debug(F101,"ttgmdm iosb error, status","",wrk_iosb.status);
        return(-1);
    }

#ifdef DT$_LAT
    if (mdminfo.type == DT$_LAT) {
        debug(F101,"ttgmdm LAT port, no modem control","",0);
        return(-2);
    }
#endif /* DT$_LAT */

#ifndef VMS64BIT			/* This works only on VAX */
/*
  It was suggested that we test for TT$M_MODEM here, but if it failed
  that does not necessarily mean we can't read modem signals.  As of
  now (12/1997) no reliable test is known that tells whether we can believe
  the mdminfo.modem bits.  But hopefully we won't get this far in that case.
*/
    if (mdminfo.type == 0) {
        debug(F101,"ttgmdm unknown driver, modem","",mdminfo.modem);
        return(-2);
    }
#endif /* VMS64BIT */

    retval = BM_DTR | BM_RTS;           /* Not visible, set by TTDRIVER */
    if (mdminfo.modem & TT$M_DS_CTS)
        retval |= BM_CTS;
    if (mdminfo.modem & TT$M_DS_DSR)
        retval |= BM_DSR;
    if (mdminfo.modem & TT$M_DS_CARRIER)
        retval |= BM_DCD;
    if (mdminfo.modem & TT$M_DS_RING)
        retval |= BM_RNG;
    return(retval);
}

long
congspd() {
    return(conspd);
}

/*
  Return serial communication device speed.  The speed returned is simply
  the last one set.  Why?  Because VMS apparently provides no method to
  query the speed.
*/
long
ttgspd() {
    int i;
    extern int speed;
    unsigned int vms_speed, s1, s2;
    long x;

#ifdef COMMENT
    struct tt_mode ttmodes;
#endif /* COMMENT */

    debug(F101,"ttgspd ttspeed","",ttspeed);
    debug(F101,"ttgspd network","",network);

#ifdef NETCONN
    if (network) {
#ifdef TN_COMPORT
        if (istncomport())
          return(tnc_get_baud());
        else
#endif /* TN_COMPORT */
          return(-1);                   /* -1 if network connection */
    }
#endif /* NETCONN */

#ifdef COMMENT
/*
  This looks like it should work and in fact it does -- but it returns
  the NOMINAL (permanent) speed of the device, not the current speed.
  So it's useless.  If there is a way to get the actual current speed,
  I'd like to know what it is.
*/
    vms_status = sys$qiow(QIOW_EFN, ttychn, IO$_SENSEMODE, &wrk_iosb, 0, 0,
                          &ttmodes, sizeof(ttmodes), 0, 0, 0, 0);
    if (vms_status != SS$_NORMAL) {
        debug(F101,"ttgspd: sys$qiow error","",vms_status);
        return(-1);
    }
    if (wrk_iosb.status != SS$_NORMAL) { /* Error executing request */
        vms_status = wrk_iosb.status;
        debug(F101,"ttgspd: sys$qiow iosb error","",wrk_iosb.status);
        return(-1);
    }
    x = ttispd((unsigned char)wrk_iosb.size);
    if (x > 0)
      ttspeed = x;
    else
#endif /* COMMENT */
      x = ttspeed;
    debug(F101,"ttgspd returns","",x);
    return(x);
}

/*  T T S C A R R  --  Set ttcarr variable, controlling carrier handling.
 *
 *  0 = Off: Always ignore carrier. E.g. you can connect without carrier.
 *  1 = On: Heed carrier, except during dialing. Carrier loss gives disconnect.
 *  2 = Auto: For "modem direct": The same as "Off".
 *            For real modem types: Heed carrier during connect, but ignore
 *                it anytime else.  Compatible with pre-5A C-Kermit versions.
 *
 * As you can see, this setting does not affect dialing, which always ignores
 * carrier (unless there is some special exception for some modem type).  It
 * does affect ttopen() if it is set before ttopen() is used.  This setting
 * takes effect on the next call to ttopen()/ttpkt()/ttvt().  And they are
 * (or should be) always called before any communications is tried, which
 * means that, practically speaking, the effect is immediate.
 * Of course, nothing of this applies to remote mode (xlocal = 0).
 */
int
ttscarr(carrier) int carrier; {
    ttcarr = carrier;
    debug(F101, "ttscarr","",ttcarr);
    return(ttcarr);
}

int
psuspend(x) int x; {

    return(-1);
}

int
vmsttyfd() {
    return(network ? ttyfd : ttychn);
}

#ifdef CK_CURSES
/*
  tgetent() support for VMS curses emulation.
  Used by all three VMS fullscreen methods.
  Called from "SET FILE DISPLAY FULLSCREEN" in ckuus7.c.
*/
int isvt52 = 0;                         /* VT52/VT1xx flag */

int
tgetent(lp, term) char *lp, *term; {
    debug(F101,"tgetent terminal type","",ccold.type);
    debug(F101,"tgetent terminal extended","",ccold.extended);

    if ((ccold.type == DT$_VT5X) || (ccold.type == DT$_VT55)) {
        debug(F100,"tgetent VT5x","",0);
        isvt52 = 1;
        return(1);
    }
    if ((ccold.extended & TT2$M_ANSICRT) == TT2$M_ANSICRT) {
        debug(F100,"tgetent ANSICRT","",0);
        isvt52 = 0;
        return(1);
    }
    if ((ccold.extended & TT2$M_DECCRT) == TT2$M_DECCRT) {
        debug(F100,"tgetent DECCRT","",0);
        isvt52 = 0;
        return(1);
    }
    return(0);                          /* Not a supported terminal type */
}
#endif /* CK_CURSES */

#ifdef CMDATE2TM
struct tm *
#ifdef CK_ANSIC
cmdate2tm(char * date, int gmt)         /* date as "yyyymmdd hh:mm:ss" */
#else
cmdate2tm(date,gmt) char * date; int gmt;
#endif
{
    /* date as "yyyymmdd hh:mm:ss" */
    static struct tm _tm;
    time_t now;

    if (strlen(date) != 17 ||
	date[8] != ' ' ||
	date[11] != ':' ||
	date[14] != ':')
      return(NULL);

    time(&now);
    if (gmt)
      _tm = *gmtime(&now); /* Only returns non-NULL for VMS > 6.x ! */
    else
      _tm = *localtime(&now);
    _tm.tm_year = (date[0]-'0')*1000 + (date[1]-'0')*100 +
                  (date[2]-'0')*10   + (date[3]-'0')-1900;
    _tm.tm_mon  = (date[4]-'0')*10   + (date[5]-'0')-1;
    _tm.tm_mday = (date[6]-'0')*10   + (date[7]-'0');
    _tm.tm_hour = (date[9]-'0')*10   + (date[10]-'0');
    _tm.tm_min  = (date[12]-'0')*10  + (date[13]-'0');
    _tm.tm_sec  = (date[15]-'0')*10  + (date[16]-'0');

    /* Should we set _tm.tm_isdst to -1 here? */

    _tm.tm_wday = 0;
    _tm.tm_yday = 0;

    return(&_tm);
}
#endif /* CMDATE2TM */
