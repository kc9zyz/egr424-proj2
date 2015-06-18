#******************************************************************************
#
# Makefile - Rules for building the proj_2 example.
#
# Copyright (c) 2005-2013 Texas Instruments Incorporated.  All rights reserved.
# Software License Agreement
# 
# Texas Instruments (TI) is supplying this software for use solely and
# exclusively on TI's microcontroller products. The software is owned by
# TI and/or its suppliers, and is protected under applicable copyright
# laws. You may not combine this software with "viral" open-source
# software in order to form a larger program.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
# NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
# NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
# CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
# DAMAGES, FOR ANY REASON WHATSOEVER.
# 
# This is part of revision 10636 of the EK-LM3S6965 Firmware Package.
#
#******************************************************************************

#
# Defines the part type that this project uses.
#
PART=LM3S6965

#
# The base directory for StellarisWare.
#
ROOT=C:/StellarisWare

#
# Include the common make definitions.
#
include ${ROOT}/makedefs

#
# Where to find source files that do not live in this directory.
#
VPATH=C:\StellarisWare\boards\ek-lm3s6965\drivers

#
# Where to find header files that do not live in the source directory.
#
IPATH=..
IPATH+=../../..
IPATH+=C:\StellarisWare
IPATH+=C:\StellarisWare\boards\ek-lm3s6965

#
# The default rule, which causes the proj_2 example to be built.
#
all: ${COMPILER}
all: ${COMPILER}/proj_2.axf

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${COMPILER} ${wildcard *~}

#
# The rule to create the target directory.
#
${COMPILER}:
	@mkdir -p ${COMPILER}

#
# Rules for building the proj_2 example.
#
${COMPILER}/proj_2.axf: ${COMPILER}/my_rit128x96x4.o
${COMPILER}/proj_2.axf: ${COMPILER}/my_uart.o
${COMPILER}/proj_2.axf: ${COMPILER}/my_ssi.o
${COMPILER}/proj_2.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/proj_2.axf: ${COMPILER}/proj_2.o
${COMPILER}/proj_2.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/proj_2.axf: proj_2.ld
SCATTERgcc_proj_2=proj_2.ld
ENTRY_proj_2=ResetISR

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
