#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "tty.h"

bool TTY::isatty()
{
    return ::isatty(0);
}

int TTY::width()
{
    if(!::isatty(0))
	return 80;
#ifdef TIOCGWINSZ
    struct winsize size;
    if(::ioctl(0, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
	return size.ws_col;
#endif
    if(::getenv("COLUMNS"))
    {
	int cols = ::atoi(::getenv("COLUMNS"));
	if(cols > 0)
	    return cols;
    }
    return 80; // default
}
