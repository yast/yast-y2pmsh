#include <iostream>

#include <Y2PM.h>
#include <y2pm/InstSrcDescr.h>
#include <y2pm/PMPackage.h>
#include <y2pm/PMPackageManager.h>
#include <y2pm/PMSelectionManager.h>

#include "y2pmsh.h"
#include "show_pm.h"
#include "helpscreen.h"

using namespace std;

static void show_basicpkginfo (PMPackagePtr p)
{
    if(!p) return;

//    cout << stringutil::form("Name        : %-27s", p->name()->c_str()) << endl;
//    cout << stringutil::form("Version     : %-27s", p->version().c_str()) << endl;
    cout << "Name        : ";
    cout.width(27); cout.setf(ios_base::left,ios_base::adjustfield);
    cout << p->name() << endl;
    
    cout << "Version     : ";
    cout.width(27); cout.setf(ios_base::left,ios_base::adjustfield);
    cout << p->version();
    cout << "Vendor      : ";
    cout << p->vendor() << endl;
    
    cout << "Release     : ";
    cout.width(27); cout.setf(ios_base::left,ios_base::adjustfield);
    cout << p->release();
    cout << "Build Date  : ";
    cout << p->buildtime() << endl;

    cout << "Install Date: ";
    cout.width(27); cout.setf(ios_base::left,ios_base::adjustfield);
    cout << p->installtime();
    cout << "Build Host  : ";
    cout << p->buildhost() << endl;
    
    cout << "Size        : ";
    cout.width(27); cout.setf(ios_base::left,ios_base::adjustfield);
    cout << p->size();
    cout << "License     : ";
    cout << p->license() << endl;

    cout << "MediaNr     : ";
    cout.width(27); cout.setf(ios_base::left,ios_base::adjustfield);
    cout << p->medianr();
    cout << "Distribution: ";
    cout << p->distribution() << endl;

    cout << "Packager    : ";
    cout.width(27); cout.setf(ios_base::left,ios_base::adjustfield);
    cout << p->packager() << endl;

    cout << "Summary     : ";
    cout.width(27); cout.setf(ios_base::left,ios_base::adjustfield);
    cout << p->summary() << endl;

    cout << "Description : ";
    cout.width(27); cout.setf(ios_base::left,ios_base::adjustfield);
    cout << strlist2string(p->description(),"\n") << endl;
}

static int show_internal(vector<string>& argv, PMManager& manager, bool selection = false)
{
    vector<string>::iterator it=argv.begin();
    ++it; // first one is function name itself

    enum { any, inst, cand, list } whichone = any;
    bool verbose = false;
    bool show_provides = false;
    bool show_requires = false;
    bool show_obsoletes = false;
    bool show_conflicts = false;

    if(it == argv.end() || *it == "--help")
    {
	HelpScreen h(argv[0]);
	h.optionaloptions();
	h.additionalparams("OBJECTS");
	h.Parameter(HelpScreenParameter("-i","", "show installed object"));
	h.Parameter(HelpScreenParameter("-c","", "show candidate object"));
	h.Parameter(HelpScreenParameter("-l","", "list installed/candidate/available"));
	h.Parameter(HelpScreenParameter("","--deps", "list all dependencies"));
	h.Parameter(HelpScreenParameter("","--provides", "list provides"));
	h.Parameter(HelpScreenParameter("","--requires", "list requires"));
	h.Parameter(HelpScreenParameter("","--obsoletes", "list obsoletes"));
	h.Parameter(HelpScreenParameter("","--conflicts", "list conflicts"));
	cout << h;
	return 0;
    }

    for(;it != argv.end() && (*it)[0] == '-'; ++it)
    {
	if(*it == "-i")
	{
	    whichone = inst;
	}
	else if(*it == "-c")
	{
	    whichone = cand;
	}
	else if(*it == "-l")
	{
	    whichone = list;
	}
	else if(*it == "-v")
	{
	    verbose = true;
	}
	else if(*it == "--deps")
	{
	    show_provides = show_requires = show_obsoletes = show_conflicts = true;
	}
	else if(*it == "--provides")
	{
	    show_provides = true;
	}
	else if(*it == "--requires")
	{
	    show_requires = true;
	}
	else if(*it == "--obsoletes")
	{
	    show_obsoletes = true;
	}
	else if(*it == "--conflicts")
	{
	    show_conflicts = true;
	}
	else
	{
	    cout << "invalid parameter " << *it << endl;
	    return 1;
	}
    }

    for(;it != argv.end();++it)
    {
	PMSelectablePtr sel = manager.getItem(*it);

	if(!sel) { cout << *it << " is not available" << endl; continue; }

	if(whichone == list)
	{
	    PMObjectPtr instobj = sel->installedObj();
	    if(instobj)
	    {
		cout << "i ";
		cout << instobj->nameEdArch() << "  (" << instobj->instSrcLabel() << ")" << endl;
	    }

	    PMSelectable::PMObjectList::const_iterator it = sel->av_begin();
	    for(;it!=sel->av_end();++it)
	    {
		PMObjectPtr obj = *it;

		if(!obj) continue;
		
		if(obj->isInstalledObj())
		    continue;
		else if(obj->isCandidateObj())
		{
		    if(instobj && obj->edition() > instobj->edition())
			cout << "> ";
		    else if(instobj && obj->edition() < instobj->edition())
			cout << "< ";
		    else if(instobj && obj->edition() == instobj->edition())
			cout << "= ";
		    else
			cout << "* ";
		}
		else
		    cout << "  ";

		cout << obj->nameEdArch() << "  (" << obj->instSrcLabel() << ")" << endl;
	    }
	}
	else
	{
	    PMObjectPtr obj;

	    switch(whichone)
	    {
		case list:
		case any: obj = sel->has_installed()?sel->installedObj():sel->theObject(); break;
		case inst: obj = sel->installedObj(); break;
		case cand: obj = sel->candidateObj(); break;
	    }

	    if(!obj) { cerr << "Object " << *it << " is NULL" << endl; continue; }

	    if(show_requires || show_provides || show_obsoletes || show_requires)
	    {
		cout << obj->nameEd() << ": " << endl;
		PMSolvable::PkgRelList_type::const_iterator it;

		if(show_provides)
		{
		    cout << "  Provides:\n";
		    cout << "    " << obj->self_provides() << '\n';
		    for(it = obj->provides_begin(); it != obj->provides_end(); ++it)
			cout << "    " << *it << '\n';
		}

		if(show_requires)
		{
		    cout << "  Requires:\n";
		    for(it = obj->requires_begin(); it != obj->requires_end(); ++it)
			cout << "    " << *it << '\n';
		}

		if(show_obsoletes)
		{
		    cout << "  Obsoletes:\n";
		    for(it = obj->obsoletes_begin(); it != obj->obsoletes_end(); ++it)
			cout << "    " << *it << '\n';
		}

		if(show_conflicts)
		{
		    cout << "  Conflicts:\n";
		    for(it = obj->conflicts_begin(); it != obj->conflicts_end(); ++it)
			cout << "    " << *it << '\n';
		}
	    }
	    else if (selection)
	    {
		show_pmselection(obj);
	    }
	    else
	    {
		if(verbose)
		    show_pmpackage(obj);
		else
		    show_basicpkginfo(obj);
	    }
	}
    }

    return 0;
}

int show(vector<string>& argv)
{
    return show_internal(argv, Y2PM::packageManager());
}

int selshow(vector<string>& argv)
{
    return show_internal(argv, Y2PM::selectionManager(), true);
}

