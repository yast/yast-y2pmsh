/*---------------------------------------------------------------------\
|                                                                      |
|                      __   __    ____ _____ ____                      |
|                      \ \ / /_ _/ ___|_   _|___ \                     |
|                       \ V / _` \___ \ | |   __) |                    |
|                        | | (_| |___) || |  / __/                     |
|                        |_|\__,_|____/ |_| |_____|                    |
|                                                                      |
|                               core system                            |
|                                                     (C) 2002 SuSE AG |
\----------------------------------------------------------------------/

   File:       testinstall.cc
   Purpose:    test dependency solver
   Author:     Ludwig Nussel <lnussel@suse.de>
   Maintainer: Ludwig Nussel <lnussel@suse.de>

/-*/

#undef Y2LOG
#define Y2LOG "y2pmsh"

#include <cstdlib> //atoi
#include <fstream>
#include <sstream>

#include <y2util/timeclass.h>
#include <y2util/stringutil.h>
#include <y2util/Y2SLog.h>
#include <Y2PM.h>
#include <y2pm/InstallOrder.h>
#include <y2pm/RpmDb.h>
#include <y2pm/PkgDep.h>
#include <y2pm/PMPackage.h>
#include <y2pm/InstTarget.h>
#include <y2pm/InstTargetError.h>
#include <y2pm/PMPackageManager.h>
#include <y2pm/PMSelectionManager.h>
#include <y2pm/InstSrcDescr.h>
#include <y2pm/PkgArch.h>
#include <y2pm/InstSrcManager.h>

#include <stdint.h>
#include <malloc.h>
#include <locale.h>
#include <signal.h>
#include <fnmatch.h>

#include "variable.h"
#include "variables.h"
#include "y2pmsh.h"
#include "helpscreen.h"
#include "solverresults.h"
#include "build.h"
#include "instsrc.h"
#include "search.h"
#include "summary.h"
#include "distrocheck.h"
#include "callbacks.h"

using namespace std;

static bool _keep_running = true;

int install(vector<string>& argv);
int consistent(vector<string>& argv);
int help(vector<string>& argv);
int enablesources(vector<string>& argv);
int show(vector<string>& argv);
int remove(vector<string>& argv);
int rpminstall(vector<string>& argv);
int instlog(vector<string>& argv);
int deselect(vector<string>& argv);
int solve(vector<string>& argv);
int rebuilddb(vector<string>& argv);
int df(vector<string>& argv);
int selshow(vector<string>& argv);
int setsel(vector<string>& argv);
int delsel(vector<string>& argv);
int solvesel(vector<string>& argv);
int setappl(vector<string>& argv);
int order(vector<string>& argv);
int upgrade(vector<string>& argv);
int commit(vector<string>& argv);
int mem(vector<string>& argv);
int checkpackage(vector<string>& argv);
int isc(vector<string>& argv);
int quit(vector<string>& argv);
int why(vector<string>& argv);

int testset(vector<string>& argv);
int testmediaorder(vector<string>& argv);

int cdattach(vector<string>& argv);

int products(vector<string>& argv);

Y2PMSH y2pmsh;

inline int init(vector<string>& argv)
{
    return y2pmsh.init(argv);
}

static ostream& operator<<( ostream& os, const struct mallinfo& i )
{
    os << "Memory from system: " << FSize(i.arena) << ", used: " << FSize(i.uordblks);
    return os;
}

static int solveandcommit(int ret, bool ignoreshellmode = false)
{
    if(ret == 0 && (ignoreshellmode || !y2pmsh.shellmode()))
    {
	vector<string> nothing;
	ret = solve(nothing);

	if(ret == 0)
	    ret = commit(nothing);
    }

    return ret;
}

/** filters */
class CheckSkipSelectable
{
    public:
	virtual ~CheckSkipSelectable() {};
	virtual bool operator() (PMSelectablePtr p)
	{
	    return false;
	}
};

/** check whether package is installed */
class SkipNotInstalled : public CheckSkipSelectable
{
    public:
	virtual ~SkipNotInstalled() {};
	// skip not installed packages
	virtual bool operator() (PMSelectablePtr p)
	{
	    if(p->has_installed())
		return false;
	    else
		return true;
	}
};

/** check whether candidate is available */
class SkipNoCandidates : public CheckSkipSelectable
{
    public:
	virtual ~SkipNoCandidates() {};
	// return true if package should be skipped
	virtual bool operator() (PMSelectablePtr p)
	{
	    if(!p->has_candidate())
		return true;
	    else
		return false;
	}
};

/** tab expansion */
class SelectableCompleter : public CmdLineIface::Completer
{
    private:
	string _name;
	PMManager& _manager;
	CheckSkipSelectable _skip;
    
    protected:
	/** return package skip object */
	virtual CheckSkipSelectable& skipper()
	{
	    return _skip;
	}

    public:
	SelectableCompleter(const string& name, PMManager& manager)
	    : _name(name), _manager(manager) {};
	virtual ~SelectableCompleter() {};
	virtual const std::string command() { return _name; };
	virtual std::vector<std::string> completions(const std::string word)
	{
	    vector<string> v;
	    if(!y2pmsh.initialized())
		return v;

	    PMManager::PMSelectableVec::iterator it;

	    for (it = _manager.begin();
		it != _manager.end(); ++it)
	    {
		if(skipper()(*it))
		    continue;

		// we cannot use fnmatch here because tecla only allows to
		// append stuff to words rather than replacing them completely
		string name = (*it)->name();
		if(name.substr(0, word.length()) == word)
		{
		    v.push_back(name);
		}
	    }

	    return v;
	}
};

