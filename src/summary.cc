#include <fstream>
#include <algorithm>

#include <y2util/Y2SLog.h>

#include <Y2PM.h>
#include <y2pm/PMPackageManager.h>
#include <y2pm/PMSelectionManager.h>

#include "y2pmsh.h"
#include "summary.h"
#include "helpscreen.h"

using namespace std;

static const char* statestr[] = { "@i", "--", " -", " >", " +", "a-", "a>", "a+", " i", "  " };

PrintSelectable::Flags::Flags()
    : onlyinstalled(false), onlychanged(false), summary(false), version(true)
{
}

PrintSelectable::PrintSelectable(std::ostream& os, PrintSelectable::Flags flags) : _os(os), _flags(flags)
{
}

void PrintSelectable::operator()(PMSelectablePtr sptr)
{
    PMSelectable::UI_Status s = sptr->status();

    if(_flags.onlyinstalled && !sptr->has_installed())
	return;
    
    switch(s)
    {
	case PMSelectable::S_Protected:
	case PMSelectable::S_Taboo:
	case PMSelectable::S_Del:
	case PMSelectable::S_Update:
	case PMSelectable::S_Install:
	case PMSelectable::S_AutoDel:
	case PMSelectable::S_AutoUpdate:
	case PMSelectable::S_AutoInstall:
	case PMSelectable::S_KeepInstalled:
	case PMSelectable::S_NoInst:

	if(_flags.onlychanged && !sptr->to_modify())
	    return;

	_os << statestr[s] << "   " << sptr->name();

	PMSelectionPtr selp=sptr->theObject();
	if(selp)
	{
	    _os << "  ";
	    if(selp->visible())
		_os << " +";
	    _os << selp->category();
	}
	else
	{
	    if(_flags.version)
	    {
		_os << " (";
		if(sptr->has_installed())
		{
		    _os << sptr->installedObj()->edition();

		    if(sptr->to_install())
		    {
			if(sptr->downgrade_condition())
			{
			    _os << " -< ";
			}
			else
			{
			    _os << " -> ";
			}
		    }
		    else if(sptr->has_candidate())
		    {
			_os << " / ";
		    }
		}
		if(sptr->has_candidate())
		{
		    if(sptr->downgrade_condition())
			_os << "_";
		    _os << sptr->candidateObj()->edition();
		}
		_os << ')';
	    }
	}

	if(_flags.summary)
	{
	    _os << " - " << sptr->theObject()->summary();
	}
	
	_os << endl;
    }
}

class SetTaboo
{
    private:
	bool _reset;
    public:
	SetTaboo(bool reset) : _reset(reset) {}
	void operator()(PMSelectablePtr p)
	{
	    if(_reset)
	    {
		if(!p->is_taboo() || !p->user_clr_taboo())
		{
		    cout << "can't set package " << p->name() << " to taboo" << endl;
		}
	    }
	    else
	    {
		if(!p->user_set_taboo())
		{
		    cout << "can't set package " << p->name() << " to taboo" << endl;
		}
	    }
	}
};

static int showstate_internal(PMManager& manager, vector<string>& argv)
{
    bool showall = true;
    
    PrintSelectable::Flags flags;
    flags.version = true;
    flags.onlyinstalled = true;

    enum statechange { NONE, SET, RESET } taboo = NONE;

    if(argv.size() > 1 && argv[1] == "--help")
    {
	    HelpScreen h(argv[0]);
	    h.Parameter(HelpScreenParameter("-a", "--all", "show all objects"));
	    h.Parameter(HelpScreenParameter("-c", "--changed", "show only objects with changes"));
	    h.Parameter(HelpScreenParameter("-t", "--taboo", "set package to taboo/protected"));
	    h.Parameter(HelpScreenParameter("-T", "--untaboo", "reset taboo/protected state"));
	    cout << h;
	    return 0;
    }

    PMManager::PMSelectableVec selectables;
    PMManager::PMSelectableVec::const_iterator begin, end;
    if(argv.size()>1)
    {
	for (unsigned i=1; i < argv.size() ; i++) {
	    string pkg = stringutil::trim(argv[i]);

	    if(pkg.empty()) continue;

	    if(pkg == "-a" || pkg == "--all")
	    {
		flags.onlyinstalled = false;
		continue;
	    }
	    else if(pkg == "-c" || pkg == "--changed")
	    {
		flags.onlychanged = true;
		continue;
	    }
	    else if(pkg == "-t" || pkg == "--taboo")
		{ taboo = SET; continue; }
	    else if(pkg == "-T" || pkg == "--untaboo")
		{ taboo = RESET; continue; }

	    PMSelectablePtr selp = manager.getItem(pkg);
	    if(!selp)
	    {
		std::cout << "package " << pkg << " is not available.\n";
		showall = false;
	    }
	    else
	    {
		flags.onlyinstalled = false;
		showall = false;
		selectables.insert(selp);
	    }
	}
	begin = selectables.begin();
	end = selectables.end();
    }

    if(showall)
    {
	begin = manager.begin();
	end = manager.end();
    }

    if( taboo != NONE)
    {
	for_each(selectables.begin(), selectables.end(), SetTaboo( taboo == RESET ));
	return 0;
    }

    for_each(begin, end, PrintSelectable(cout, flags));
    
    return 0;
}

