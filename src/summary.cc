#include <fstream>

#include <y2util/Y2SLog.h>

#include <Y2PM.h>
#include <y2pm/PMPackageManager.h>
#include <y2pm/PMSelectionManager.h>

#include "y2pmsh.h"
#include "summary.h"
#include "helpscreen.h"

using namespace std;

static const char* statestr[] = { "@i", "--", " -", " >", " +", "a-", "a>", "a+", " i", "  " };

static int showstate_internal(PMManager& manager, vector<string>& argv)
{
    bool nonone = true;
    bool changed = false;
    bool showall = true;

    if(argv.size() > 1 && argv[1] == "--help")
    {
	    HelpScreen h(argv[0]);
	    h.Parameter(HelpScreenParameter("-a", "--all", "show all objects"));
	    h.Parameter(HelpScreenParameter("-c", "--canged", "show only objects with changes"));
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
		nonone = false;
		continue;
	    }
	    else if(pkg == "-c" || pkg == "--changed")
	    {
		changed = true;
		continue;
	    }

	    PMSelectablePtr selp = manager.getItem(pkg);
	    if(!selp)
	    {
		std::cout << "package " << pkg << " is not available.\n";
		showall = false;
	    }
	    else
	    {
		nonone = false;
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

    for(PMManager::PMSelectableVec::const_iterator cit = begin;
	cit != end; ++cit)
    {
	PMSelectable::UI_Status s = (*cit)->status();

	if(nonone && s == PMSelectable::S_NoInst)
	    continue;
	
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

	    if(changed && !(*cit)->to_modify())
		continue;

	    cout << statestr[s] << "   " << (*cit)->name();

	    PMSelectionPtr selp=(*cit)->theObject();
	    if(selp)
	    {
		cout << '\t';
		if(selp->visible())
		    cout << " +";
		cout << selp->category();
	    }
	    else
	    {
		if((*cit)->has_installed())
		{
		    cout << " (" << (*cit)->installedObj()->edition();
		    if((*cit)->has_candidate())
		    {
			if((*cit)->to_install())
			    cout << " -> ";
			else
			    cout << " / ";

			cout << (*cit)->candidateObj()->edition();
			if(!(*cit)->downgrade_condition())
			{
			    cout << '*';
			}
		    }
		    cout << ')';
		}
	    }
	    cout << endl;
	}
    }
    
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