class InstalledSelectableCompleter : public SelectableCompleter
{
    private:
	SkipNotInstalled _skip;
    
    protected:
	/** return Selectable skip object */
	virtual CheckSkipSelectable& skipper()
	{
	    return _skip;
	}

    public:
	InstalledSelectableCompleter(const string& name, PMManager& manager) : SelectableCompleter(name, manager) {}
	virtual ~InstalledSelectableCompleter()
	{
	}
};

class CandidateSelectableCompleter : public SelectableCompleter
{
    private:
	SkipNoCandidates _skip;
    
    protected:
	/** return Selectable skip object */
	virtual CheckSkipSelectable& skipper()
	{
	    return _skip;
	}

    public:
	CandidateSelectableCompleter(const string& name, PMManager& manager) : SelectableCompleter(name, manager) {}
	virtual ~CandidateSelectableCompleter()
	{
	}
};

/** wildcard expansion */
class SelectableWordExpander : public Command::WordExpander
{
    public:
	typedef std::vector<std::string> Words;

    private:
	PMManager& _manager;

    protected:
	class MatchPackage
	{
	    private:
		const string& _pattern;
		Words& _words;
		CheckSkipSelectable& _skip;
		bool& _havematch;

	    public:
		MatchPackage(const string& pattern, Words& words, CheckSkipSelectable& skip, bool& havematch)
		    : _pattern(pattern), _words(words), _skip(skip), _havematch(havematch)
		{
		}

		void operator()(PMSelectablePtr p)
		{
		    if(_skip(p))
			return;

		    const char* name = p->name().asString().c_str();

		    if(::fnmatch(_pattern.c_str(), name, 0) == 0)
		    {
			_words.push_back(p->name());
			_havematch = true;
		    }
		}
	};

	class ExpandWord
	{
	    private:
		Words& _words;
		CheckSkipSelectable& _skip;
		PMManager& _manager;
		
	    public:
		ExpandWord(Words& words, CheckSkipSelectable& skip, PMManager& manager)
		    : _words(words), _skip(skip), _manager(manager) {};
		void operator()(const std::string& word)
		{
		    bool havematch = false;
		    for_each(
			_manager.begin(),
			_manager.end(),
			MatchPackage(word, _words, _skip, havematch));

		    if(!havematch)
			_words.push_back(word);
		}
	};

	CheckSkipSelectable _skip;
	/** return package skip object */
	virtual CheckSkipSelectable& skipper()
	{
	    return _skip;
	}

    public:
	SelectableWordExpander(PMManager& manager) : WordExpander(), _manager(manager) {}
	virtual ~SelectableWordExpander() {}
	virtual std::vector<std::string> operator()(const std::vector<std::string>& words)
	{
	    Words out;

	    if(!y2pmsh.initialized())
		return out;

	    if(words.begin() == words.end())
		return words;

	    out.push_back(*words.begin());
	    CheckSkipSelectable& skip = skipper();
	    for_each(words.begin()+1, words.end(), ExpandWord(out, skip, _manager));

	    return out;
	}
};

class CandidateSelectableWordExpander : public SelectableWordExpander
{
    protected:
	SkipNoCandidates _skip;
	virtual CheckSkipSelectable& skipper()
	{
	    return _skip;
	}
    public:
	CandidateSelectableWordExpander(PMManager& manager) : SelectableWordExpander(manager) {}
	virtual ~CandidateSelectableWordExpander() {}
};

class InstalledSelectableWordExpander : public SelectableWordExpander
{
    protected:
	SkipNotInstalled _skip;
	virtual CheckSkipSelectable& skipper()
	{
	    return _skip;
	}
    public:
	InstalledSelectableWordExpander(PMManager& manager) : SelectableWordExpander(manager) {}
	virtual ~InstalledSelectableWordExpander() {}
};


