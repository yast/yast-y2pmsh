#ifndef Y2PMSH_H
#define Y2PMSH_H

#include <list>
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
	~Command();

	const std::string& name() const;
	const CommandFunc func() const;
	const unsigned flags() const;
	const std::string& help() const;
	
	int run(std::vector<std::string>& argv) const;

	class WordExpander
	{
	    public:
		WordExpander() {}
		virtual ~WordExpander() {}
		virtual std::vector<std::string> operator()(const std::vector<std::string>& words)
		{
		    return words;
		}
	};

	void Expander(WordExpander* e);

	WordExpander& Expander();

	bool hasExpander();

    private:
	std::string _name;
	CommandFunc _func;
	std::string _help;
	unsigned _flags;
	WordExpander* _expander;
};

class Commands
{
    public:
	typedef std::map<std::string,Command*> CommandList;
	typedef CommandList::const_iterator const_iterator;

	Commands();
	~Commands();

	unsigned count() const;

	void add(Command*);

	const_iterator begin() const;
	const_iterator end() const;

	std::vector<std::string> startswith(std::string) const;

	Command* operator[](const std::string&);

    private:
	CommandList _commands;


	
};

class QuitHandler;

class Y2PMSH
{
    private:
	bool _initialized;
	bool _releasemediainstalled; // whether the atexit handler is installed
	bool _mediadeleteinstalled;  // whether the atexit handler is installed

	bool _interactive;
	bool _shellmode;
	bool _dosetenv;

	CmdLineIface* _cli;

	static const char appname[];
	
	typedef std::list<QuitHandler*> QuitHandlerList;
	QuitHandlerList _quithandlers;

	void addQuitHandler(QuitHandler*);

    public:
	Y2PMSH();

	Commands cmd;

	bool initialized();

	int init(std::vector<std::string>& argv);
	
	/** initialize target
	 * @param root root directory. Variable "root" is used if empty
	 * */
	bool targetinit(std::string root = "");

	bool ReleaseMediaAtExit();
	
	bool DeleteMediaAtExit();

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

	/** run and destroy quit handlers */
	void quit();
};

extern Y2PMSH y2pmsh;

#endif
