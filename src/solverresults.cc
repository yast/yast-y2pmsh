#include <iostream>

#include <y2pm/PkgDep.h>

#include "y2pmsh.h"
#include "variables.h"

using namespace std;

int printgoodlist(PkgDep::ResultList& good)
{
    int numinst = 0;
    int _verbose = variables["verbose"].getInt();
    cout << "*** Packages to install ***" << endl;

     // otherwise, print what should be installed and what removed
    for(PkgDep::ResultList::const_iterator p=good.begin();p!=good.end();++p)
    {
	switch (_verbose) {
	    case 0:
		if(p!=good.begin())
		    cout << " ";
		cout <<  p->name;
		break;
	    case 1:
		cout << "install " << p->name << "-" << p->edition << endl;
		break;
	    default:
		cout << "install " << *p << endl;
		break;
	}
	numinst++;

    }
    if(!_verbose) cout << endl;

    return numinst;
}

int printremovelist(PkgDep::SolvableList& to_remove)
{
    int numrem = 0;
    int _verbose = variables["verbose"].getInt();

    cout << "*** Packages to remove ***" << endl;
    for(PkgDep::SolvableList::const_iterator q=to_remove.begin();q!=to_remove.end();++q)
    {
	switch (_verbose)
	{
	    case 0:
		if(q!=to_remove.begin())
		    cout << " ";
		cout << (*q)->name();
		break;
	    case 1:
		cout << (*q)->name() << "-" << (*q)->edition() << endl;
		break;
	    default:
		cout << *q << endl;
		break;
	}
	numrem++;
    }
    if(!_verbose) cout << endl;

    return numrem;
}

int printbadlist(PkgDep::ErrorResultList& bad)
{
    int numbad = 0;

    if(bad.empty()) return 0;

    cout << "*** Conflicts ***" << endl;
    for( PkgDep::ErrorResultList::const_iterator p = bad.begin();
	 p != bad.end(); ++p ) {
	cout << *p << endl;
	numbad++;
    }
    return numbad;
}
