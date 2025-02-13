# Kernel Trace

This document outlines the design and functionality of the various trace functions within the monix kernel.

---

```
- dmsg/kmsg are part of sysproc, so can't call the logging kmsg


These are #define's that use _printk as a backend
- printk(const char *fmt, ...)
- pr_info, pr_debug, pr_warn, pr_crit

These are the printk backend
- _printk(int level, const char *fmt, ...)
    - takes the logging level, and a list of va args and passes to __vprintk
- __vprintk(int level, const char *fmt, va_list list)
    - checks the console is initialised
    - saves the message to the ring buffer
    - fetches the current time and prints it
    - prints the rest of the message






linux:

- vprintk
    - vprintk_default -> vprintk_emit(LOGLEVEL_DEFAULT)
        - vprintk_store
            - ts_nsec = local_clock();
            - printk_sprint
                - trace_console(text, text_len)

    - printk_parse_prefix

-----------------
OLD

typical printf functions:
- printf
- sprintf
- snprintf
- asprintf
- dprintf
- vprintf
- vsprintf
- vasprintf

additional:
- kprintf
    [ 0.0000] interface: some kind of message
    [ 1.7057] interface: some kind of message
- kmsg
    [ 0.0000] [kmsg] some kind of additional debugging message

** kernel printf **

- a few of the typical printf's can be implemented, each prepended with a 'k'
- traditional printf's will be implemented as part of the libc, and use syscalls
  to print to the console

** ring buffer **

need to eventually implement a kernel ring buffer. each log will be written into
the ring buffer, and then the ring buffer is dumped to the console. some things
to look at with this:
- what is the structure of the ring buffer
- earliest we can create/use it
- how does it print to the screen?
    - each print writes both to the console and ring buffer
    - ability to dump the current contents of the ring buffer


** source structure **

kern/trace
    kprintf.c       -   printf backend
    kmsg.c          -   kmsg, interface logging backend
    trace.c         -   trace backend, ring buffer, ring dump, etc

** TODO **
- rewrite printf backend in kern/trace/kprintf.c
- implement kernel ring buffer, test that it works
- modify kprintf.c to use the ring buffer
- implement kmsg.c

```