#include <fstream>

#include <algorithm>

#include <y2util/Y2SLog.h>

#include <Y2PM.h>
#include <y2pm/PMPackageManager.h>
#include <y2pm/PMSelectionManager.h>
#include <y2pm/PkgSet.h>

#include "y2pmsh.h"
#include "summary.h"
#include "helpscreen.h"

using namespace std;

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
	void operator()(const PkgRelation& rel)
	{
	    _os << stringutil::form(_form, rel.asString().c_str());
	}
};

class CheckIfBroken
{
    private:
	PkgRelation& _rel;
	bool& _matchfound;
    public:
	CheckIfBroken(PkgRelation& rel, bool& matchfound) : _rel(rel), _matchfound(matchfound) {}
	void operator()(const PkgRevRelation& rrel)
	{
	    if(rrel.relation().matches(_rel))
	    {
		_matchfound = true;
	    }
	}
};

class CheckRequirements
{
    private:
	PMSolvablePtr _p;
	PkgSet& _c;
	PMSolvable::PkgRelList_type& _broken;
    public:
	CheckRequirements(PMSolvablePtr p, PkgSet& c, PMSolvable::PkgRelList_type& broken)
	    : _p(p), _c(c), _broken(broken) {}
	void operator()(PkgRelation& rel)
	{
	    // ignore rpmlib dep
	    if(rel.name()->find("rpmlib(") != string::npos)
		return;

	    const PkgSet::RevRelList_type& provides_rel =
		PkgSet::getRevRelforPkg(_c.provided(), rel.name());

	    bool matchfound = false;
	    for_each(provides_rel.begin(), provides_rel.end(), CheckIfBroken(rel, matchfound));
	    if(!matchfound)
	    {
		_broken.push_back(rel);
	    }
	}
};

class CheckCandidateDeps
{
    private:
	PkgSet& _c;
    public:
	CheckCandidateDeps(PkgSet& c) : _c(c) {}
	void operator()(std::pair<PkgName,PMSolvablePtr> p)
	{
	    PMSolvable::PkgRelList_type brokendeps;
	    for_each(
		p.second->requires_begin(),
		p.second->requires_end(),
		CheckRequirements(p.second, _c, brokendeps));

	    for_each(
		brokendeps.begin(),
		brokendeps.end(),
		Print(cout, string(p.second->name().asString() + " requires %s\n").c_str()));
	}
	
};

int distrocheck(vector<string>& argv)
{
    if(argv.size() > 1)
    {
	if(argv[1] == "--help")
	{
	    HelpScreen h(argv[0]);
	    cout << h;
	    return 0;
	}
    }

    PMManager& manager = Y2PM::packageManager();
    PMManager::PMSelectableVec::const_iterator it, end;

    PkgSet candidates;

    it = manager.begin();
    end = manager.end();
    for (;it!=end;++it)
    {
	PMSelectablePtr sel(*it);
	if(!sel) continue;

	PMPackagePtr pkg;
	pkg = sel->candidateObj();
	if(!pkg) continue;

	if(candidates.includes(pkg->name()))
	{
	    cout << pkg->name() << " already in set" << endl;
	}
	candidates.add(pkg, true);
    }

    for_each(candidates.begin(), candidates.end(), CheckCandidateDeps(candidates));

    return 0;
}


// vim: sw=4
