#
# (C) Copyright 2000-2003
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= lib$(CPU).a

START	= start.o

COBJS	= clock.o \
	cpu.o \
	interrupts.o \
	fastboot.o \
	mem.o \
	mmc.o \
	nand.o \
	sys_info.o \
	syslib.o

AOBJS	= lowlevel_init.o

OBJS	= $(COBJS) $(AOBJS)
all:	.depend $(START) $(LIB)

$(LIB):	$(OBJS)
	$(AR) crv $@ $(OBJS)

#########################################################################

.depend:	Makefile $(AOBJS:.o=.S) $(COBJS:.o=.c) $(START:.o=.S)
		$(CC) -M $(CFLAGS) $(AOBJS:.o=.S) $(COBJS:.o=.c) $(START:.o=.S) > $@

sinclude .depend

#########################################################################
