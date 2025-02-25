#===-----------------------------------------------------------------------===//
#
#                                  tinyOS
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
#	Copyright (C) 2023-2024, Harry Moulton <me@h3adsh0tzz.com>
#
#===-----------------------------------------------------------------------===//

KERNEL_SOURCES	+=	kern/main.o						\
					kern/cpu.o						\
					kern/processor.o				\
					kern/task.o						\
					kern/thread.o					\
					kern/exception.o				\
					kern/sched.o					\
					kern/machine.o					\
					kern/panic.o					\
					kern/mm/zalloc.o				\
					kern/mm/stack.o					\
					kern/vm/vm.o					\
					kern/vm/vm_page.o				\
					kern/vm/vm_map.o				\
					kern/vm/pmap.o					\
					kern/trace/printk.o				\
					kern/machine/machine_timer.o	\
					kern/machine/machine-irq.o