void init_commands()
{
#define newcmd(a,b,c,d) \
    y2pmsh.cmd.add(new Command(a,b,c,d))
#define newpkgcmd(a,b,c,d) \
    newcmd(a,b,c,d);
#define newpkgcmdC(a,b,c,d) \
    newpkgcmd(a,b,c,d); \
    y2pmsh.cli().registerCompleter(new CandidateSelectableCompleter(a, Y2PM::packageManager())); \
    y2pmsh.cmd[a]->Expander(new CandidateSelectableWordExpander(Y2PM::packageManager()));
#define newpkgcmdI(a,b,c,d) \
    newpkgcmd(a,b,c,d); \
    y2pmsh.cli().registerCompleter(new InstalledSelectableCompleter(a, Y2PM::packageManager())); \
    y2pmsh.cmd[a]->Expander(new InstalledSelectableWordExpander(Y2PM::packageManager()));
#define newpkgcmdA(a,b,c,d) \
    newpkgcmd(a,b,c,d); \
    y2pmsh.cli().registerCompleter(new SelectableCompleter(a, Y2PM::packageManager())); \
    y2pmsh.cmd[a]->Expander(new SelectableWordExpander(Y2PM::packageManager()));

#define newselcmd(a,b,c,d) \
    newcmd(a,b,c,d);
#define newselcmdC(a,b,c,d) \
    newselcmd(a,b,c,d); \
    y2pmsh.cli().registerCompleter(new CandidateSelectableCompleter(a, Y2PM::selectionManager())); \
    y2pmsh.cmd[a]->Expander(new CandidateSelectableWordExpander(Y2PM::selectionManager()));
#define newselcmdI(a,b,c,d) \
    newselcmd(a,b,c,d); \
    y2pmsh.cli().registerCompleter(new InstalledSelectableCompleter(a, Y2PM::selectionManager())); \
    y2pmsh.cmd[a]->Expander(new InstalledSelectableWordExpander(Y2PM::selectionManager()));
#define newselcmdA(a,b,c,d) \
    newselcmd(a,b,c,d); \
    y2pmsh.cli().registerCompleter(new SelectableCompleter(a, Y2PM::selectionManager())); \
    y2pmsh.cmd[a]->Expander(new SelectableWordExpander(Y2PM::selectionManager()));

// flags: 1 = need init, 2 = hidden, 4 = advanced
    newpkgcmdC("install",	install, 1, "select packages for installation");
    newpkgcmdC("isc",	isc, 1, "install, solve and commit");
    newpkgcmdA("deselect",	deselect, 1, "deselect packages marked for installation/removal");
    newpkgcmdI("remove",	remove, 1, "select package for removal");
    newcmd("solve",	solve, 1, "solve dependencies");
    newpkgcmdA("state",	state, 1, "show state of package(s)");
    newcmd("newer",	newer, 1, "show packages with newer candiate available");
    newcmd("_rpminstall",	rpminstall, 3, "install rpm files");
    newcmd("_consistent",	consistent, 3, "check consistency");
    newcmd("set",		varset, 0, "set or show variable");
    newcmd("unset",	varunset, 0, "unset variable");
    newpkgcmdA("show",	show, 1, "show package info");
    newcmd("search",	search, 1, "search for packages");
    newcmd("whatprovides",	whatprovides, 1, "search for package provides");
    newcmd("whatrequires",	whatrequires, 1, "search for package requirement");
    newpkgcmdA("whatdependson",	whatdependson, 1, "search for depending packages");
    newpkgcmdA("whatconflictswith", whatconflictswith, 1, "search for conflicting packages");
    newpkgcmdA("allconflicts", allconflicts, 1, "display all conflicting packages");
    newpkgcmdA("depends",	depends, 1, "search for depending packages");
    newpkgcmdA("why",	why, 1, "print solve results for arguments");
    newcmd("alternatives",	alternatives, 5, "search for depending packages");
    newcmd("source",	source, 0, "manage installation sources");
    newcmd("_enablesources", enablesources, 3, "enable all sources");
    newcmd("_deletemediaatexit", _deletemediaatexit, 3, "delete all medias at exit");
    newcmd("buildsolve",	buildsolve, 3, "solve dependencies like the build script");
    newcmd("_rebuilddb",	rebuilddb, 3, "rebuild rpm db");
    newcmd("df",	df, 1, "display disk space forecast");
    newpkgcmdC("_installappl",	setappl, 5, "set package to installed like a selection would do");
    newcmd("_order",	order, 3, "compute installation order");
    newcmd("upgrade",	upgrade, 1, "compute upgrade");
    newcmd("summary",	summary, 1, "display summary about what would be done on commit");
    newcmd("depstats",	depstats, 1, "dependency statistics");
    newcmd("commit",	commit, 1, "commit changes. actually performs installation");
    newselcmdA("selstate",	selstate, 1,  "show state of selection");
    newselcmdA("selshow",	selshow, 1, "show selection info");
    newselcmdC("selinstall",	setsel, 1, "mark selection for installation, need to call solvesel");
    newselcmdI("selremove",	delsel, 1, "mark selection for removal, need to call solvesel");
    newselcmd("selsolve",	solvesel, 1, "solve selection dependencies and apply state to packages");
    newcmd("_cdattach",	cdattach, 2, "cdattach");
    newpkgcmdC("distrocheck", distrocheck, 7, "find unsatisfied dependencies among candidates");
// not useful    newcmd("you", onlineupdate, 1, "yast online update");
    newcmd("mem",	mem, 2, "memory statistics");
    newcmd("_testset",	testset, 2, "test memory consumption of PkgSet");
    newcmd("_testmediaorder", testmediaorder, 3, "test media order");
    newcmd("init",	init, 1, "initialize packagemanager (happens automatically if needed)");
    newcmd("help",	help, 0, "this screen");
    newcmd("products", products, 1, "show installed products");
    newcmd("sourceorder", sourceorder, 1, "set installation order for sources");
    newcmd("_checkpackage", checkpackage, 6, "check rpm file signature");
    newcmd("quit",	quit, 2, "quit");
    newcmd("exit",	quit, 2, "exit");
#undef newselcmdA
#undef newselcmdI
#undef newselcmdC
#undef newselcmd
#undef newpkgcmdA
#undef newpkgcmdI
#undef newpkgcmdC
#undef newpkgcmd
#undef newcmd
}

