/* Gforth support functions

  Copyright (C) 1995,1996,1997,1998,2000 Free Software Foundation, Inc.

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
*/

#include "config.h"
#include "forth.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

#ifdef HAS_FILE
char *cstr(Char *from, UCell size, int clear)
/* return a C-string corresponding to the Forth string ( FROM SIZE ).
   the C-string lives until the next call of cstr with CLEAR being true */
{
  static struct cstr_buffer {
    char *buffer;
    size_t size;
  } *buffers=NULL;
  static int nbuffers=0;
  static int used=0;
  struct cstr_buffer *b;

  if (buffers==NULL)
    buffers=malloc(0);
  if (clear)
    used=0;
  if (used>=nbuffers) {
    buffers=realloc(buffers,sizeof(struct cstr_buffer)*(used+1));
    buffers[used]=(struct cstr_buffer){malloc(0),0};
    nbuffers=used+1;
  }
  b=&buffers[used];
  if (size+1 > b->size) {
    b->buffer = realloc(b->buffer,size+1);
    b->size = size+1;
  }
  memcpy(b->buffer,from,size);
  b->buffer[size]='\0';
  used++;
  return b->buffer;
}

char *tilde_cstr(Char *from, UCell size, int clear)
/* like cstr(), but perform tilde expansion on the string */
{
  char *s1,*s2;
  int s1_len, s2_len;
  struct passwd *getpwnam (), *user_entry;

  if (size<1 || from[0]!='~')
    return cstr(from, size, clear);
  if (size<2 || from[1]=='/') {
    s1 = (char *)getenv ("HOME");
    if(s1 == NULL)
      s1 = "";
    s2 = from+1;
    s2_len = size-1;
  } else {
    UCell i;
    for (i=1; i<size && from[i]!='/'; i++)
      ;
    if (i==2 && from[1]=='+') /* deal with "~+", i.e., the wd */
      return cstr(from+3, size<3?0:size-3,clear);
    {
      char user[i];
      memcpy(user,from+1,i-1);
      user[i-1]='\0';
      user_entry=getpwnam(user);
    }
    if (user_entry==NULL)
      return cstr(from, size, clear);
    s1 = user_entry->pw_dir;
    s2 = from+i;
    s2_len = size-i;
  }
  s1_len = strlen(s1);
  if (s1_len>1 && s1[s1_len-1]=='/')
    s1_len--;
  {
    char path[s1_len+s2_len];
    memcpy(path,s1,s1_len);
    memcpy(path+s1_len,s2,s2_len);
    return cstr(path,s1_len+s2_len,clear);
  }
}
#endif

DCell timeval2us(struct timeval *tvp)
{
#ifndef BUGGY_LONG_LONG
  return (tvp->tv_sec*(DCell)1000000)+tvp->tv_usec;
#else
  DCell d2;
  DCell d1=mmul(tvp->tv_sec,1000000);
  d2.lo = d1.lo+tvp->tv_usec;
  d2.hi = d1.hi + (d2.lo<d1.lo);
  return d2;
#endif
}

DCell double2ll(Float r)
{
#ifndef BUGGY_LONG_LONG
  return (DCell)(r);
#else
  double ldexp(double x, int exp);
  DCell d;
  if (r<0) {
    d.hi = ldexp(-r,-(int)(CELL_BITS));
    d.lo = (-r)-ldexp((Float)d.hi,CELL_BITS);
    return dnegate(d);
  }
  d.hi = ldexp(r,-(int)(CELL_BITS));
  d.lo = r-ldexp((Float)d.hi,CELL_BITS);
  return d;
#endif
}

void cmove(Char *c_from, Char *c_to, UCell u)
{
  while (u-- > 0)
    *c_to++ = *c_from++;
}

void cmove_up(Char *c_from, Char *c_to, UCell u)
{
  while (u-- > 0)
    c_to[u] = c_from[u];
}

Cell compare(Char *c_addr1, UCell u1, Char *c_addr2, UCell u2)
{
  Cell n;

  n = memcmp(c_addr1, c_addr2, u1<u2 ? u1 : u2);
  if (n==0)
    n = u1-u2;
  if (n<0)
    n = -1;
  else if (n>0)
    n = 1;
  return n;
}

Cell memcasecmp(const Char *s1, const Char *s2, Cell n)
{
  Cell i;

  for (i=0; i<n; i++) {
    Char c1=toupper(s1[i]);
    Char c2=toupper(s2[i]);
    if (c1 != c2) {
      if (c1 < c2)
	return -1;
      else
	return 1;
    }
  }
  return 0;
}

struct Longname *listlfind(Char *c_addr, UCell u, struct Longname *longname1)
{
  for (; longname1 != NULL; longname1 = (struct Longname *)(longname1->next))
    if ((UCell)LONGNAME_COUNT(longname1)==u &&
	memcasecmp(c_addr, longname1->name, u)== 0 /* or inline? */)
      break;
  return longname1;
}

struct Longname *hashlfind(Char *c_addr, UCell u, Cell *a_addr)
{
  struct Longname *longname1;

  while(a_addr != NULL) {
    longname1=(struct Longname *)(a_addr[1]);
    a_addr=(Cell *)(a_addr[0]);
    if ((UCell)LONGNAME_COUNT(longname1)==u &&
	memcasecmp(c_addr, longname1->name, u)== 0 /* or inline? */) {
      return longname1;
    }
  }
  return NULL;
}

