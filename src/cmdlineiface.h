#ifndef Y2PMSH_CMDLINEIFACE_H
#define Y2PMSH_CMDLINEIFACE_H

#include <string>
#include <vector>
#include <map>

class CmdLineIface
{
    public:
	/** primitive completion function for tab expansion */
	class Completer
	{
	    public:
		virtual const std::string command();
		virtual std::vector<std::string> completions(const std::string word);
	};

    private:
	std::string _appname;
	int _lastretcode;
	bool _interactive;
	std::map<std::string, Completer*> completors;

    public:
	/** construct the interface, name is used for the history file */
	CmdLineIface(const std::string& name);
	virtual ~CmdLineIface();

	/** read one line from user, return false on EOF */
	virtual bool getLine(std::string& line);

	/** set return code for prompt */
	void lastRetCode(int retcode);
	int lastRetCode();

	/** get/set appname */
	std::string& appname();
	
	/** add a line to the history file */
	virtual void addToHistory(const std::string cmd);

	/** save the history file to disk */
	virtual void saveHistory();

	/** whether this is an interactive session */
	bool interactive();

	/** tokenzie a line into words */
	void parsewords(const std::string& line, std::vector<std::string>& words);
	
	/** register command completer. don't free afterwards! */
	void registerCompleter(Completer* c);

	/** get completions for command and word */
	std::vector<std::string> completeCommand(const std::string& command, const std::string& word);
};

#endif