int quit(vector<string>& argv)
{
    _keep_running = false;
    return 0;
}

int df(vector<string>& argv)
{
    const PkgDuMaster &dum = Y2PM::packageManager().updateDu();
    const set<PkgDuMaster::MountPoint> &mp = dum.mountpoints();

    cout << " Available      Used    Needed      Free  Mountpoint" << endl;
    for(set<PkgDuMaster::MountPoint>::iterator it = mp.begin();
	    it != mp.end(); ++it)
    {
	cout.width(10);
	cout << it->total();
	cout.width(10);
	cout << it->initial_used();
	cout.width(10);
	cout << it->pkg_diff();
	cout.width(10);
	cout << it->pkg_available();
	cout << "  ";
	cout << it->mountpoint() << endl;
    }
    return 0;
}

int rebuilddb(vector<string>& argv)
{
    PMError err = Y2PM::instTarget().bringIntoCleanState();
    if(err)
    {
	return 1;
    }

    return 0;
}

/********************/

int help(vector<string>& argv)
{
    unsigned filtermask = 6;
    if(argv.size()>1)
    {
	if(argv[1]=="all")
	    filtermask = 0;
	else if(argv[1]=="hidden")
	    filtermask = 4;
	else if(argv[1]=="advanced")
	    filtermask = 2;
    }

    unsigned max = 0;

    for(unsigned c = 0; c < 2; ++c)
    {
	for(Commands::const_iterator cit = y2pmsh.cmd.begin();
		cit != y2pmsh.cmd.end(); ++cit)
	{
	    if(cit->second->flags() && cit->second->flags()&filtermask) continue;

	    size_t len = cit->second->name().length();

	    if(c)
	    {
		for(unsigned space = 0; space < max - len; ++space)
		    cout << ' ';

		cout << cit->second->name();
		cout << "  ";
		cout << cit->second->help() << endl;
	    }
	    else
	    {
		if(len > max) max = len;
	    }
	}
    }

    return 0;
}

static int install_internal(PMManager& manager, vector<string>& argv, bool appl=false)
{
    PMManager::PMSelectableVec::iterator begin, end;
    PMManager::PMSelectableVec vec;
    int failed = 0;
    bool installall = false;
    bool force = false;
    bool help = false;

    set<PMSelectablePtr> todel;
    vector<string>::iterator it;

    if(argv.size() == 1)
    {
	help = true;
    }
    else
    {
	for(it = argv.begin()+1;
	    it != argv.end(); ++it)
	{
	    if(!it->empty() && (*it)[0] != '-') break; 
	    if(*it == "--") { ++it; break; }

	    if(*it == "-h" || *it == "--help") help = true;
	    else if(*it == "-a" || *it == "--all") installall = true;
	    else if(*it == "-f" || *it == "--force") force = true;
	    else if((*it)[0] == '-')
	    {
		cout << "invalid parameter " << *it << endl;
		help = true;
		break;
	    }
	}
    }

    if(help)
    {
	HelpScreen h(argv[0]);
	h.additionalparams("PACKAGES");
	h.Parameter(HelpScreenParameter("-a", "--all", "install/update all"));
	h.Parameter(HelpScreenParameter("-f", "--force", "also perform downgrades and reinstallations"));
	cout << h;
	return 0;
    }

    if(installall)
    {
	begin = manager.begin();
	end = manager.end();
    }
    else
    {
	for (; it != argv.end(); ++it)
	{
	    bool remove = false;
	    string pkg = stringutil::trim(*it);

	    if(pkg.empty()) continue;
	
	    if(pkg[0] == '-')
	    {
		remove = true;
		pkg = pkg.substr(1);

		if(pkg.empty()) continue;
	    }

	    PMSelectablePtr selp;
	    PkgEdition ed;

	    selp = manager.getItem(pkg);

	    if(!selp)
	    {
		PkgNameEd ned = PkgNameEd::fromString(pkg);
		selp = manager.getItem(ned.name);
		ed = ned.edition;
	    }

	    if(!selp || !selp->has_candidate())
	    {
		failed = 1;
		std::cout << "package " << pkg << " is not available." << endl;
		continue;
	    }

	    if(!ed.is_unspecified())
	    {
		bool gotmatch = false;
		PMObjectPtr op = selp->candidateObj();
		if(!op) { std::cout << "OOPS: got NULL for " << pkg << endl; continue; }
		if(op->edition() == ed)
		{
		    gotmatch = true;
		}
		else
		{
		    PMSelectable::PMObjectList::const_iterator it;
		    for(it = selp->av_begin(); it != selp->av_end(); ++it)
		    {
			op = *it;
			if(op->edition() == ed )
			{
			    if(selp->setUserCandidate(op))
			    {
				gotmatch = true;
				break;
			    }
			    else
			    {
				cout << "could not set " << op->nameEd() << endl;
			    }
			}
		    }
		}

		if(!gotmatch)
		{
		    cout << pkg << " not available in version " << ed << endl;
		    continue;
		}
	    }
	    
	    vec.insert(selp);
	    if(remove)
		todel.insert(selp);
	}
	begin=vec.begin();
	end=vec.end();
    }

    for(PMManager::PMSelectableVec::iterator it=begin; it!=end;++it)
    {
	bool ok;

	if(todel.find(*it) != todel.end())
	{
	    if(appl)
		ok = (*it)->appl_set_offSystem();
	    else
		ok = (*it)->user_set_offSystem();
	}
	else
	{
	    if(!(*it)->has_candidate())
		continue;

	    if(!force && !(*it)->to_delete() && (*it)->downgrade_condition())
	    {
		if(!installall)
		    cout << "not downgrading " << (*it)->name() << endl;
		continue;
	    }

	    if(appl)
		ok = (*it)->appl_set_install();
	    else
		ok = (*it)->user_set_install();
	}

	if(!ok)
	{
	    cout << stringutil::form("could not mark %s for installation/update",
		    (*it)->theObject()->name()->c_str());
	    failed = 2;
	}
    }

    PrintSelectable::Flags flags;
    flags.summary=true;
    for_each(begin,end,PrintSelectable(cout, flags));

    return failed;
}

