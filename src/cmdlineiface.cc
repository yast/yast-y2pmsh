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

static void parsecmdline(const char* line, void (*wordcb)(string, void*), void* data);

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

static void wordtovector(string word, void* data)
{
    vector<string>* vec = static_cast<vector<string>* >(data);
    vec->push_back(word);
}

void CmdLineIface::parsewords(const string& line, vector<string>& words)
{
    parsecmdline(line.c_str(), &wordtovector, &words);
}

static inline const char* skipspaces(const char* line)
{
    const char* ptr = line;
    while(isspace(*ptr))
	++ptr;
    return ptr;
}

class State
{
    private:
	unsigned stack[16];
	unsigned pos;
    public:
	State(unsigned num) : pos(0)
	{
	    stack[0] = num;
	}
	void push(unsigned num)
	{
	    ++pos;
	    stack[pos] = num;
	}
	unsigned pop()
	{
	    --pos;
	    return stack[pos];
	}
	bool operator==(unsigned num)
	{
	    return stack[pos] == num;
	}
	State& operator=(unsigned num)
	{
	    push(num);
	    return *this;
	}
	State& operator--(void)
	{
	    pop();
	    return *this;
	}
};

static void parsecmdline(const char* line, void (*wordcb)(string, void*), void* data)
{
    const char* ptr = line;
    string word;
    enum { normal, escape, dquote };
    State state(normal);

    ptr = skipspaces(ptr);

    while(*ptr)
    {
	switch(*ptr)
	{
	    case '\\':
		if(state == escape)
		{
		    --state;
		    word += *ptr;
		    ++ptr;
		}
		else
		{
		    state = escape;
		    ++ptr;
		}
		break;
	    case '"':
		if(state == escape)
		{
		    --state;
		    word += *ptr;
		    ++ptr;
		}
		else if(state == normal)
		{
		    state = dquote;
		    ++ptr;
		}
		else if(state == dquote)
		{
		    --state;
		    ++ptr;
		}
		else
		{
		    word += *ptr;
		    ++ptr;
		}
		break;
	    case ' ':
		if(state == escape)
		{
		    --state;
		    word += *ptr;
		    ++ptr;
		}
		else if(state == dquote)
		{
		    word += *ptr;
		    ++ptr;
		}
		else
		{
		    wordcb(word, data);
		    word = "";
		    ptr = skipspaces(ptr);
		}
		break;
	    default:
		if(state == escape)
		{ // no escape sequences defined yet
		    --state;
		    word += '\\';
		    word += *ptr;
		    ++ptr;
		}
		else
		{
		    word += *ptr;
		    ++ptr;
		}
		break;
	}
    }

    if(word.length() > 0)
    {
	wordcb(word, data);
    }
}

// vim: sw=4
