
/* WARNING -- This C source program generated by Wart preprocessor. */
/* Do not edit this file; edit the Wart-format source file instead, */
/* and then run it through Wart to produce a new C source file.     */


char *protv = "C-Kermit Protocol Module 4.0(014), 5 Feb 85"; /* -*-C-*- */

/* C K P R O T  -- C-Kermit Protocol Module, in Wart preprocessor notation. */

/* Authors: Jeff Damens, Bill Catchings, Frank da Cruz (Columbia University) */

#include "ckermi.h"

/* Define the states for Wart */

#define rfile 1
#define rdata 2
#define ssinit 3
#define ssdata 4
#define sseof 5
#define sseot 6
#define serve 7
#define generic 8
#define get 9
#define rgen 10

/* Declare external C variables */

  extern char sstate, *versio, *srvtxt, *cmarg, *cmarg2;
  extern char data[], filnam[], srvcmd[], ttname[], *srvptr;
  extern int pktnum, timint, nfils, image, hcflg, xflg, speed, flow;
  extern int prvpkt, cxseen, czseen, server, local, displa, bctu, bctr, quiet;
  extern int putsrv(), puttrm(), putfil(), errpkt();
  extern char *DIRCMD, *DELCMD, *TYPCMD, *SPACMD, *SPACM2, *WHOCMD;

#define SERVE  tinit(); BEGIN serve
#define RESUME if (server) { SERVE; } else return


#define BEGIN state =

int state = 0;

wart()
{
  int c,actno;
  extern int tbl[];
  while (1) {
	c = input();
	if ((actno = tbl[c + state*128]) != -1)
	  switch(actno) {
case 1:
{ tinit();	    	    	    	    	    /* Do Send command */
    if (sinit()) BEGIN ssinit;
       else RESUME; }
break;
case 2:
{ tinit(); BEGIN get; }
break;
case 3:
{ tinit(); srinit(); BEGIN get; }
break;
case 4:
{ tinit(); scmd('C',cmarg); BEGIN rgen; }
break;
case 5:
{ tinit(); scmd('G',cmarg); BEGIN rgen; }
break;
case 6:
{ SERVE; }
break;
case 7:
{ rinit(data); bctu = bctr; BEGIN rfile; }
break;
case 8:
{ srvptr = srvcmd; decode(data,putsrv); /* Get Receive-Init */
	   cmarg = srvcmd;
	   nfils = -1;
    	   if (sinit()) BEGIN ssinit; else { SERVE; } }
break;
case 9:
{ spar(data);			/* Get Init-Parameters */
	   rpar(data);
	   ack1(data);
	   pktnum = 0; prvpkt = -1; }
break;
case 10:
{ srvptr = srvcmd; decode(data,putsrv); /* Get & decode command. */
	   putsrv('\0'); putsrv('\0');
	   sstate = srvcmd[0]; BEGIN generic; }
break;
case 11:
{ srvptr = srvcmd;		/* Get command for shell */
	   decode(data,putsrv);
	   putsrv('\0');
	   if (syscmd("",srvcmd)) BEGIN ssinit;
	   else { errpkt("Can't do shell command"); SERVE; } }
break;
case 12:
{ errpkt("Unimplemented server function"); SERVE; }
break;
case 13:
{ if (!cwd(srvcmd+1)) errpkt("Can't change directory");
    	     SERVE; }
break;
case 14:
{ if (syscmd(DIRCMD,srvcmd+2)) BEGIN ssinit;
    	     else { errpkt("Can't list directory"); SERVE; } }
break;
case 15:
{ if (syscmd(DELCMD,srvcmd+2)) BEGIN ssinit;
    	     else { errpkt("Can't remove file"); SERVE; } }
break;
case 16:
{ ack(); return; }
break;
case 17:
{ if (sndhlp()) BEGIN ssinit;
    	     else { errpkt("Can't send help"); SERVE; } }
break;
case 18:
{ if (syscmd(TYPCMD,srvcmd+2)) BEGIN ssinit;
    	     else { errpkt("Can't type file"); SERVE; } }
break;
case 19:
{ int x;			/* Disk Usage query */
    	     x = *(srvcmd+1);
    	     x = ((x == '\0') || (x == unchar(0)));
	     x = (x ? syscmd(SPACMD,"") : syscmd(SPACM2,srvcmd+2));
    	     if (x) BEGIN ssinit; else { errpkt("Can't check space"); SERVE; }}
break;
case 20:
{ if (syscmd(WHOCMD,srvcmd+2)) BEGIN ssinit;
    	     else { errpkt("Can't do who command"); SERVE; } }
break;
case 21:
{ errpkt("Unimplemented generic server function"); SERVE; }
break;
case 22:
{ decode(data,puttrm); RESUME; }
break;
case 23:
{ if (rcvfil()) { ack(); BEGIN rdata; }
		 else { errpkt("Can't open file"); RESUME; } }
break;
case 24:
{ opent(); ack(); BEGIN rdata; }
break;
case 25:
{ ack(); reot(); RESUME; }
break;
case 26:
{ if (cxseen) ack1("X");
    	   else if (czseen) ack1("Z");
	   else ack();
	   decode(data,putfil); }
break;
case 27:
{ ack(); reof(); BEGIN rfile; }
break;
case 28:
{ resend(); }
break;
case 29:
{  int x; char *s;
    	     spar(data);
    	     bctu = bctr;
	     if (xflg) { x = sxpack(); s = "Can't execute command"; }
	    	  else { x = sfile(); s = "Can't open file"; }
	     if (x) BEGIN ssdata; else { errpkt(s); RESUME; }
          }
break;
case 30:
{ if (canned(data) || !sdata()) {
		clsif();
		seof();
		BEGIN sseof; } }
break;
case 31:
{ if (gnfile() > 0) {
		if (sfile()) BEGIN ssdata;
		else { errpkt("Can't open file") ; RESUME; }
	   } else {
		seot();
		BEGIN sseot; } }
break;
case 32:
{ RESUME; }
break;
case 33:
{ int x;				/* Error packet */
    ermsg(data);			/* Issue message */
    x = quiet; quiet = 1;		/* Close files silently */
    clsif(); clsof();
    quiet = x; RESUME; }
break;
case 34:
{ nack(); }
break;

    }
  }
}