int isc(vector<string>& argv)
{
    int ret = install(argv);
    if(!ret)
	return solveandcommit(ret, true);

    return ret;
}

int install(vector<string>& argv)
{
    int ret = install_internal(Y2PM::packageManager(), argv);

    return solveandcommit(ret);
}

int setappl(vector<string>& argv)
{
    return install_internal(Y2PM::packageManager(), argv, true);
}

int setsel(vector<string>& argv)
{
    return install_internal(Y2PM::selectionManager(), argv);
}

static int remove_internal(vector<string>& argv, PMManager& manager)
{
    int failed = 0;
    for (unsigned i=1; i < argv.size() ; i++)
    {
	string name = stringutil::trim(argv[i]);

	if(name.empty()) continue;

	PMSelectablePtr selp = manager.getItem(name);
	if(!selp)
	{
	    std::cout << name << " is not available.\n";
	    failed = 1;
	    continue;
	}

	if(!selp->user_set_offSystem())
	{
	    ++failed;
	    cout << stringutil::form("coult not mark %s for deletion", name.c_str()) << endl;
	}
    }
    return failed;
}

int remove(vector<string>& argv)
{
    int ret = remove_internal(argv, Y2PM::packageManager());

    return solveandcommit(ret);
}

int delsel(vector<string>& argv)
{
    return remove_internal(argv, Y2PM::selectionManager());
}

static PkgDep::ResultList solve_good;
static PkgDep::ErrorResultList solve_bad;

static bool solve_internal(PMManager& manager, vector<string>& argv)
{
    bool success = false;
    bool filter = false;

    for(vector<string>::iterator it= argv.begin(); it != argv.end();++it)
    {
	if(*it == "-u")
	{
	    filter = true;
	    cout << "filtering" << endl;
	}
    }

    solve_good.clear();
    solve_bad.clear();

    success = manager.solveInstall(solve_good, solve_bad, filter);

    return success;
}

int solve(vector<string>& argv)
{
    bool ok = solve_internal(Y2PM::packageManager(), argv);

    printgoodlist(solve_good);
    unsigned numbad = printbadlist(solve_bad);

    cout << endl;
    cout << "Packages with errors: " << numbad  << endl;

    vector<string> dummy;
    summary(dummy);

    return ok?0:1;
}

int solvesel(vector<string>& argv)
{
    bool ok = solve_internal(Y2PM::selectionManager(), argv);

    unsigned numinst = printgoodlist(solve_good);
    unsigned numbad = printbadlist(solve_bad);

    cout << endl;
    cout << numbad << " bad, " << numinst << " to install" << endl;

    if(ok)
    {
	cout << "\nsolve ok, activating selection" << endl;
	Y2PM::selectionManager().activate();
    }
    return ok?0:1;
}

int why(vector<string>& argv)
{
    set<string> want;
    int ret = 1;

    for(vector<string>::iterator it= argv.begin()+1; it != argv.end();++it)
    {
	want.insert(*it);
    }

    for(PkgDep::ResultList::const_iterator p=solve_good.begin();p!=solve_good.end();++p)
    {
	if(want.find(p->name) != want.end())
	{
	    cout << *p << endl;
	    ret = 0;
	}
    }
    for( PkgDep::ErrorResultList::const_iterator bp = solve_bad.begin();
	 bp != solve_bad.end(); ++bp )
    {
	if(want.find(bp->name) != want.end())
	{
	    cout << *bp << endl;
	    ret = 0;
	}
    }

    return ret;
}

