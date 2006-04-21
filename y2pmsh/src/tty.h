#ifndef Y2PMSH_TTY_H
#define Y2PMSH_TTY_H

struct TTY
{
    static bool isatty();
    static int width();
};

#endif
// vim: sw=4
