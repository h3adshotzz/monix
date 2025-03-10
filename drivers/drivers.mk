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

# PL011
PL011_DRV_SOURCES	:=	drivers/pl011/pl011.o

# IRQ
GICV3_DRV_SOURCES	:=	drivers/irq/irq-gicv3.o

# Driver sources
KERNEL_SOURCES		+=	${PL011_DRV_SOURCES}	\
						${GICV3_DRV_SOURCES}
