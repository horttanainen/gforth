\ envos.fs set os-specific environmental queries 	07jul97jaw

\ Copyright (C) 1997,1999,2000,2003 Free Software Foundation, Inc.

\ This file is part of Gforth.

\ Gforth is free software; you can redistribute it and/or
\ modify it under the terms of the GNU General Public License
\ as published by the Free Software Foundation; either version 2
\ of the License, or (at your option) any later version.

\ This program is distributed in the hope that it will be useful,
\ but WITHOUT ANY WARRANTY; without even the implied warranty of
\ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
\ GNU General Public License for more details.

\ You should have received a copy of the GNU General Public License
\ along with this program; if not, write to the Free Software
\ Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

get-current environment-wordlist set-current
: os-class ( -- c-addr u ) \ gforth-environment
    \G Counted string representing a description of the host operating system.
    s" unix" ;
: os-type ( -- c-addr u ) \ gforth-environment
    \G Counted string equal to "$host_os"
    s" cygwin" ;
set-current
