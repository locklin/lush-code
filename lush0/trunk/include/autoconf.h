/*   Lush
 *   Copyright (C) 1987-2001 
 *   Leon Bottou, Yann Le Cun, Neuristique S.A., AT&T Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
/***********************************************************************
        $Id: autoconf.h,v 0.1 2001-10-31 17:29:21 profshadoko Exp $
********************************************************************** */

  { "SHELL", "/bin/sh" },
  { "CFLAGS", "-O2 -Wall -Wno-uninitialized -Wstrict-prototypes" },
  { "CPPFLAGS", "" },
  { "CXXFLAGS", "" },
  { "FFLAGS", "" },
  { "DEFS", "-DHAVE_CONFIG_H" },
  { "LDFLAGS", "" },
  { "LIBS", "-lm -ldl " },
  { "exec_prefix", "${prefix}" },
  { "prefix", "/usr/local" },
  { "program_transform_name", "s,x,x," },
  { "bindir", "${exec_prefix}/bin" },
  { "sbindir", "${exec_prefix}/sbin" },
  { "libexecdir", "${exec_prefix}/libexec" },
  { "datadir", "${prefix}/share" },
  { "sysconfdir", "${prefix}/etc" },
  { "sharedstatedir", "${prefix}/com" },
  { "localstatedir", "${prefix}/var" },
  { "libdir", "${exec_prefix}/lib" },
  { "includedir", "${prefix}/include" },
  { "oldincludedir", "/usr/include" },
  { "infodir", "${prefix}/info" },
  { "mandir", "${prefix}/man" },
  { "host", "i686-pc-linux-gnu" },
  { "host_alias", "i686-pc-linux-gnu" },
  { "host_cpu", "i686" },
  { "host_vendor", "pc" },
  { "host_os", "linux-gnu" },
  { "lushdir", "lush/lush" },
  { "SET_MAKE", "" },
  { "CC", "gcc" },
  { "CPP", "gcc -E" },
  { "RANLIB", "ranlib" },
  { "LN_S", "ln -s" },
  { "X_CFLAGS", " -I/usr/X11R6/include" },
  { "X_PRE_LIBS", " -lSM -lICE" },
  { "X_LIBS", " -L/usr/X11R6/lib  -lSM -lICE -lX11 " },
  { "X_EXTRA_LIBS", "" },
  { "have_shlib", "yes" },
  { "cc_pic_flag", "-fPIC" },
  { "cc_exp_flag", "-Wl,-export-dynamic" },
  { "cc_link_shlib", "-shared" },
  { "command_link_shlib", "${CC} ${cc_link_shlib} ${cc_exp_flag} -o" },
  { "exe_suffix", "" },
  { "shlib_suffix", ".so" },
