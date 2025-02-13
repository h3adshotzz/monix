//===----------------------------------------------------------------------===//
//
//                                  tinyOS
//                             The Monix Kernel
//
// 	This program is free software: you can redistribute it and/or modify
// 	it under the terms of the GNU General Public License as published by
// 	the Free Software Foundation, either version 3 of the License, or
// 	(at your option) any later version.
//
// 	This program is distributed in the hope that it will be useful,
// 	but WITHOUT ANY WARRANTY; without even the implied warranty of
// 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// 	GNU General Public License for more details.
//
// 	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//	Copyright (C) 2023-2025, Harry Moulton <me@h3adsh0tzz.com>
//
//===----------------------------------------------------------------------===//

/**
 * printk is the log/trace interface for the monix kernel, and is loosely based
 * on printk.h from the linux kernel. There are multiple levels of logging, and
 * a maximum log level defined in defaults.h, anything below that log level will
 * not be displayed to the console.
 * 
 * printk will, for the time being, also handle configuring the uart/console,
 * at least until that is moved to it's own interface/driver.
 */

#ifndef __KERN_PRINTK_H__
#define __KERN_PRINTF_H__

#include <tinylibc/stdint.h>
#include <tinylibc/string.h>
#include <tinylibc/stddef.h>

#include <kern/defaults.h>

#define KERN_SOH	"\001"
#define KERN_TEST	KERN_SOH "0"

/**
 * Kernel loglevels. The higher this number goes, the more logging that is
 * enabled. By default, a minimal amount of logging is enabled, the next level
 * is to show critical errors, then warning messages, and finally debug messages
 * 
 * printk will check the loglevel against DEFAULTS_KERNEL_LOGLEVEL. If the
 * loglevel is lower than the defaults level, the log is outputted, otherwise
 * it's skipped.
 */
#define LOGLEVEL_DEFAULT	0	/* default, printed in all cases */
#define LOGLEVEL_CRITICAL	1	/* critical messagess, either panic or error */
#define LOGLEVEL_WARNING	2	/* warn, but not at risk of catastrophe */
#define LOGLEVEL_INFO		3	/* additional information */
#define LOGLEVEL_DEBUG		4	/* very verbose debug info */

/* default loglevel for printk() */
#define TRACE_LOGLEVEL_DEFAULT  LOGLEVEL_DEFAULT

/* printk flags */
#define PK_FLAGS_NONE		0
#define PK_FLAGS_CONT		1	/* continue on the same line */

/* printk api */
extern int _printk(int level, int flags, const char *fmt, ...);
extern int vprintk(const char *fmt, va_list args);

// tmp
#define __strfmt(fmt)	"[   0.0000] " fmt

/**
 * printk, kprintf: print a kernel message
 * 
 * The two main kernel logging functions. These use LOGLEVEL_DEFAULT and so will
 * always be printed. The idea with these two is to use them for messages that
 * really do need to be logged in all cases, and not for verbose or debugging
 * information - for that, please use pr_info or pr_debug.
 * 
 * These are also not expected to have a interface prefix, they'll simply print
 * what's passed to them, prefixed by the timestamp.
 */
#define printk(fmt, ...)	\
	_printk(LOGLEVEL_DEFAULT, PK_FLAGS_NONE, __strfmt(fmt), ##__VA_ARGS__)
#define kprintf(fmt, ...)	\
	_printk(LOGLEVEL_DEFAULT, PK_FLAGS_NONE, __strfmt(fmt), ##__VA_ARGS__)

/**
 * Standard prints for each loglevel
 */
#define pr_err(fmt, ...)	\
	_printk(LOGLEVEL_CRITICAL, PK_FLAGS_NONE, __strfmt(pr_fmt(fmt)), ##__VA_ARGS__)
#define pr_warn(fmt, ...)	\
	_printk(LOGLEVEL_WARNING, PK_FLAGS_NONE, __strfmt(pr_fmt(fmt)), ##__VA_ARGS__)
#define pr_info(fmt, ...)	\
	_printk(LOGLEVEL_INFO, PK_FLAGS_NONE, __strfmt(pr_fmt(fmt)), ##__VA_ARGS__)
#define pr_debug(fmt, ...)	\
	_printk(LOGLEVEL_DEBUG, PK_FLAGS_NONE, __strfmt(pr_fmt(fmt)), ##__VA_ARGS__)

/**
 * Print on the same line, without a uptime tag
 */
#define pr_cont(fmt, ...)	\
	_printk(LOGLEVEL_DEFAULT, PK_FLAGS_CONT, fmt, ##__VA_ARGS__)

#endif /* __kern_printk_h__ */