int order(vector<string>& argv)
{
    PkgSet toinstall;
    PkgSet installed;
    for (PMManager::PMSelectableVec::iterator it = Y2PM::packageManager().begin();
	it != Y2PM::packageManager().end(); ++it )
    {
	if((*it)->has_installed())
	{
	    installed.add((*it)->installedObj());
	}
	if((*it)->to_install())
	{
	    toinstall.add((*it)->candidateObj());
	}
    }

    InstallOrder order(toinstall,installed);

    InstallOrder::SolvableList nowinstall;

    unsigned count=0;

    cout << "Installation order:" << endl;
    for(nowinstall = order.computeNextSet(); !nowinstall.empty(); nowinstall = order.computeNextSet())
    {
	count++;
	for(InstallOrder::SolvableList::const_iterator cit = nowinstall.begin();
	    cit != nowinstall.end(); ++cit)
	{
	    cout << (*cit)->name() << " ";
	}
	cout << endl;
	{
	    string filename=stringutil::form("/tmp/graph.run%d",count);
	    ofstream fs(filename.c_str());
	    if(fs)
	    {
		order.printAdj(fs,false);
		fs.close();
	    }
	    else
	    {
		cout << "unable to open " << filename << endl;
	    }
	}
	{
	    string filename=stringutil::form("/tmp/rgraph.run%d",count);
	    ofstream fs(filename.c_str());
	    if(fs)
	    {
		order.printAdj(fs,true);
		fs.close();
	    }
	    else
	    {
		cout << "unable to open " << filename << endl;
	    }
	}
	order.setInstalled(nowinstall);
    }

    return 0;
}

int upgrade(vector<string>& argv)
{
    PMUpdateStats stats;
    stats.delete_unmaintained = false;

    for(vector<string>::iterator it= argv.begin(); it != argv.end();++it)
    {
	if(*it == "-u")
	{
	    stats.delete_unmaintained = true;
	    cout << "delete unmaintained" << endl;
	}
    }

    Y2PM::packageManager().doUpdate(stats);
    cout << stats << endl;
    return 0;
}

class Print
{
    private:
	ostream& _os;
	const char* _form;
    public:
	Print(ostream& os, const char* form)
	    : _os(os), _form(form)
	{
	}
	void operator()(const string& str)
	{
	    _os << stringutil::form(_form, str.c_str());
	}
};

// TODO: need this as interface in pkgmanager
class CountToInstall
{
	int _toinst;
	int _todel;
    public:
	CountToInstall() : _toinst(0), _todel(0)
	{
	}
	~CountToInstall()
	{
	    y2pmshCB::toinstallcounter.init(0, _toinst, 0);
	    y2pmshCB::todeletecounter.init(0, _todel, 0);
	}
	void operator()(PMSelectablePtr p)
	{
	    if(!p) return;

	    if(p->to_install())
		++_toinst;
	    else if(p->to_delete())
		++_todel;
	}
};

int commit(vector<string>& argv)
{
    std::list<std::string> errors_r;
    std::list<std::string> remaining_r;
    std::list<std::string> srcremaining_r;

    Pathname etc(variables["root"].getString());
    etc+="/etc";
    PathInfo::assert_dir(etc);

#if 0
    PathInfo passwd(etc+"passwd");
    if(!passwd.isExist())
    {
	ofstream os;
	os.open(passwd.asString().c_str());
	os << "root:x:0:0:root:/root:/bin/bash" << endl;
	os.close();
    }

    passwd = etc+"group";
    if(!passwd.isExist())
    {
	ofstream os;
	os.open(passwd.asString().c_str());
	os << "root:x:0:root" << endl;
	os.close();
    }
#endif

    for_each(
	Y2PM::packageManager().begin(),
	Y2PM::packageManager().end(),
	CountToInstall());

    int failed = 0;
    int num = Y2PM::commitPackages (0,errors_r, remaining_r, srcremaining_r);

    if(num < 0) // XXX
    {
	failed = 1;
	num += 100000;
    }

    cout << num << " packages installed" << endl;

    if(!errors_r.empty())
    {
	failed = 1;
	cout << "failed packages:" << endl;
	for_each(errors_r.begin(), errors_r.end(), Print(cout, "  %s\n"));
    }

    if(!remaining_r.empty())
    {
	failed = 1;
	cout << "remaining packages:" << endl;
	for_each(remaining_r.begin(), remaining_r.end(), Print(cout, " %s"));
	cout << "\n" << endl;
    }

    if(variables["quitoncommit"].getBool())
    {
	_keep_running = false;
    }
    else
    {
	PMError err = Y2PM::instTargetUpdate();
	if(err)
	{
	    cout << "Error reinitializing target: " << err << endl;
	    cout << "!!! System is in an undefined state now, please quit !!!" << endl;
	}
    }

    return failed;
}

int testmediaorder(vector<string>& argv)
{
    std::list<PMPackagePtr> dellist;
    std::list<PMPackagePtr> inslist;
    std::list<PMPackagePtr> srclist;

    Y2PM::packageManager().getPackagesToInsDel (dellist, inslist, srclist);

    bool quiet = false;
    unsigned int current_src_media = 0;
    constInstSrcPtr current_src_ptr = 0;

    if(argv.size() > 1 && argv[1] == "-q")
    {
	quiet = true;
    }

    for (std::list<PMPackagePtr>::iterator it = inslist.begin();
	it != inslist.end(); ++it)
    {
	unsigned int pkgmedianr = 0;
	pkgmedianr = (*it)->medianr();

	if (((*it)->source() != current_src_ptr)	// source or media change
	    || (pkgmedianr != current_src_media))
	{
	    constInstSrcDescrPtr descr = (*it)->source()->descr();
	    if(current_src_ptr!=0)
		cout << endl;
	    cout << "Installing from " << descr->content_label()
		<< " (" << descr->url() << ")"
		<< " Media Nr. " << pkgmedianr << ":";

	    current_src_ptr = (*it)->source();
	    current_src_media = pkgmedianr;

	}

	if(!quiet)
	    cout << ' ' << (*it)->name();
    }
    cout << endl;

    return 0;
}

