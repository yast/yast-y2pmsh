#include <fstream>
#include <sstream>

#include <y2util/Y2SLog.h>

#include <Y2PM.h>
#include <y2pm/PMPackageManager.h>
//#include <y2pm/Query.h>

#include "y2pmsh.h"
#include "summary.h"
#include "helpscreen.h"

using namespace std;

int search(vector<string>& argv)
{
    list<PMSelectablePtr> matches;

    if(argv.size() < 2 || argv[1] == "--help")
    {
	HelpScreen h("search");
	h.additionalparams("STRING");
//	h.Parameter(HelpScreenParameter("-a", "", "all matches"));
	cout << h;
	return 0;
    }

    string what = argv[1];
    
    PMManager::PMSelectableVec::const_iterator it = Y2PM::packageManager().begin();
    for(; it != Y2PM::packageManager().end(); ++it)
    {
	PMSelectablePtr sp = (*it);
	if(!sp) continue;

//	if(sp->name().asString().substr(0,what.length()) == what)
	if(sp->name().asString().find(what) != string::npos)
	{
	    matches.push_back(sp);
	}
    }

    PrintSelectable::Flags flags;
    flags.summary = true;

    for_each(matches.begin(), matches.end(), PrintSelectable(cout, flags));

    return 0;
}

struct matches_t
{
    matches_t(PMSolvablePtr p, PkgRelation r) : pkg(p), rel(r) {};
    PMSolvablePtr pkg;
    PkgRelation rel;
};

// TODO: belongs into PMSolvable
static bool solvable_does_require(PMSolvablePtr s, PkgRelation what, list<matches_t>* matches = NULL)
{
    bool found = false;

    if(!s) return false;
    
    PMSolvable::PkgRelList_iterator it = s->requires_begin();

    for(; it != s->requires_end(); ++it)
    {
	if((*it).matches(PkgRelation(what)))
	{
	    if(matches)
		matches->push_back(matches_t(s, *it));
	    found = true;
	}
    }
    return found;
}

int whatrequires(vector<string>& argv)
{
    if(argv.size() < 2 || argv[1] == "--help")
    {
	HelpScreen h("whatrequires");
//	h.additionalparams("STRING");
//	h.Parameter(HelpScreenParameter("-a", "", "all matches"));
	cout << h;
	return 0;
    }


    PkgName what(argv[1]);
    list<matches_t> matches;
    
    PMManager::PMSelectableVec::const_iterator it = Y2PM::packageManager().begin();
    for(; it != Y2PM::packageManager().end(); ++it)
    {
	PMSelectablePtr sp = (*it);
	if(!sp) continue;

	PMPackagePtr p;

	p = sp->theObject();

	if(!p) continue;

	solvable_does_require(p, what, &matches);
    }

    for(list<matches_t>::iterator it = matches.begin();
	it != matches.end(); ++it)
    {
	PMPackagePtr pp = it->pkg;
	if(!pp) continue;

	cout << pp->name() << " requires " << it->rel << endl;
    }

    return 0;
}

int whatdependson(vector<string>& argv)
{
    if(argv.size() < 2 || argv[1] == "--help")
    {
	HelpScreen h("whatdependson");
	h.additionalparams("PACKAGE");
//	h.Parameter(HelpScreenParameter("-a", "", "all matches"));
	cout << h;
	return 0;
    }


    string what(argv[1]);
    list<matches_t> matches;
    
    PMSelectablePtr sp1 = Y2PM::packageManager().getItem(what);
    if(!sp1)
    {
	cout << what << " not found" << endl;
	return 1;
    };

    PMPackagePtr p1;
    p1 = sp1->theObject();

    if(!p1) return 1;

    PMManager::PMSelectableVec::const_iterator it = Y2PM::packageManager().begin();
    for(; it != Y2PM::packageManager().end(); ++it)
    {
	PMSelectablePtr sp = (*it);
	if(!sp || sp == sp1) continue;

	PMPackagePtr p;

	p = sp->theObject();

	if(!p) continue;

	PMSolvable::Provides_iterator it = p1->all_provides_begin();

	for(; it != p1->all_provides_end(); ++it)
	{
	    solvable_does_require(p, *it, &matches);
	}
    }

    for(list<matches_t>::iterator it = matches.begin();
	it != matches.end(); ++it)
    {
	PMPackagePtr pp = it->pkg;
	if(!pp) continue;

	cout << pp->name() << " requires " << it->rel << endl;
    }

    return 0;
}

int whatprovides(vector<string>& argv)
{
    if(argv.size() < 2 || argv[1] == "--help")
    {
	HelpScreen h("whatprovides");
	cout << h;
	return 0;
    }


    PkgName what(argv[1]);
    list<matches_t> matches;
    
    PMManager::PMSelectableVec::const_iterator it = Y2PM::packageManager().begin();
    for(; it != Y2PM::packageManager().end(); ++it)
    {
	PMSelectablePtr sp = (*it);
	if(!sp) continue;

	PMPackagePtr p;

	p = sp->theObject();

	if(!p) continue;

	PMSolvable::Provides_iterator it = p->all_provides_begin();

	for(; it != p->all_provides_end(); ++it)
	{
	    if((*it).matches(PkgRelation(what)))
	    {
		matches.push_back(matches_t(p, *it));
	    }
	}

    }

    for(list<matches_t>::iterator it = matches.begin();
	it != matches.end(); ++it)
    {
	PMPackagePtr pp = it->pkg;
	if(!pp) continue;

	cout << pp->name() << " provides " << it->rel << endl;
    }

    return 0;
}