struct Longname *tablelfind(Char *c_addr, UCell u, Cell *a_addr)
{
  struct Longname *longname1;
  while(a_addr != NULL) {
    longname1=(struct Longname *)(a_addr[1]);
    a_addr=(Cell *)(a_addr[0]);
    if ((UCell)LONGNAME_COUNT(longname1)==u &&
	memcmp(c_addr, longname1->name, u)== 0 /* or inline? */) {
      return longname1;
    }
  }
  return NULL;
}

UCell hashkey1(Char *c_addr, UCell u, UCell ubits)
/* this hash function rotates the key at every step by rot bits within
   ubits bits and xors it with the character. This function does ok in
   the chi-sqare-test.  Rot should be <=7 (preferably <=5) for
   ASCII strings (larger if ubits is large), and should share no
   divisors with ubits.
*/
{
  static char rot_values[] = {5,0,1,2,3,4,5,5,5,5,3,5,5,5,5,7,5,5,5,5,7,5,5,5,5,6,5,5,5,5,7,5,5};
  unsigned rot = rot_values[ubits];
  Char *cp = c_addr;
  UCell ukey;

  for (ukey=0; cp<c_addr+u; cp++)
    ukey = ((((ukey<<rot) | (ukey>>(ubits-rot))) 
	     ^ toupper(*cp))
	    & ((1<<ubits)-1));
  return ukey;
}

struct Cellpair parse_white(Char *c_addr1, UCell u1)
{
  /* use !isgraph instead of isspace? */
  struct Cellpair result;
  Char *c_addr2;
  Char *endp = c_addr1+u1;
  while (c_addr1<endp && isspace(*c_addr1))
    c_addr1++;
  if (c_addr1<endp) {
    for (c_addr2 = c_addr1; c_addr1<endp && !isspace(*c_addr1); c_addr1++)
      ;
    result.n1 = (Cell)c_addr2;
    result.n2 = c_addr1-c_addr2;
  } else {
    result.n1 = (Cell)c_addr1;
    result.n2 = 0;
  }
  return result;
}

Cell rename_file(Char *c_addr1, UCell u1, Char *c_addr2, UCell u2)
{
  char *s1=tilde_cstr(c_addr2, u2, 1);
  return IOR(rename(tilde_cstr(c_addr1, u1, 0), s1)==-1);
}

struct Cellquad read_line(Char *c_addr, UCell u1, Cell wfileid)
{
  UCell u2, u3;
  Cell flag, wior;
  Cell c;
  struct Cellquad r;

  flag=-1;
  u3=0;
  for(u2=0; u2<u1; u2++) {
    c = getc((FILE *)wfileid);
    u3++;
    if (c=='\n') break;
    if (c=='\r') {
      if ((c = getc((FILE *)wfileid))!='\n')
	ungetc(c,(FILE *)wfileid);
      else
	u3++;
      break;
    }
    if (c==EOF) {
      flag=FLAG(u2!=0);
      break;
    }
    c_addr[u2] = (Char)c;
  }
  wior=FILEIO(ferror((FILE *)wfileid));
  r.n1 = u2;
  r.n2 = flag;
  r.n3 = u3;
  r.n4 = wior;
  return r;
}

struct Cellpair file_status(Char *c_addr, UCell u)
{
  struct Cellpair r;
  Cell wfam;
  Cell wior;
  char *filename=tilde_cstr(c_addr, u, 1);

  if (access (filename, F_OK) != 0) {
    wfam=0;
    wior=IOR(1);
  }
  else if (access (filename, R_OK | W_OK) == 0) {
    wfam=2; /* r/w */
    wior=0;
  }
  else if (access (filename, R_OK) == 0) {
    wfam=0; /* r/o */
    wior=0;
  }
  else if (access (filename, W_OK) == 0) {
    wfam=4; /* w/o */
    wior=0;
  }
  else {
    wfam=1; /* well, we cannot access the file, but better deliver a
	       legal access mode (r/o bin), so we get a decent error
	       later upon open. */
    wior=0;
  }
  r.n1 = wfam;
  r.n2 = wior;
  return r;
}

Cell to_float(Char *c_addr, UCell u, Float *rp)
{
  Float r;
  Cell flag;
  char *number=cstr(c_addr, u, 1);
  char *endconv;
  int sign = 0;
  if(number[0]=='-') {
    sign = 1;
    number++;
    u--;
  }
  while(isspace((unsigned)(number[--u])) && u>0)
    ;
  switch(number[u]) {
  case 'd':
  case 'D':
  case 'e':
  case 'E':  break;
  default :  u++; break;
  }
  number[u]='\0';
  r=strtod(number,&endconv);
  if((flag=FLAG(!(Cell)*endconv))) {
    if (sign)
      r = -r;
  } else if(*endconv=='d' || *endconv=='D') {
    *endconv='E';
    r=strtod(number,&endconv);
    if((flag=FLAG(!(Cell)*endconv))) {
      if (sign)
	r = -r;
    }
  }
  *rp = r;
  return flag;
}

Float v_star(Float *f_addr1, Cell nstride1, Float *f_addr2, Cell nstride2, UCell ucount)
{
  Float r;

  for (r=0.; ucount>0; ucount--) {
    r += *f_addr1 * *f_addr2;
    f_addr1 = (Float *)(((Address)f_addr1)+nstride1);
    f_addr2 = (Float *)(((Address)f_addr2)+nstride2);
  }
  return r;
}

void faxpy(Float ra, Float *f_x, Cell nstridex, Float *f_y, Cell nstridey, UCell ucount)
{
  for (; ucount>0; ucount--) {
    *f_y += ra * *f_x;
    f_x = (Float *)(((Address)f_x)+nstridex);
    f_y = (Float *)(((Address)f_y)+nstridey);
  }
}
