# Makefile to build C-Kermit for Berkeley Unix 4.2

CFLAGS = -std=c89

wermit: ckmain.o ckcmd.o ckuser.o ckprot.o ckfns.o ckconu.o ckxbsd.o ckzbsd.o
	cc -o wermit ckmain.o ckcmd.o ckuser.o ckprot.o ckfns.o ckconu.o ckzbsd.o ckxbsd.o

ckmain.o: ckmain.c ckermi.h

ckuser.o: ckuser.c ckcmd.h ckermi.h

ckcmd.o: ckcmd.c ckcmd.h

ckprot.o: ckprot.w wart.o ckermi.h
	./wart ckprot.w ckprot.c ; cc -c ckprot.c

ckfns.o: ckfns.c ckermi.h

ckzbsd.o: ckzbsd.c ckermi.h

ckxbsd.o: ckxbsd.c

ckconu.o: ckconu.c

wart: wart.o
	cc -o wart wart.o

wart.o: wart.c
