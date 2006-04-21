#include <cstdio>
#include <cstring>
#include <readline/readline.h>
#include <readline/history.h>
#include <cctype> // get rid of readline defining isxdigit et al.

//getpw*
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <iostream>

#include "cmdlineiface_readline.h"
#include "y2pmsh.h"

using namespace std;

static char *command_generator (const char *, int);
static char **y2pmsh_completion (const char *, int, int);

CmdLineIfaceRL::CmdLineIfaceRL(const std::string& name) : CmdLineIface(name)
{
    rl_attempted_completion_function = y2pmsh_completion;
    if(!appname().empty())
    {
	rl_readline_name = appname().c_str();

	struct passwd* pwd = getpwuid(getuid());
	if(pwd)
	{
	    char* home = pwd->pw_dir;
	    if(home)
	    {
		::using_history();
		historyfile=home;
		historyfile+="/.";
		historyfile+=appname()+"_history";
		::read_history(historyfile.c_str());
	    }
	}
    }
}

CmdLineIfaceRL::~CmdLineIfaceRL()
{
}

bool CmdLineIfaceRL::getLine(string& line)
{
    char prompt[256] = {0};
    char* buf = NULL;
    if(interactive())
	snprintf(prompt,sizeof(prompt)-1,"[%d] y2pm > ", lastRetCode());
    else
	prompt[0]='\0';

    buf = readline(prompt);

    if(buf)
    {
	line = buf;
	::free(buf);
    }
    else
	line = "";

    return (buf!=NULL);
}

void CmdLineIfaceRL::saveHistory()
{
    // don't save history when non-interactive
    if(!interactive())
	return;

    if(!historyfile.empty())
    {
	int ret = 0;
	if((ret = ::write_history(historyfile.c_str())))
	{
	    cerr << "writing " << historyfile  << " failed: "<<
		strerror(ret) << endl;
	}
	::history_truncate_file(historyfile.c_str(),500);
    }
}

void CmdLineIfaceRL::addToHistory(const std::string line)
{
    // don't save history when non-interactive
    if(!interactive())
	return;

    ::add_history(line.c_str());
}

/* readline command completions */

static char **y2pmsh_completion (const char *text, int start, int end)
{
    char **matches;

    matches = (char **)NULL;

    /* If this word is at the start of the line, then it is a command
       to complete.  Otherwise it is the name of a file in the current
       directory. */
    if (start == 0)
	matches = ::rl_completion_matches (text, command_generator);

    return (matches);
}

/* Generator function for command completion.  STATE lets us know whether to
   start from scratch; without any state (i.e. STATE == 0), then we start at
   the top of the list. */
static char *command_generator (const char *text, int state)
{
    static int list_index, len;
    const char *name;

    /* If this is a new word to complete, initialize now.  This
       includes saving the length of TEXT for efficiency, and initializing the
       index variable to 0. */
    if (!state)
    {
	list_index = 0;
	len = ::strlen (text);
    }

    /* Return the next name which partially matches from the
       command list. */
    while ((name = commands[list_index].name))
    {
	++list_index;

	if (strncmp (name, text, len) == 0)
	    return (::strdup(name));
    }

    /* If no names matched, then return NULL. */
    return ((char *)NULL);
}


// vim: sw=4
