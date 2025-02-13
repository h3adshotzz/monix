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

#include <drivers/pl011/pl011.h>
#include <kern/trace/printk.h>

/* printk formatting */
#define POPT_LONG		(1 << 1)
#define POPT_LONGLONG	(1 << 2)
#define POPT_PADRIGHT	(1 << 3)

/* longlong to string & hex */
static char *llstr (char *buf, unsigned long long n, int len);
static char *llhex (char *buf, unsigned long long u, int len);

/* internal console api */
static int console_initialised = 0;
static int __console_emits(const char *fmt, va_list list);
static int __console_emit_stdout(char c);

/* printk api */
static int __vprintk(int level, int flags, const char *fmt, va_list args);

/**
 * mid-level printk handler, used as a backend for pr_x macros and takes the
 * loglevel and flags, and passes this to the internal __vprintk.
 */
int _printk(int level, int flags, const char *fmt, ...)
{
	va_list args;
	int res;

	va_start(args, fmt);
	res = __vprintk(level, flags, fmt, args);
	va_end(args);

	return res;
}

/* exposed vprintk */
int vprintk(const char *fmt, va_list args)
{
	return __vprintk(LOGLEVEL_DEFAULT, 0, fmt, args);
}

static int __vprintk(int level, int flags, const char *fmt, va_list args)
{
	uint64_t sec;

	/* check whether the loglevel permits us to continue */
	if (level > DEFAULTS_KERNEL_LOGLEVEL)
		return 0;

	/* check for certains flags */
	if (flags & PK_FLAGS_CONT)
		return __console_emits(fmt, args);

	/**
	 * TODO: ringbuffer
	 * 
	 * need to fetch the uptime value, and then write the format string into a
	 * buffer and create a ringbuffer entry, storing the uptime within that rb
	 * entry. the rb entries would then get printed.
	 * 
	 * would need to rethink the PK_FLAGS_CONT, as we don't want a pr_cont to be
	 * printed outside of the ringbuffer.
	 */

	return __console_emits(fmt, args);
}

/**
 * printk is the trace/logging interface for the kernel (see printk.h), but as
 * there is no console driver, all the console-stuff needs to be done here.
 */
void console_setup()
{
	pl011_init(DEFAULTS_KERNEL_VM_PERIPH_BASE, DEFAULTS_KERNEL_DEBUG_UART_BAUD,
				DEFAULTS_KERNEL_DEBUG_UART_CLK);
	pl011_puts("\n");

	console_initialised = 1;
}

/**
 * emit a single character on the pl011 uart console
 */
static int __console_emit_stdout(char c)
{
	if (c != '\0')
		pl011_putc(c);
	return 0;
}

/**
 * emit a formatted string and va_list args to the console, using
 * __console_emit_stdout
 */
static int __console_emits(const char *fmt, va_list ap)
{
	unsigned long long n;
	const char *str;
	uint32_t opts;
	int width = 0, len = 0;
	char buf[64];
	char c;

	if (!console_initialised)
		return -1;

	for (;;) {
		/**
		 *  Output normal characters over serial, but break if the formatter
		 *  '%' is found.
		 */
		while ((c = *fmt++) != 0) {
			if (c == '%') break;
			__console_emit_stdout(c);
		}

		/* check that the character is not NULL */
		if (c == 0) break;

next:
		/* next character */
		c = *fmt++;
		if (c == 0) break;

		switch (c) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				width = (width * 10) + c - '0';
				goto next;

			case '%':
				__console_emit_stdout('%');
				break;

			case '-':
				opts |= POPT_PADRIGHT;
				goto next;

			case 'c':
				c = va_arg(ap, unsigned int);
				__console_emit_stdout(c);
				break;

			case 'l':
				if (opts & POPT_LONG)
					opts |= POPT_LONGLONG;
				opts |= POPT_LONG;
				goto next;

			case 'i':
			case 'd':
				n = va_arg(ap, unsigned int);
				str = llstr(buf, n, sizeof (buf));
				goto output;

			case 'p':
			case 'x':
				n = va_arg(ap, long long);

				str = llhex(buf, n, sizeof (buf));
				goto output;

			case 's':
				str = va_arg(ap, const char *);
				goto output;
		}

		continue;

output:
		width -= strlen(str);
		if (!(opts & POPT_PADRIGHT))
			while (width-- > 0)
				__console_emit_stdout('0');

		while (*str != 0)
			__console_emit_stdout(*str++);

		if (opts & POPT_PADRIGHT)
			while (width-- > 0)
				__console_emit_stdout(' ');

		width = 0;
		continue;
	}
	return width;
}

static char *llstr (char *buf, unsigned long long n, int len)
{
	int digit, pos = len, neg = 0;

	buf[--pos] = 0;
	while (n >= 10) {
		digit = n % 10;
		n /= 10;
		buf[--pos] = digit + '0';
	}
	buf[--pos] = n + '0';
	return &buf[pos];
}

static char *llhex (char *buf, unsigned long long u, int len)
{
	static const char hextable[] = { '0', '1', '2', '3', '4', '5', '6', '7',
									 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
	unsigned int d;
	int pos = len;

	buf[--pos] = 0;
	do {
		d = u % 16;
		u /= 16;
		buf[--pos] = hextable[d];
	} while (u != 0);
	return &buf[pos];
}