int depends(vector<string>& argv)
{
    if(argv.size() < 2 || argv[1] == "--help")
    {
	HelpScreen h("depends");
	h.additionalparams("PACKAGE");
//	h.Parameter(HelpScreenParameter("-a", "", "all matches"));
	cout << h;
	return 0;
    }


    string what(argv[1]);
    list<matches_t> matches;
    
    PMSelectablePtr sp1 = Y2PM::packageManager().getItem(what);
    if(!sp1)
    {
	cout << what << " not found" << endl;
	return 1;
    };

    PMPackagePtr p1;
    p1 = sp1->theObject();

    if(!p1) return 1;

    PMManager::PMSelectableVec::const_iterator it = Y2PM::packageManager().begin();
    for(; it != Y2PM::packageManager().end(); ++it)
    {
	PMSelectablePtr sp = (*it);
	if(!sp || sp == sp1) continue;

	PMPackagePtr p;

	p = sp->theObject();

	if(!p) continue;

	PMSolvable::PkgRelList_iterator it = p1->requires_begin();

	for(; it != p1->requires_end(); ++it)
	{
	    if(p->doesProvide(*it))
		matches.push_back(matches_t(p, *it));
	}
    }

    for(list<matches_t>::iterator it = matches.begin();
	it != matches.end(); ++it)
    {
	PMPackagePtr pp = it->pkg;
	if(!pp) continue;

	cout << pp->name() << " via provides " << it->rel << endl;
    }

    return 0;
}


#if 0
int query(vector<string>& argv)
{
    stringstream str;

    for(vector<string>::iterator it = argv.begin()+1; it != argv.end(); ++it)
    {
	if(it != argv.begin()+1)
	    str << ' ';

	str << *it;
    }

    cout << str.str() << endl;;

    Query q;
    PMPackageManager& m = Y2PM::packageManager();

    PMError err = q.queryPackage(str.str());

    if(err)
    {
	cout << "Error starting query: " << err << " at " << q.failedPos() << endl;
	return 1;
    }

    PMPackagePtr p = q.nextPackage(m);

    while(p)
    {
	cout << p->name() << endl;
	p = q.nextPackage(m);
    }
    
    return 0;
}
#endif

int newer(vector<string>& argv)
{
    bool install = false;
    if(argv.size() > 1)
    {
	if(argv[1] == "--help")
	{
	    HelpScreen h("source");
	    h.Parameter(HelpScreenParameter("-i", "", "install newer packages"));
	    cout << h;
	    return 0;
	}
	else if(argv[1] == "-i")
	{
	    install = true;
	}
    }

    for (PMManager::PMSelectableVec::iterator it = Y2PM::packageManager().begin();
	it != Y2PM::packageManager().end(); ++it )
    {
	if((*it)->has_both_objects() && !(*it)->downgrade_condition())
	{
	    if(install)
	    {
		cout << (*it)->name();
		if((*it)->is_taboo())
		    cout << " taboo";
		else if(!(*it)->user_set_install())
		    cout << " FAILED";

		cout << endl;
	    }
	    else
	    {
		cout << (*it)->name() << " ("
		     << (*it)->installedObj()->edition().asString() << " / "
		     << (*it)->candidateObj()->edition().asString() << ")" << endl;
	    }
	}
    }

    return 0;
}

int alternatives(vector<string>& argv)
{
    list<matches_t> matches;
    PkgSet candidates;
    
    PMManager::PMSelectableVec::const_iterator it = Y2PM::packageManager().begin();
    for(; it != Y2PM::packageManager().end(); ++it)
    {
	PMSelectablePtr sp = (*it);
	if(!sp) continue;

	PMPackagePtr p;

	p = sp->candidateObj();

	if(!p) continue;

	candidates.add(p);
    }

    PkgSet::InvRel_const_iterator irit; 

    for(irit = candidates.provided_begin();
	irit != candidates.provided_end(); ++irit)
    {
	const PkgSet::RevRelList_type& rrl = irit->second;
	if(rrl.size() > 1)
	{
	    PkgRelation *prevrel = NULL;
	    for(PkgSet::RevRelList_const_iterator rrlit = rrl.begin();
		    rrlit != rrl.end(); ++rrlit)
	    {
		if(!prevrel || *prevrel != rrlit->relation())
		{
		    delete prevrel;
		    prevrel = new PkgRelation(rrlit->relation());
		    cout << *prevrel << ':';
		}

		cout << ' ' << rrlit->pkg()->nameEd();
	    }
	    cout << endl;
	    delete(prevrel);
	}
    }

    return 0;
}

