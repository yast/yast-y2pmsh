#ifndef Y2PMSH_CMDLINEIFACE_H
#define Y2PMSH_CMDLINEIFACE_H

#include <string>
#include <vector>

class CmdLineIface
{
    private:
	std::string _appname;
	int _lastretcode;
	bool _interactive;

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
};

#endif