int tbl[] = {
-1, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 33, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 4, 34, 
34, 34, 5, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 3, 1, 34, 34, 2, 34, 6, 
34, 34, 34, 34, 34, 34, 34, -1, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 25, 34, 34, 33, 23, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 24, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 4, 34, 34, 34, 5, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 3, 1, 34, 34, 2, 34, 6, 34, 34, 34, 34, 34, 34, 34, -1, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 26, 33, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 27, 34, 34, 34, 34, 34, 34, 34, 34, 4, 34, 34, 34, 5, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 3, 1, 34, 34, 2, 34, 6, 34, 34, 34, 34, 
34, 34, 34, -1, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 33, 34, 34, 34, 34, 34, 34, 34, 
34, 28, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 29, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 4, 34, 34, 34, 5, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 3, 1, 34, 
34, 2, 34, 6, 34, 34, 34, 34, 34, 34, 34, -1, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
33, 34, 34, 34, 34, 34, 34, 34, 34, 28, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
30, 34, 34, 34, 34, 34, 34, 34, 34, 34, 4, 34, 34, 34, 5, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 3, 1, 34, 34, 2, 34, 6, 34, 34, 34, 34, 34, 34, 34, -1, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 33, 34, 34, 34, 34, 34, 34, 34, 34, 28, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 31, 34, 34, 34, 34, 34, 34, 34, 34, 34, 4, 34, 
34, 34, 5, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 3, 1, 34, 34, 2, 34, 6, 
34, 34, 34, 34, 34, 34, 34, -1, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 33, 34, 34, 34, 
34, 34, 34, 34, 34, 28, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 32, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 4, 34, 34, 34, 5, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 3, 1, 34, 34, 2, 34, 6, 34, 34, 34, 34, 34, 34, 34, -1, 12, 12, 12, 12, 
12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 
12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 
12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 
12, 12, 11, 12, 12, 12, 10, 12, 9, 12, 12, 12, 12, 12, 12, 12, 12, 8, 7, 12, 
12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 4, 12, 12, 12, 5, 12, 
12, 12, 12, 12, 12, 12, 12, 12, 12, 3, 1, 12, 12, 2, 12, 6, 12, 12, 12, 12, 
12, 12, 12, -1, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 
21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 
21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 
21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 13, 14, 15, 16, 21, 17, 21, 21, 21, 21, 
21, 21, 21, 21, 21, 21, 21, 18, 19, 21, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 
21, 21, 4, 21, 21, 21, 5, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 3, 1, 21, 
21, 2, 21, 6, 21, 21, 21, 21, 21, 21, 21, -1, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
33, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 7, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 4, 34, 34, 34, 5, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 3, 1, 34, 34, 2, 34, 6, 34, 34, 34, 34, 34, 34, 34, 0, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 34, 34, 34, 34, 34, 34, 33, 23, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 
34, 34, 7, 34, 34, 34, 34, 24, 22, 34, 34, 34, 34, 34, 34, 34, 34, 34, 4, 34, 
34, 34, 5, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 3, 1, 34, 34, 2, 34, 6, 
34, 34, 34, 34, 34, 34, 34, };



/*  P R O T O  --  Protocol entry function  */

proto() {

    extern int sigint();
    int x;

    conint(sigint);			/* Enable console interrupts */

/* Set up the communication line for file transfer. */

    if (local && (speed < 0)) {
	screen(2,0,"Sorry, you must 'set speed' first");
	return;
    }
    if (ttopen(ttname) < 0) {
	screen(2,0,"Can't open line");
	return;
    }
    x = (local) ? speed : -1;
    if (ttpkt(x,flow) < 0) {		/* Put line in packet mode, */
	screen(2,0,"Can't condition line"); /* setting speed, flow control */
	return;
    }
    if (sstate == 'x') {		/* If entering server mode, */
	server = 1;			/* set flag, */
	if (!quiet) {
	    if (!local)			/* and issue appropriate message. */
	    	conol(srvtxt);
	    else {
	    	conol("Entering server mode on ");
		conoll(ttname);
	    }
	}
    } else server = 0;
    sleep(1);

/*
 The 'wart()' function is generated by the wart program.  It gets a
 character from the input() routine and then based on that character and
 the current state, selects the appropriate action, according to the state
 table above, which is transformed by the wart program into a big case
 statement.  The function is active for one transaction.
*/

    wart();				/* Enter the state table switcher. */

/* Restore the communication line */
    
    ttclos();				/* Close the line. */
    if (server) {
	server = 0;
    	if (!quiet)  			/* Give appropriate message */
	    conoll("C-Kermit server done");
    } else
    	screen(BEL,0,"");		/* Or beep */
}
