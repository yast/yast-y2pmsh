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

class Deps
{
    public:
	class RelationLessOperator
	{
	    public:
		bool operator()(const PkgRelation& l, const PkgRelation& r)
		{
		    return l.asString() < r.asString();
		}
	};
	typedef std::set<PkgRelation, RelationLessOperator> Relations;
	typedef std::map<PMSolvablePtr, Relations> PkgDeps;
	typedef std::map<PMSolvablePtr, PkgDeps > Content;

	class DumpDepending
	{
	    private:
		std::ostream& _os;
	    public:
		DumpDepending(std::ostream& os) : _os(os) {}
		// PMSolvablePtr, Relations
		void operator()(PkgDeps::value_type p)
		{
		    _os << "    " << p.first->name() << ":";
		    for_each(
			p.second.begin(),
			p.second.end(),
			Print(_os, " %s"));
		    _os << endl;
		}
	};

	class DumpPackage
	{
	    private:
		std::ostream& _os;
	    public:
		DumpPackage(std::ostream& os) : _os(os) {}
		// PMSolvablePtr, PkgDeps
		void operator()(Content::value_type p)
		{
		    _os << p.first->nameEd() << ": " << endl;
		    _os << "  Requires:" << endl;
		    for_each(
			p.second.begin(),
			p.second.end(),
			DumpDepending(_os));
		}
	};
	    

    private:
	Content _requirements;

    public:
	Deps()
	{
	}

	void add(PMSolvablePtr r, PMSolvablePtr t, const PkgRelation& rel)
	{
	    _requirements[r][t].insert(rel);
	}

	Content::iterator begin()
	{
	    return _requirements.begin();
	}
	
	Content::iterator end()
	{
	    return _requirements.end();
	}

	std::ostream& dumpOn(std::ostream& os)
	{
	    for_each(_requirements.begin(), _requirements.end(), DumpPackage(os));

	    return os;
	}
	void clear()
	{
	    _requirements.clear();
	}
};

// FIXME
static Deps alldeps;

class CheckIfBroken
{
    private:
	PMSolvablePtr _p;
	PkgRelation& _rel;
	bool& _matchfound;
    public:
	CheckIfBroken(PMSolvablePtr p, PkgRelation& rel, bool& matchfound)
	    : _p(p), _rel(rel), _matchfound(matchfound) {}
	void operator()(const PkgRevRelation& rrel)
	{
	    if(rrel.relation().matches(_rel))
	    {
		_matchfound = true;
//		cout << _p->name() << "depends on " << rrel.pkg()->name() << " via " << rrel.relation() << endl;
		alldeps.add(_p, rrel.pkg(), rrel.relation());
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
	    for_each(provides_rel.begin(), provides_rel.end(), CheckIfBroken(_p, rel, matchfound));
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
	void operator()(PMPackagePtr p)
	{
	    PMSolvable::PkgRelList_type brokendeps;
	    for_each(
		p->requires_begin(),
		p->requires_end(),
		CheckRequirements(p, _c, brokendeps));

	    for_each(
		brokendeps.begin(),
		brokendeps.end(),
		Print(cout, string(p->nameEd() + ": broken requirement %s\n").c_str()));
	}
	
};

int distrocheck(vector<string>& argv)
{
    vector<string>::iterator arg = ++argv.begin();
    bool verbose = false;

    for(;arg != argv.end() && (*arg)[0] == '-'; ++arg)
    {
	if(*arg == "-h" || *arg == "--help")
	{
	    HelpScreen h(argv[0]);
	    cout << h;
	    return 0;
	}
	else if(*arg == "-v")
	{
	    verbose = true;
	}
    }

    PMManager& manager = Y2PM::packageManager();
    PMManager::PMSelectableVec::const_iterator it, end;
    list<PMPackagePtr> tocheck;
    bool checkall = true;

    PkgSet candidates;
    
    if(arg != argv.end())
    {
	checkall = false;
	for(; arg != argv.end(); ++arg)
	{
	    PMSelectablePtr p = manager.getItem(stringutil::trim(*arg));
	    PMPackagePtr pkg;

	    if(p) pkg = p->candidateObj();

	    if(!p || !pkg)
	    {
		cout << *arg << " has no candidate" << endl;
		continue;
	    }
	    tocheck.push_back(pkg);
	}
    }
    
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
	if(checkall)
	    tocheck.push_back(pkg);
    }

    for_each(tocheck.begin(), tocheck.end(), CheckCandidateDeps(candidates));

    if(verbose)
    {
	alldeps.dumpOn(cout);
	alldeps.clear();
    }

    return 0;
}


// vim: sw=4
