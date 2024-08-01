###############################################################################
# Copyright IBM Corp. and others 1991
# 
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at https://www.eclipse.org/legal/epl-2.0/
# or the Apache License, Version 2.0 which accompanies this distribution and
# is available at https://www.apache.org/licenses/LICENSE-2.0.
#      
# This Source Code may also be made available under the following
# Secondary Licenses when the conditions for such availability set
# forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
# General Public License, version 2 with the GNU Classpath
# Exception [1] and GNU General Public License, version 2 with the
# OpenJDK Assembly Exception [2].
#    
# [1] https://www.gnu.org/software/classpath/license.html
# [2] https://openjdk.org/legal/assembly-exception.html
#       
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
###############################################################################

## This macro should be the first statement in every control
## section. The equivalent of CSECT, but without the label.
##

    .macro    START name
    .align    2

##
## Make the symbol externally visible. All the other names are
## local.
##

    .global   \name

##
## On the face of it, this would have seemed to be the right way to
## introduce a control section. But in practise, no symbol gets
## generated. So I've dropped it in favor of a simple text & label
## sequence. Which gives us what we want.
##
##   .section  \name,"ax",@progbits
##

    .text
    .type   \name,@function
\name:
    .endm


##
## Everything this macro does is currently ignored by the assembler.
## But one day there may be something we need to put in here. This
## macro should be placed at the end of every control section.
##

    .macro    END   name
    .ifndef   \name
    .print    "END does not match START"
    .endif
    .size   \name,.-\name
    .ident    "(c) IBM Corp. 2010. JIT compiler support. \name"
    .endm


    .equ r0,0
    .equ r1,1
    .set CARG1,2
    .set CARG2,3
    .set CARG3,4
    .set CARG4,5
    .set CARG5,6
    .equ r2,2
    .equ r3,3
    .equ r4,4
    .equ r5,5
    .equ r8,8
    .equ r9,9
    .equ r10,10
    .equ r11,11
    .equ r12,12
    .equ r13,13
    .set CRA,14
    .equ r15,15

.align 8
       START _zosZero

         aghi     CARG2,-1
         jl       L2L3
         lgr      r4,CARG1
         srag     r0,CARG2,8
         je       L2L20
         srag     r0,r0,3
         je       LE2K
ZERO2K:
         xc       0(256,r4),0(r4)
         xc       256(256,r4),256(r4)
         xc       512(256,r4),512(r4)
         xc       768(256,r4),768(r4)
         xc       1024(256,r4),1024(r4)
         xc       1280(256,r4),1280(r4)
         xc       1536(256,r4),1536(r4)
         xc       1792(256,r4),1792(r4)
         la       r4,2048(,r4)
         brct     r0,ZERO2K
LE2K:
##       risbg	%r0,%r3,61,191,56
         .long    0xEC033DBF
         .short   0x3855
         je       L2L20
TKLOOP:
         xc       0(256,r4),0(r4)
         la       r4,256(,r4)
         brct     r0,TKLOOP

L2L20:
         larl     r1,L2XC
         ex       CARG2,0(0,r1)
L2L3:
         br       CRA
L2XC:
         xc       0(1,r4),0(r4)

       END _zosZero
