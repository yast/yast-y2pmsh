#ifndef Y2PMSH_H
#define Y2PMSH_H

#include <vector>
#include <string>
#include <map>

#include "cmdlineiface.h"

#define COMMAND_FUNC(func) int (*func)(std::vector<std::string>& argv)

class Command
{
    public:
	enum { None, Init, Hidden, Advanced, Debug };

	typedef COMMAND_FUNC(CommandFunc);
	
	Command(const char* name, CommandFunc func, unsigned flags, const char* help);

	const std::string& name() const;
	const CommandFunc func() const;
	const unsigned flags() const;
	const std::string& help() const;
	
	int run(std::vector<std::string>& argv) const;

    private:
	std::string _name;
	CommandFunc _func;
	std::string _help;
	unsigned _flags;
};

class Commands
{
    public:
	typedef std::map<std::string,const Command*> CommandList;
	typedef CommandList::const_iterator const_iterator;

	Commands();
	~Commands();

	unsigned count() const;

	void add(const Command*);

	const_iterator begin() const;
	const_iterator end() const;

	std::vector<std::string> startswith(std::string) const;

	const Command* operator[](const std::string&);

    private:
	CommandList _commands;


	
};

class Y2PMSH
{
    private:
	bool _initialized;
	bool _releasemediainstalled; // whether the atexit handler is installed

	bool _interactive;
	bool _shellmode;
	bool _dosetenv;

	CmdLineIface* _cli;

	static const char appname[];

    public:
	Y2PMSH();

	Commands cmd;

	bool initialized();

	int init(std::vector<std::string>& argv);
	
	bool targetinit();

	bool ReleaseMediaAtExit();

	/** whether we are connected to a terminal */
	bool interactive();

	/** whether we read commands from stdin or from command line args */
	bool shellmode();
	
	/** (un)set shellmode */
	void shellmode(bool yes);

	/** set YAST_IS_RUNNING */
	void setenv(bool instsys = false);

	/** access to command line interface */
	CmdLineIface& cli(void);
};

extern Y2PMSH y2pmsh;

#endif
