#ifndef Y2PMSH_CMDLINEIFACE_TECLA_H
#define Y2PMSH_CMDLINEIFACE_TECLA_H

#include <string>
#include "cmdlineiface.h"

class CmdLineIfaceTecla : public CmdLineIface
{
    private:
	std::string _historyfile;
	void * _pd;

    public:
	/** construct the interface, name is used for the history file */
	CmdLineIfaceTecla(const std::string& name);
	~CmdLineIfaceTecla();

	/** read one line from user, return false on EOF */
	bool getLine(std::string& line);

	/** add a line to the history file */
	void addToHistory(const std::string cmd);

	/** save the history file to disk */
	void saveHistory();
};

#endif