int consistent(vector<string>& argv)
{
    int numbad=0;
    bool success = false;

    PkgDep::ErrorResultList bad;

    success = Y2PM::packageManager().solveConsistent(bad);

    numbad = printbadlist(bad);

    cout << "***" << endl;
    cout << numbad << " bad" << endl;

    return success?0:1;
}

int deselect(vector<string>& argv)
{
    int failed = 0;
    for (unsigned i=1; i < argv.size() ; i++)
    {
	string pkg = stringutil::trim(argv[i]);

	if(pkg.empty()) continue;

	PMSelectablePtr selp = Y2PM::packageManager().getItem(pkg);
	if(!selp)
	{
	    std::cout << "package " << pkg << " is not available.\n";
	    failed = 1;
	    continue;
	}

	selp->setNothingSelected();
    }

    return failed;
}

int taboo(vector<string>& argv)
{
    int failed = 0;
    for (unsigned i=1; i < argv.size() ; i++)
    {
	string pkg = stringutil::trim(argv[i]);

	if(pkg.empty()) continue;

	PMSelectablePtr selp = Y2PM::packageManager().getItem(pkg);
	if(!selp)
	{
	    std::cout << "package " << pkg << " is not available.\n";
	    failed = 1;
	    continue;
	}

	selp->setNothingSelected();
    }

    return failed;
}

int rpminstall(vector<string>& argv)
{
    vector<string>::iterator it=argv.begin();
    list<Pathname> pkgs;
    ++it; // first one is function name itself

    for(;it!=argv.end();++it)
    {
	pkgs.push_back (Pathname (*it));
    }

    if(Y2PM::instTargetInit(variables["root"].getString()) != InstTargetError::E_ok)
    {
	cout << "initialization failed" << endl;
	return 1;
    }
    else
	Y2PM::instTarget().installPackages(pkgs);

    return 0;
}

int mem(vector<string>& argv)
{
    cout << mallinfo() << endl;
    return 0;
}

int cdattach(vector<string>& argv)
{
    MediaAccessPtr media = new MediaAccess;
    string dev;

    if(argv.size() > 1)
    {
	dev = argv[1];
    }

    Url mediaurl_r = (string("cd:///;") + dev);


    PMError err;
    if ( (err = media->open( mediaurl_r, "/tmp/blub" )) )
    {
	ERR << "Failed to open " << mediaurl_r << " " << err << endl;
	return 1;
    }

    DBG << "open ok" << endl;
    bool repeat = true;
    bool notfirst = false;
    do
    {
	if(media->isAttached())
	{
	    cout << "release medium " << media->release() << endl;
	}
	cout << "try attach medium " << endl;
	if ( (err = media->attach(notfirst)) )
	{
	    cout << "Failed to attach media: " << err << endl;
	    repeat = false;
	    return 1;
	}
	else
	    cout << "attach ok" << endl;
	notfirst = true;
    } while (repeat);

    if(media->isAttached())
    {
	DBG << "release medium final" << endl;
	media->release();
    }

    return 0;
}

int products( vector<string>& argv )
{
    int remove = -1;
    vector<string>::iterator arg = argv.begin()+1;
    for(;arg != argv.end() && (*arg)[0] == '-'; ++arg)
    {
	if(*arg == "--")
	{
	    ++arg;
	    break;
	}
	else if(*arg == "-h" || *arg == "--help")
	{
	    HelpScreen h(argv[0]);
	    h.Parameter(HelpScreenParameter("-R ID", "--remove ID", "remove product number ID"));
	    cout << h;
	    return 0;
	}
	else if(*arg == "-R" || *arg == "--remove")
	{
	    if (++arg == argv.end())
	    {
		cout << "must specify number" << endl;
		return 1;
	    }
	    remove = atoi((*arg).c_str());
	}
    }

    const std::list<constInstSrcDescrPtr> &products = Y2PM::instTarget().getProducts();

    int i = 0;    
    std::list<constInstSrcDescrPtr>::const_iterator it;
    for( it = products.begin(); it != products.end(); ++it, ++i )
    {
	if(remove != -1)
	{
	    if(i != remove) continue;
	    PMError err = Y2PM::instTarget().removeProduct(*it);
	    if(err)
	    {
		cout << err << endl;
		return 1;
	    }
	    return 0;
	}
	else
	{
	    cout << i << ": " << (*it)->content_product();
	    std::string youurl = (*it)->content_youurl();
	    if ( !youurl.empty() ) cout << ", YouUrl: " << youurl;
	    std::string youpath = (*it)->content_youpath();
	    if ( !youpath.empty() ) cout << ", YouPath: " << youpath;
	    std::string youtype = (*it)->content_youtype();
	    if ( !youtype.empty() ) cout << ", YouType: \"" << youtype << "\"";
	    cout << endl;
	}
    }

    return 0;
}

void y2pmshquitsignalhandler(int sig)
{
    y2pmsh.quit();
    exit(sig);
}

void installsignalhandlers(void)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = y2pmshquitsignalhandler;
    ::sigaction(SIGINT, &act, NULL);
    ::sigaction(SIGHUP, &act, NULL);
}

