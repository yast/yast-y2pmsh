#include <fstream>

#include <Y2PM.h>
#include <y2pm/InstSrcManager.h>
#include <y2pm/InstTarget.h>
#include <y2pm/InstTargetError.h>

#include "cmdlineiface.h"
#ifdef USEREADLINE
#include "cmdlineiface_readline.h"
#elif defined(USETECLA)
#include "cmdlineiface_tecla.h"
#endif

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
	
Y2PMSH::Y2PMSH() : _initialized(false), _shellmode(true), _cli(NULL)
{
    _interactive = ::isatty(0);
    _dosetenv = (::getenv("YAST_IS_RUNNING") == NULL);
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

void Y2PMSH::setenv(bool instsys)
{
    if(!_dosetenv) return;

    if(instsys)
	::setenv("YAST_IS_RUNNING","instsys",1);
    else
	::setenv("YAST_IS_RUNNING","yes",1);
}

class QuitHandler
{
    public:
	virtual void quit() {};
};

class MediaReleaseQuitHandler : public QuitHandler
{
    public:
	virtual void quit()
	{
	    PMError err = Y2PM::instSrcManager().releaseAllMedia();
	    if(err)
		cerr << "Failed to release media: " << err << endl;
	}
};

class DeleteMediaQuitHandler : public QuitHandler
{
    public:
	virtual void quit()
	{
	    InstSrcManager::ISrcIdList isrclist;

	    Y2PM::instSrcManager().getSources(isrclist, true);

	    cout << "clean up sources" << endl;
	    for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
		it != isrclist.end(); ++it)
	    {
		Y2PM::instSrcManager().deleteSource(*it);
	    }
	}
};

bool Y2PMSH::ReleaseMediaAtExit()
{
    if(!_releasemediainstalled)
    {
	_releasemediainstalled = true;
	addQuitHandler(new MediaReleaseQuitHandler());
    }

    return _releasemediainstalled;
}

bool Y2PMSH::DeleteMediaAtExit()
{
    if(!_mediadeleteinstalled)
    {
	_mediadeleteinstalled = true;
	addQuitHandler(new DeleteMediaQuitHandler());
    }

    return _mediadeleteinstalled;
}

void Y2PMSH::addQuitHandler(QuitHandler* q)
{
    _quithandlers.push_back(q);
}

void Y2PMSH::quit()
{
    for(QuitHandlerList::iterator it = _quithandlers.begin();
	it != _quithandlers.end(); ++it)
    {
	(*it)->quit();
	delete(*it);
    }
    _quithandlers.clear();
}

CmdLineIface& Y2PMSH::cli(void)
{
    if(!_cli)
    {
#ifdef USEREADLINE
	_cli = new CmdLineIfaceRL(appname);
#elif defined(USETECLA)
	_cli = new CmdLineIfaceTecla(appname);
#else
	_cli = new CmdLineIface(appname);
#endif
    }

    return *_cli;
}

const char Y2PMSH::appname[] = "y2pmsh";

// vim: sw=4
