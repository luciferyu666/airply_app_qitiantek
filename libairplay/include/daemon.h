#ifndef _DAEMON_H
#define _DAEMON_H

#include <stdarg.h>
extern "C"
{
    void daemon_init();
    void daemon_ready();
    void daemon_fail(const char *format, va_list arg);
    void daemon_exit();
}
#endif // _DAEMON_H
