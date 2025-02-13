#===-----------------------------------------------------------------------===//
#
#                                  tinyOS
#                             The Monix Kernel
#
# 	This program is free software: you can redistribute it and/or modify
# 	it under the terms of the GNU General Public License as published by
# 	the Free Software Foundation, either version 3 of the License, or
# 	(at your option) any later version.
#
# 	This program is distributed in the hope that it will be useful,
# 	but WITHOUT ANY WARRANTY; without even the implied warranty of
# 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# 	GNU General Public License for more details.
#
# 	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#	Copyright (C) 2023-2025, Harry Moulton <me@h3adsh0tzz.com>
#
#===-----------------------------------------------------------------------===//

# Tinylibc sources
KERNEL_SOURCES	+=	libkern/tinylibc/string/memchr.o	\
					libkern/tinylibc/string/memcmp.o	\
					libkern/tinylibc/string/memcpy.o	\
					libkern/tinylibc/string/memmove.o	\
					libkern/tinylibc/string/memset.o	\
					libkern/tinylibc/string/strchr.o	\
					libkern/tinylibc/string/strcmp.o	\
					libkern/tinylibc/string/strlcpy.o	\
					libkern/tinylibc/string/strlen.o	\
					libkern/tinylibc/string/strncmp.o	\
					libkern/tinylibc/string/strnlen.o	\
					libkern/tinylibc/string/strrchr.o	\
					libkern/tinylibc/string/strtoul.o

# Libfdt sources
KERNEL_SOURCES	+=	libkern/libfdt/fdt.o					\
					libkern/libfdt/fdt_addresses.o			\
					libkern/libfdt/fdt_empty_tree.o		\
					libkern/libfdt/fdt_overlay.o			\
					libkern/libfdt/fdt_ro.o				\
					libkern/libfdt/fdt_rw.o				\
					libkern/libfdt/fdt_strerror.o			\
					libkern/libfdt/fdt_sw.o				\
					libkern/libfdt/fdt_wip.o