int main( int argc, char *argv[] )
{
    bool noinit = false;
    bool cliok = true;

    int mainret = 0;
    
    if(argc > 1 && string(argv[1]) == "--no-init")
    {
	noinit = true;
	++argv;
	--argc;
    }

    if(argc > 1)
	y2pmsh.shellmode(false);


    init_variables();
    
    init_commands();

    y2pmsh.setenv();

    setlocale(LC_CTYPE, "");

    cout << "Welcome to the YaST2 Package Manager!" << endl;
    cout << "This tool is meant for debugging purpose only." << endl << endl;

    if(y2pmsh.shellmode() && y2pmsh.interactive())
    {
	if(!noinit)
	{
	    vector<string> args;
	    args.push_back("init");
	    mainret = y2pmsh.init(args);

	    if(!mainret)
	    {
		args[0] = "distrocheck";
		mainret = distrocheck(args);
	    }
	}

	cout << endl << "type help for help, ^D to exit" << endl << endl;
    }

    installsignalhandlers();

    while( cliok && _keep_running)
    {
	vector<vector<string> > cmds;

	if(y2pmsh.shellmode())
	{
	    string inputstr;

	    y2pmsh.cli().lastRetCode(mainret);

	    cliok = y2pmsh.cli().getLine(inputstr);

	    if(!cliok) break;

	    inputstr = stringutil::trim(inputstr);

	    if(inputstr.empty())
		continue;

	    y2pmsh.cli().addToHistory(inputstr);

	    vector<string> tmp;
	    y2pmsh.cli().parsewords(inputstr, tmp);
	    cmds.push_back(tmp);
	}
	else
	{
	    vector<string> tmp;
	    for(int i = 1; i < argc; ++i)
	    {
		tmp.push_back(argv[i]);
	    }
	    cmds.push_back(tmp);
	    cliok = false;

//	    Variable::assign(variables["quitoncommit"], "true");
	}

	for(vector<vector<string> >::iterator vit = cmds.begin();
		vit != cmds.end() && _keep_running; ++vit)
	{
	    vector<string> argv = *vit;
	    Command* cmdptr = NULL;

	    cmdptr = y2pmsh.cmd[argv[0]];

	    if(cmdptr)
	    {
		TimeClass t;
		bool showtimes = variables["timestat"].getBool();
		if((cmdptr->flags()&1) && !y2pmsh.initialized() && cmdptr->func() != init)
		{
		    vector<string> args;
		    if(showtimes) t.startTimer();
		    mainret = y2pmsh.init(args);
		    if(showtimes) t.stopTimer();
		    if(showtimes) cout << "time: " << t << endl;
		}

		if(cmdptr->hasExpander())
		{
		    argv = cmdptr->Expander()(argv);
		}

//		for_each(argv.begin(), argv.end(), Print(cout, "'%s' "));
//		cout << endl;

		if(showtimes) t.startTimer();
		mainret = cmdptr->run(argv);
		if(showtimes) t.stopTimer();
		if(showtimes) cout << "time: " << t << endl;
		if(mainret)
		{
		    if(!y2pmsh.shellmode()) // switch to interactive on error
		    {
			y2pmsh.shellmode(true);
			cliok = true;
		    }
		    else if(variables["quitonfail"].getBool())
		    {
			_keep_running = false;
		    }
		}
	    }
	    else
	    {
		cout << "unknown function " << argv[0] << endl;
		mainret = 1;
	    }
	}
    }

    cout << endl;

    if(y2pmsh.shellmode())
	y2pmsh.cli().saveHistory();

    y2pmsh.quit();

    return mainret;
}

int testset(vector<string>& argv)
{
    int i = 0;
    cout << mallinfo() << endl;
    PkgSet* testset = new PkgSet;
    cout << mallinfo() << endl;

    if(!testset)
    {
	ERR << "out of memory" << endl;
	exit(EXIT_FAILURE);
    }

    PMManager::PMSelectableVec::const_iterator it, end;
    it = Y2PM::packageManager().begin();
    end = Y2PM::packageManager().end();
    for (;it!=end;++it)
    {
	PMSelectablePtr selp = *it;
	if(!selp)
	{
	    std::cerr << "invalid pointer" << endl;
	    continue;
	}

	PMSolvablePtr solp;
	if(selp->has_installed())
	{
	    solp = selp->installedObj();
	}
	else if(selp->has_candidate())
	{
	    solp = selp->candidateObj();
	}
	if(!solp)
	{
	    std::cout << "no solvable" << endl;
	    continue;
	}

	testset->add(solp);
	if(i<5)
	{
	    cout << mallinfo() << endl;
	}
	++i;
    }

    cout << mallinfo() << endl;

    delete testset;
    testset = NULL;

    cout << mallinfo() << endl;
    return 0;
}


int checkpackage(vector<string>& argv)
{
    if(argv.size() < 2)
    {
	cout << "need arg" << endl;
	return 1;
    }

    string file = argv[1];

#if 0
    cout << Y2PM::instTarget().checkPackage(file) << endl;
#else
    RpmDb rpm;

    unsigned ret = rpm.checkPackage(file);

    cout << rpm.checkPackageResult2string(ret) << endl;
#endif

    return 0;
}

// vim:sw=4