static bool getconflictsfor(PMSelectablePtr sp1, list<matches_t>& matches)
{
    bool hasconflicts = false;

    if (!sp1) return false;

    PMPackagePtr p1;
    p1 = sp1->theObject();

    if(!p1) return false;
    
    PMManager::PMSelectableVec::const_iterator it = Y2PM::packageManager().begin();
    for(; it != Y2PM::packageManager().end(); ++it)
    {
	PMSelectablePtr sp = (*it);
	if(!sp || sp == sp1) continue;

	PMPackagePtr p;

	p = sp->theObject();

	if(!p) continue;

	PMSolvable::PkgRelList_const_iterator it = p1->conflicts_begin();

	for(; it != p1->conflicts_end(); ++it)
	{
	    if(p->doesProvide(*it))
	    {
		matches.push_back(matches_t(p, *it));
		hasconflicts = true;
	    }
	}
    }

    return hasconflicts;
}

// inefficient
int allconflicts(vector<string>& argv)
{
    if(argv.size() > 1 && argv[1] == "--help")
    {
	HelpScreen h("allconflicts");
//	h.Parameter(HelpScreenParameter("-a", "", "all matches"));
	cout << h;
	return 0;
    }

    map<PMSelectablePtr, set<PMSolvablePtr> > conflictpkgs;

    PMManager::PMSelectableVec::const_iterator it = Y2PM::packageManager().begin();
    for(; it != Y2PM::packageManager().end(); ++it)
    {
	PMSelectablePtr sp = (*it);
	if(!sp) continue;

	list<matches_t> matches;
	getconflictsfor(sp, matches);

	for(list<matches_t>::iterator it = matches.begin();
	    it != matches.end(); ++it)
	{
	    PMSolvablePtr pp = it->pkg;
	    if(!pp) continue;

	    conflictpkgs[sp].insert(pp);
	}
    }

    map<PMSelectablePtr, set<PMSolvablePtr> >::iterator cit = conflictpkgs.begin();

    for(; cit != conflictpkgs.end(); ++cit)
    {
	cout << cit->first->name() << " conflicts";

	set<PMSolvablePtr>::iterator sit = cit->second.begin();
	for(; sit != cit->second.end(); ++sit)
	{
	     cout << " " << (*sit)->name();
	}

	cout << endl;
    }

    return 0;
}

#if 0 // needs to be fixed
int allconflicts(vector<string>& argv)
{
    if(argv.size() > 1 || argv[1] == "--help")
    {
	HelpScreen h("allconflicts");
//	h.Parameter(HelpScreenParameter("-a", "", "all matches"));
	cout << h;
	return 0;
    }

    PkgSet pkgs;

    map<PMSolvablePtr, set<PMSolvablePtr> > conflictpkgs;

    PMManager::PMSelectableVec::const_iterator it = Y2PM::packageManager().begin();
    for(; it != Y2PM::packageManager().end(); ++it)
    {
	PMSelectablePtr sp = (*it);
	if(!sp) continue;

	PMPackagePtr p;
	p = sp->theObject();
	if(!p) continue;

	pkgs.add(p);
    }

    for(PkgSet::iterator it = pkgs.begin(); it != pkgs.end(); ++it)
    {
	list<matches_t> matches;

	PMSolvablePtr solv = it->second;

	for(PMSolvable::PkgRelList_const_iterator conflit = solv->conflicts_begin();
	    conflit != solv->conflicts_end(); ++conflit)
	{
	    PkgSet::InvRel_iterator providinglist = pkgs.provided()[conflit->name()];

	    if(providinglist == pkgs.provided_end())
		continue;

	    for(PkgSet::RevRelList_iterator provider = providinglist.begin();
		provider != providinglist.end())
	    {
		if(conflit->matches(provider->relation()))
		    conflictpkgs[solv] = provider->pkg();
	    }
	}
    }


    map<PMSolvablePtr, set<PMSolvablePtr> >::iterator cit = conflictpkgs.begin();

    for(; cit != conflictpkgs.end(); ++cit)
    {
	cout << cit->first->name() << " conflicts";

	set<PMSolvablePtr>::iterator sit = cit->second.begin();
	for(; sit != cit->second.end(); ++sit)
	{
	     cout << (*sit)->name();
	}

	cout << endl;
    }

    return 0;
}
#endif

int whatconflictswith(vector<string>& argv)
{
    if(argv.size() < 2 || argv[1] == "--help")
    {
	HelpScreen h("whatconflictswith");
	h.additionalparams("PACKAGE");
	cout << h;
	return 0;
    }

    string what(argv[1]);
    list<matches_t> matches;

    PMSelectablePtr sp1 = Y2PM::packageManager().getItem(what);
    if(!sp1)
    {
	cout << what << " not found" << endl;
	return 1;
    };

    getconflictsfor(sp1, matches);

    for(list<matches_t>::iterator it = matches.begin();
	it != matches.end(); ++it)
    {
	PMPackagePtr pp = it->pkg;
	if(!pp) continue;

	cout << pp->name() << " conflicts via provides " << it->rel << endl;
    }

    return 0;
}

// vim: sw=4
