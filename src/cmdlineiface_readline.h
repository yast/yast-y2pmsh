#ifndef Y2PMSH_CMDLINEIFACE_READLINE_H
#define Y2PMSH_CMDLINEIFACE_READLINE_H

#include <string>
#include "cmdlineiface.h"

class CmdLineIfaceRL : public CmdLineIface
{
    private:
	std::string historyfile;

    public:
	/** construct the interface, name is used for the history file */
	CmdLineIfaceRL(const std::string& name);
	~CmdLineIfaceRL();

	/** read one line from user, return false on EOF */
	bool getLine(std::string& line);

	/** add a line to the history file */
	void addToHistory(const std::string cmd);

	/** save the history file to disk */
	void saveHistory();
};

#endif
