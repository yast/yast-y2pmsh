#include <fstream>

#include <Y2PM.h>
#include <y2pm/InstTarget.h>
#include <y2pm/InstTargetError.h>

#include "y2pmsh.h"
#include "variables.h"
#include "instsrc.h"

using namespace std;

Command::Command(const char* name, CommandFunc func, unsigned flags, const char* help)
    : _name(name), _func(func), _help(help), _flags(flags)
{
}

const std::string& Command::name() const
{
    return _name;
}

const Command::CommandFunc Command::func() const
{
    return _func;
}

const unsigned Command::flags() const
{
    return _flags;
}

const std::string& Command::help() const
{
    return _help;
}

int Command::run(std::vector<std::string>& argv) const
{
    return _func(argv);
}

Commands::Commands()
{
}

Commands::~Commands()
{
    for(CommandList::iterator it = _commands.begin();
	    it != _commands.end(); ++it)
    {
	delete it->second;
	it->second = NULL;
    }
}

void Commands::add(const Command* cmd)
{
    if(!cmd) return;
    _commands[cmd->name()] = cmd;
}

Commands::const_iterator Commands::begin() const
{
    return _commands.begin();
}

Commands::const_iterator Commands::end() const
{
    return _commands.end();
}

std::vector<std::string> Commands::startswith(std::string str) const
{
    size_t len = str.length();
    std::vector<std::string> res;
    for(const_iterator it = _commands.begin();
	    it != _commands.end(); ++it)
    {
	if(it->second->name().substr(0,len) == str)
	    res.push_back(it->second->name());
    }
    return res;
}

const Command* Commands::operator[](const std::string& str)
{
    const_iterator it = _commands.find(str);
    if(it == end()) return NULL;
    return it->second;
}

unsigned Commands::count() const
{
    return _commands.size();
}
	
Y2PMSH::Y2PMSH() : _initialized(false), _shellmode(true)
{
    _interactive = ::isatty(0);
}

bool Y2PMSH::initialized()
{
    return _initialized;
}

bool Y2PMSH::targetinit()
{
    cout << "reading installed packages ..." << flush;

    PMError err = Y2PM::instTargetInit(variables["root"].getString());

    cout << err << endl;

    if( err != InstTargetError::E_ok )
    {
	_initialized = false;
	return false;
    }

    Y2PM::instTarget().createPackageBackups(variables["createbackups"].getBool());
    if(variables["cachetodisk"].getBool() == false)
    {
	Y2PM::setCacheToRamdisk(false);
	Y2PM::setNotRunningFromSystem();
    }

    return true;
}

int Y2PMSH::init(vector<string>& argv)
{
    if(_initialized)
    {
	return 0;
    }

    Y2PM::packageManager();
    Y2PM::selectionManager();

    Y2PM::noAutoInstSrcManager();

    if(variables["autosource"].getBool())
    {
	vector<string> v;
	enablesources(v);
    }

    Y2PM::instSrcManager();

    if(!targetinit())
	return 1;

    _initialized = true;

    ReleaseMediaAtExit();

    return 0;
}

bool Y2PMSH::interactive()
{
    return _interactive = ::isatty(0);
}

bool Y2PMSH::shellmode()
{
    return _shellmode;
}

void Y2PMSH::shellmode(bool yes)
{
    _shellmode = yes;
}
