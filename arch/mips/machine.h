/* This is the machine-specific part for MIPS R[2346810]000 processors

  Copyright (C) 1995,1996,1997,1998 Free Software Foundation, Inc.

  This file is part of Gforth.

  Gforth is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef THREADING_SCHEME
#define THREADING_SCHEME 5
#endif

#if !defined(USE_TOS) && !defined(USE_NO_TOS)
/* on the mips this is a mixed blessing, since defining this spills
   the rp with some gcc versions. This machine has 31 regs, yet that's
   not enough for gcc-2.4.5 :-( */
#define USE_TOS
#endif

/* cache flush stuff */

#ifndef INDIRECT_THREADED
#ifndef DIRECT_THREADED
#define DIRECT_THREADED
/* direct threading saves 2 cycles per primitive on an R3000, 4 on an R4000 */
#endif
#endif

#if defined(INDIRECT_THREADED)
#undef DIRECT_THREADED
/* configure may define both on the MIPS */
#endif

#ifdef ultrix
#include <mips/cachectl.h>
#else
/* works on Irix */
#include <sys/cachectl.h>
#endif

#define FLUSH_ICACHE(addr,size) \
			cacheflush((char *)(addr), (int)(size), BCACHE)

#include "../generic/machine.h"

#ifdef DIRECT_THREADED
/* some definitions for composing opcodes */
#define JUMP_MASK	0x03ffffff
#define J_PATTERN	0x08000000
#define JAL_PATTERN	0x0c000000
/* this provides the first 4 bits of a jump address, i.e. it must be <16 */
#define SEGMENT_NUM	1

#define JUMP(addr)	(J_PATTERN|((((unsigned)(addr))&JUMP_MASK)>>2))
#define CALL(addr)	(JAL_PATTERN|((((unsigned)(addr))&JUMP_MASK)>>2))

	/* PFA gives the parameter field address corresponding to a cfa */
#	define PFA(cfa)	(((Cell *)cfa)+2)
	/* PFA1 is a special version for use just after a NEXT1 */
#	define PFA1(cfa)	PFA(cfa)
	/* CODE_ADDRESS is the address of the code jumped to through the code field */
#	define CODE_ADDRESS(cfa)	((Label)(((*(unsigned *)(cfa))^J_PATTERN^(SEGMENT_NUM<<26))<<2))
	/* MAKE_CF creates an appropriate code field at the cfa; ca is the code address */
#	define MAKE_CF(cfa,ca)	({long * _cfa = (long *)(cfa); \
					  _cfa[0] = JUMP(ca); \
					  _cfa[1] = 0; /* nop */})
#	ifdef undefined
		/* the following version uses JAL to make PFA1 faster */
#		define PFA1(label)	({register Cell *pfa asm("$31"); \
						pfa; })
		/* CODE_ADDRESS is the address of the code jumped to through the code field */
#		define CODE_ADDRESS(cfa)	((Label)(((*(unsigned *)(cfa))^JAL_PATTERN^(SEGMENT_NUM<<26))<<2))
#		define MAKE_CF(cfa,ca)	({long *_cfa = (long *)(cfa); \
					  long _ca = (long)(ca); \
						  _cfa[0] = CALL(_ca); \
						  _cfa[1] = 0; /* *(long *)_ca; delay slot */})
#	endif /* undefined */

/* this is the point where the does code starts if cfa points to a
   code field of a does>-defined word */
#define DOES_CODE(cfa)	\
     ({ Xt _cfa=(Xt)(cfa); \
	Label _ca=CODE_ADDRESS(_cfa); \
	((*(unsigned *)_cfa)&(~JUMP_MASK)) == J_PATTERN && \
	(*(unsigned *)_ca) == JUMP(symbols[DODOES]) \
	? _ca+DOES_HANDLER_SIZE : 0; })


/* this is a special version of DOES_CODE for use in dodoes */
#define DOES_CODE1(cfa)	((Xt *)(((char *)CODE_ADDRESS(cfa))+DOES_HANDLER_SIZE))


#	define MAKE_DOES_CF(cfa,does_code) \
			({long does_handlerp=((long)(does_code))-DOES_HANDLER_SIZE; \
			  long *_cfa = (long*)(cfa); \
			  _cfa[0] = JUMP(does_handlerp); \
			  _cfa[1] = 0; /* nop */})
/*
#	define MAKE_DOES_CF(cfa, does_code)	({char *does_handlerp=((char *)does_code)-DOES_HANDLER_SIZE;	\
						  MAKE_CF(cfa,does_handlerp);	\
						  MAKE_DOES_HANDLER(does_handlerp) ;})
*/
	/* this stores a jump dodoes at addr */
#	define MAKE_DOES_HANDLER(addr)	MAKE_CF(addr,symbols[DODOES])

#endif
#ifdef undefined
/* and here are some more efficient versions that can be tried later */

/* the first version saves one cycle by doing something useful in the
   delay slot. !! check that the instruction in the delay slot is legal
*/

#define MAKE_DOESJUMP(addr)	({long * _addr = (long *)addr; \
				  _addr[0] = JUMP(((unsigned)symbols[DODOES])+4); \
				  _addr[1] = *(long *)symbols[DODOES]; /* delay */})

/* the following version uses JAL to make DOES_CODE1 faster */
/* !! does the declaration clear the register ? */
/* it's ok to use the same reg as in PFA1:
   dodoes is the only potential problem and I have taken care of it */

#define DOES_CODE1(cfa)	({register Code *_does_code asm("$31"); \
				    _does_code; })
#define MAKE_DOESJUMP(addr)	({long * _addr = (long *)addr; \
				  _addr[0] = CALL(((long)symbols[DODOES])+4);
				  _addr[1] = *(long *)symbols[DODOES]; /* delay */})

#endif

#ifdef FORCE_REG
#define IPREG asm("$16")
#define SPREG asm("$17")
#define RPREG asm("$18")
#define LPREG asm("$19")
#define CFAREG asm("$20")
#define TOSREG asm("$21")
#endif /* FORCE_REG */
