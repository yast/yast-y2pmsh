#include <cstdio>
#include <cstring>
#include <libtecla.h>

//getpw*
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <iostream>

#include "cmdlineiface_tecla.h"
#include "y2pmsh.h"

#define HISTORYLINES 500

using namespace std;

CPL_MATCH_FN(y2pmsh_completion);

CmdLineIfaceTecla::CmdLineIfaceTecla(const std::string& name) : CmdLineIface(name)
{
    GetLine* gl = new_GetLine(4096, 4096*8);

    gl_customize_completion(gl,NULL,y2pmsh_completion);
    
    _pd = static_cast<void*>(gl);

    if(interactive())
    {
	if(!appname().empty())
	{
	    struct passwd* pwd = getpwuid(getuid());
	    if(pwd)
	    {
		char* home = pwd->pw_dir;
		if(home)
		{
		    _historyfile=home;
		    _historyfile+="/.";
		    _historyfile+=appname()+"_history";
		}
	    }
	}

	gl_limit_history(gl, HISTORYLINES);

	if(!_historyfile.empty())
	{
	    gl_load_history(gl, _historyfile.c_str(), "#");
	}
    }

    gl_configure_getline(gl,"bind M-[5~ history-search-backward",NULL,NULL);
    gl_configure_getline(gl,"bind M-[6~ history-search-backward",NULL,NULL);
}

CmdLineIfaceTecla::~CmdLineIfaceTecla()
{
    _pd = del_GetLine(static_cast<GetLine*>(_pd));
}

bool CmdLineIfaceTecla::getLine(string& line)
{
    char prompt[256] = {0};
    char* buf;

    if(interactive())
	snprintf(prompt,sizeof(prompt)-1,"[%d] y2pm > ", lastRetCode());
    else
	prompt[0]='\0';

    snprintf(prompt,sizeof(prompt)-1,"[%d] y2pm > ", lastRetCode());
    buf = gl_get_line(static_cast<GetLine*>(_pd), prompt, NULL, -1);

    if(buf)
    {
	size_t len = strlen(buf);
	if(buf[len]=='\n')
	    --len;
	line = string(buf,strlen(buf)-1);
    }
    else
	line = "";

    return buf?true:false;
}

void CmdLineIfaceTecla::saveHistory()
{
    if(interactive())
    {
	gl_save_history(static_cast<GetLine*>(_pd),
	    _historyfile.c_str(), "#",  HISTORYLINES);
    }
}

void CmdLineIfaceTecla::addToHistory(const std::string line)
{
}

static bool y2pmsh_complete_packages(WordCompletion *cpl, const char* line,
    int word_start, int word_end, const string& cmd, const string& word)
{
    if(!y2pmsh.initialized())
	return true;

    vector<string> words = y2pmsh.cli().completeCommand(cmd, word);

    if(!words.size())
	return false;

    for (vector<string>::iterator it = words.begin();
	it != words.end(); ++it)
    {
	cpl_add_completion(cpl, line, word_start, word_end, it->c_str()+word_end-word_start, NULL, " ");
    }

    return true;
}

CPL_MATCH_FN(y2pmsh_completion)
{
    int word_start = word_end - 1;
    int len;
    int pos;

    // skip backwards until space or start of line
    while(word_start >= 0 && line[word_start] != ' ')
    {
	word_start--;
    }
    ++word_start;
    len = word_end-word_start;

    // check whether it's the first word
    for(pos = 0; pos < word_end && line[pos] == ' '; ++pos);

    // not first word, try custom command completion or file completion
    if(pos != word_start)
    {
	vector<string> tmp;
	y2pmsh.cli().parsewords(line, tmp);

	if(tmp.size() > 0 &&
	    y2pmsh_complete_packages(cpl, line, word_start, word_end, tmp[0], string(line+word_start, len)))
	{
	    return 0;
	}
	return cpl_file_completions(cpl, data, line, word_end);
    }

    std::vector<std::string> cmdlist =
	y2pmsh.cmd.startswith(string(line+word_start, len));
    for (std::vector<std::string>::iterator it = cmdlist.begin();
	    it != cmdlist.end(); ++it)
    {
	cpl_add_completion(cpl, line, word_start, word_end, it->c_str()+len, NULL, " ");
    }

    return 0;
}

// vim: sw=4
