#include <fstream>

#include <y2util/Y2SLog.h>

#include <Y2PM.h>
#include <y2pm/PMPackageManager.h>
#include <y2pm/PkgSet.h>

#include "y2pmsh.h"
#include "build.h"
#include "solverresults.h"

using namespace std;

static void fillavailable(PkgSet& available)
{
    PMManager::PMSelectableVec::const_iterator begin, end;

    begin=Y2PM::packageManager().begin();
    end=Y2PM::packageManager().end();


    for ( PMManager::PMSelectableVec::iterator it = begin; it != end; ++it )
    {
	// remove auto state
	if((*it)->by_auto())
	{
	    (*it)->auto_unset();
	}

	if((*it)->has_candidate())
	{
	    PMSolvablePtr sp = (*it)->candidateObj();
	    if(sp != NULL)
	    {
		available.add(sp);
	    }
	}
    }
}

static void build_printstringset(string name, set<string> s)
{
    if(s.empty()) return;
    cout << name << ":";
    for(set<string>::const_iterator cit=s.begin();
	    cit != s.end(); ++cit)
    {
	cout << " ";
	cout << *cit;
    }
    cout << endl;
}

static void filltoinstall(PkgSet& toinstall, vector<string>& argv)
{
    unsigned nt = 0;
    for (vector<string>::iterator it = ++argv.begin();
	    it != argv.end(); ++it)
    {
	string pkg = stringutil::trim(*it);

	if(pkg.empty()) continue;

	PMSelectablePtr selp = Y2PM::packageManager().getItem(pkg);
	if(!selp || !selp->has_candidate())
	{
	    std::cout << "\033[31mpackage " << pkg << " is not available.\033[m\n";
	    continue;
	}
	toinstall.add(selp->candidateObj());
	++nt;
    }
    cout << nt << " packages to install" << endl;
}

// pretend an empty buildroot and try to install all packages specified on
// command line
static bool runsolver(vector<string>& argv, PkgDep::ResultList& good, PkgDep::ErrorResultList& bad)
{
    PkgSet installed; // already installed
    PkgSet available; // available for installation
    PkgSet toinstall; // user selected for installion

    PkgDep::ErrorResultList obsolete;

    // add all packages in argv into toinstall, provided that the
    // PMPackageManager knows them
    filltoinstall(toinstall, argv);

    // add all packages with candidates to available
    fillavailable(available);

    PkgDep engine( installed, available ); // TODO alternative_default

    engine.set_alternatives_mode(PkgDep::AUTO_IF_NO_DEFAULT);

    bool ret = engine.solvesystemnoauto( toinstall, good, bad, obsolete);

    return bad.empty() && ret;
}

static void computechanges(
    set<string>& need,
    set<string>& keep,
    set<string>& install,
    set<string>& update,
    set<string>& remove,
    bool apply)
{
    PMManager::PMSelectableVec::const_iterator begin, end;

    begin=Y2PM::packageManager().begin();
    end=Y2PM::packageManager().end();


    for(PMManager::PMSelectableVec::const_iterator cit = begin;
	cit != end; ++cit)
    {
	PMSelectablePtr selp = *cit;
	if(!selp)
	{
	    ERR << "invalid selectable pointer" << endl;
	    continue;
	}

	// we know what we are doing.
	selp->user_clr_taboo();

	bool needit = (need.find(selp->name()) != need.end());
	if(needit)
	{
	    if(selp->has_installed())
	    {
		if(selp->candidateObj()->edition() == selp->installedObj()->edition())
		{
		    if(apply) selp->appl_unset();
		    keep.insert(selp->name());
		}
		else
		{
		    if(apply) selp->appl_force_install();
		    update.insert(selp->name());
		}
	    }
	    else
	    {
		if(apply) selp->appl_force_install();
		install.insert(selp->name());
	    }
	}
	else
	{
	    if(selp->has_installed())
	    {
		if(apply)
		{
	    	    if(!selp->appl_set_delete())
		    {
			cerr << "couldnt delete " << selp->name() << endl;
		    }
		}
		remove.insert(selp->name());
	    }
	}
    }
}

int buildsolve(vector<string>& argv)
{
    int numbad=0;
    bool success = false;

    set<string> need, keep, install, remove, update;

    PkgDep::ResultList good;
    PkgDep::ErrorResultList bad;

    // pretend an empty buildroot and try to install all packages specified on
    // command line
    success = runsolver(argv, good, bad);

    if(!success)
    {
	cout << "unresolvable dependencies:" << endl;
	numbad = printbadlist(bad);
	return 1;
    }

    // all packages that are in the good list need to be present in the
    // buildroot
    for(PkgDep::ResultList::const_iterator p=good.begin();p!=good.end();++p)
    {
	need.insert(p->name);
    }

    // check which packages are already in the buildroot, fill the sets
    // acordingly
    computechanges(need, keep, install, update, remove, true);

    cout << "after first run "
	<< stringutil::form("keep %d, install %d, update %d, remove %d",
	    keep.size(),install.size(),update.size(),remove.size()) << endl;

    success = Y2PM::packageManager().solveInstall(good, bad, true);
    
    computechanges(need, keep, install, update, remove, false);

    cout << "after second run "
	<< stringutil::form("keep %d, install %d, update %d, remove %d",
	    keep.size(),install.size(),update.size(),remove.size()) << endl;

    build_printstringset("install", install);
    build_printstringset("update", update);
    build_printstringset("keep", keep);
    build_printstringset("remove", remove);

    if(!success)
    {
	cout << "unresolvable dependencies after second run, exit" << endl;
	numbad = printbadlist(bad);
	return 1;
    }
    
    return 0;
}

