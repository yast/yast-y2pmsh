#include <cstdio>
#include <cstring>
#include <cctype> // get rid of readline defining isxdigit et al.

//getpw*
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <iostream>

#include "cmdlineiface.h"
#include "y2pmsh.h"

using namespace std;

CmdLineIface::CmdLineIface(const string& name) : _appname(name), _lastretcode(0)
{
    _interactive = ::isatty(0);
}

CmdLineIface::~CmdLineIface()
{
}

bool CmdLineIface::getLine(string& line)
{
    char buf[4096] = {0};

    if(!cin)
	return false;

    cin.clear();

    if(_interactive)
	cout << "[" << _lastretcode << "] y2pm > " << flush;

    cin.getline(buf, sizeof(buf));

    line = string(buf);

    return true;
}

void CmdLineIface::saveHistory()
{
}

void CmdLineIface::addToHistory(const std::string line)
{
}

void CmdLineIface::lastRetCode(int retcode)
{
    _lastretcode = retcode;
}

int CmdLineIface::lastRetCode()
{
    return _lastretcode;
}

std::string& CmdLineIface::appname()
{
    return _appname;
}

bool CmdLineIface::interactive()
{
    return _interactive;
}

// vim: sw=4