int state(vector<string>& argv)
{
    return showstate_internal(Y2PM::packageManager(),argv);
}

int selstate(vector<string>& argv)
{
    return showstate_internal(Y2PM::selectionManager(),argv);
}

int summary(vector<string>& argv)
{
    PMManager& manager = Y2PM::packageManager();
    PMManager::PMSelectableVec::const_iterator it, end;

    FSize sizeinstall;
    FSize sizefetch;
    unsigned numinstall = 0;
    unsigned numdelete = 0;

    it = manager.begin();
    end = manager.end();
    for (;it!=end;++it)
    {
	PMSelectablePtr sel(*it);
	if(!sel) continue;

	if(sel->to_install())
	{
	    PMPackagePtr pkg = sel->candidateObj();
	    if(!pkg) { INT << "pkg NULL" << endl; continue; }

	    ++numinstall;
	    sizeinstall+=pkg->size();
	    sizefetch+=pkg->archivesize();
	}
	if(sel->to_delete())
	{
	    PMPackagePtr pkg = sel->installedObj();
	    if(!pkg) { INT << "pkg NULL" << endl; continue; }

	    ++numdelete;
	    sizeinstall-=pkg->size();
	}
    }

    cout << "Packages to install: " << numinstall << endl;
    cout << "Packages to delete:  " << numdelete << endl;
    cout << "Download size:       " << sizefetch << endl;
    cout << "Needed Space:        " << sizeinstall << endl;

    return 0;
}

int depstats(vector<string>& argv)
{
    bool installed = false;
    if(argv.size() > 1)
    {
	if(argv[1] == "--help")
	{
	    HelpScreen h(argv[0]);
	    h.Parameter(HelpScreenParameter("-i", "", "count installed instead of candidates"));
	    cout << h;
	    return 0;
	}
	if(argv[1] == "-i")
	    installed = true;
    }


    PMManager& manager = Y2PM::packageManager();
    PMManager::PMSelectableVec::const_iterator it, end;

    unsigned packages = 0;
    unsigned provs = 0;
    unsigned reqs = 0;

    unsigned maxprovs = 0;
    unsigned maxreqs = 0;
    PkgName topprov;
    PkgName topreq;

    it = manager.begin();
    end = manager.end();
    for (;it!=end;++it)
    {
	PMSelectablePtr sel(*it);
	if(!sel) continue;

	PMPackagePtr pkg;
	if(installed)
	    pkg = sel->installedObj();
	else
	    pkg = sel->candidateObj();

	if(!pkg) continue;

	++packages;
	unsigned prov = pkg->provides().size();
	unsigned req = pkg->requires().size();
	provs += prov;
	reqs += req;

	if(prov > maxprovs)
	{
		maxprovs = prov;
		topprov = pkg->name();
	}
	if(req > maxreqs)
	{
		maxreqs = req;
		topreq = pkg->name();
	}

    }

    cout << "Packages: " << packages << endl;
    cout << stringutil::form("Provides: %6d / %6.2f avg, top: %s (%d provides)", provs, (double)provs/packages, topprov->c_str(), maxprovs) << endl;
    cout << stringutil::form("Requires: %6d / %6.2f avg, top: %s (%d requires)", reqs, (double)reqs/packages, topreq->c_str(), maxreqs) << endl;

    return 0;
}

