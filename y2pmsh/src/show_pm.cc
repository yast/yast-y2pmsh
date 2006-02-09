/*
  show_PM.cc

  cout << PMPackage

*/

#include <list>
#include <string>
#include <y2pm/PMError.h>
#include <y2pm/PMSolvable.h>
#include <y2pm/PMSolvablePtr.h>
#include <y2pm/PMObject.h>
#include <y2pm/PMObjectPtr.h>
#include <y2pm/PMPackage.h>
#include <y2pm/PMPackagePtr.h>
#include <y2pm/PMSelectable.h>
#include <y2pm/PMSelectablePtr.h>
#include <y2pm/PkgDu.h>
#include <iostream>
#include <algorithm>

#include "show_pm.h"

using namespace std;

std::string
strlist2string (const std::list<std::string>& slist, const char *sep)
{
   std::string ret;
   if (slist.empty())
	return ret;
   for (std::list<std::string>::const_iterator pos = slist.begin();
	pos != slist.end(); ++pos)
   {
	if (!ret.empty())
	    ret += sep;
	ret += *pos;
   }
   return ret;
}

std::string
pkglist2string (const std::list<PMPackagePtr>& plist, const char *sep)
{
   std::string ret;
   if (plist.empty())
	return ret;
   for (std::list<PMPackagePtr>::const_iterator pos = plist.begin();
	pos != plist.end(); ++pos)
   {
	if (!ret.empty())
	    ret += sep;
	if (!(*pos))
	    ret += "(nil)";
	else
	    ret += ((const std::string &)((*pos)->name()) + "-" + (*pos)->version() + "-" + (*pos)->release() + "." + (const std::string &)((*pos)->arch()));
   }
   return ret;
}


std::string
slclist2string (const std::set<PMSelectablePtr>& slist, const char *sep)
{
   std::string ret;
   if (slist.empty())
	return ret;
   for (std::set<PMSelectablePtr>::const_iterator pos = slist.begin();
	pos != slist.end(); ++pos)
   {
	if (!ret.empty())
	    ret += sep;
	if (!(*pos))
	    ret += "(nil)";
	else
	{
	    PMPackagePtr package = (*pos)->candidateObj();
	    if (package)
	    {
		ret += ((const std::string &)(package->name()) + "-" + package->version() + "-" + package->release() + "." + (const std::string &)(package->arch()));
	    }
	    else
		ret += (*pos)->name();
	}
   }
   return ret;
}


std::string
sellist2string (const std::list<PMSelectionPtr>& slist, const char *sep)
{
   std::string ret;
   if (slist.empty())
	return ret;
   for (std::list<PMSelectionPtr>::const_iterator pos = slist.begin();
	pos != slist.end(); ++pos)
   {
	if (!ret.empty())
	    ret += sep;
	ret += ((const std::string &)((*pos)->name()) + "-" + (*pos)->version() + "-" + (*pos)->release() + "." + (const std::string &)((*pos)->arch()));
   }
   return ret;
}


void
show_pmsolvable (PMSolvablePtr p)
{
    cout << "-- Solvable --" << endl;
    cout << p->name() << "-" << p->version() << "-" << p->release() << "." << p->arch() << endl;
    cout << "Requires: " << strlist2string (PMSolvable::PkgRelList2StringList (p->requires())) << endl;
    cout << "PreRequires: " << strlist2string (PMSolvable::PkgRelList2StringList (p->prerequires())) << endl;
    cout << "Provides: " << strlist2string (PMSolvable::PkgRelList2StringList (p->provides())) << endl;
    cout << "Obsoletes: " << strlist2string (PMSolvable::PkgRelList2StringList (p->obsoletes())) << endl;
    cout << "Conflicts: " << strlist2string (PMSolvable::PkgRelList2StringList (p->conflicts())) << endl;

    return;
}

void
show_pmobject (PMObjectPtr p, bool only_cached)
{
    show_pmsolvable (p);
    cout << "-- Object --" << endl;

    cout << "Summary: " << p->summary() << endl;
    cout << "Size: " << p->size().asString() << endl;
    if (!only_cached)
    {
	cout << "Description: " << strlist2string (p->description(), "\n") << endl;
	cout << "InsNotify: " << strlist2string (p->insnotify(), "\n") << endl;
	cout << "DelNotify: " << strlist2string (p->delnotify(), "\n") << endl;
    }
    return;
}

void
show_pmpackage (PMPackagePtr p, bool only_cached)
{
    if (!p)
	return;

    show_pmobject ((PMObjectPtr)p, only_cached);

    cout << "-- Package --" << endl;

    cout << "Splitprovides: ";
    {
	PkgSplitSet sps = p->splitprovides();
	for (PkgSplitSet::iterator it = sps.begin(); it != sps.end(); ++it)
	{
	    if(it!= sps.begin()) cout << ' ';
	    cout << *it;
	}
	cout << endl;
    }
    cout << "Buildtime: " << p->buildtime() << endl;
    cout << "Group: " << p->group() << endl;

    if (only_cached)
    {
	return;
    }
    cout << "Buildhost: " << p->buildhost() << endl;
    cout << "Installtime: " << p->installtime() << endl;
    cout << "Distribution: " << p->distribution() << endl;
    cout << "Vendor: " << p->vendor() << endl;
    cout << "License: " << p->license() << endl;
    cout << "Packager: " << p->packager() << endl;
    cout << "Changelog: " << strlist2string(p->changelog(), "\n") << endl;
    cout << "Url: " << p->url() << endl;
    cout << "OS: " << p->os() << endl;
    cout << "PreIn: " << strlist2string(p->prein(), "\n") << endl;
    cout << "PostIn: " << strlist2string(p->postin(), "\n") << endl;
    cout << "PreUn: " << strlist2string(p->preun(), "\n") << endl;
    cout << "PostUn: " << strlist2string(p->postun(), "\n") << endl;
    cout << "SourceLoc: " << p->sourceloc() << endl;
    cout << "SourceSize: " << p->sourcesize() << endl;
    cout << "Archivesize: " << p->archivesize() << endl;
    cout << "Authors: " << strlist2string(p->authors(), ", ") << endl;
    cout << "Files: " << strlist2string(p->filenames(), "\n") << endl;
    cout << "Recommends: " << strlist2string(p->recommends()) << endl;
    cout << "Suggests: " << strlist2string(p->suggests()) << endl;
    cout << "Location: '" << p->location() << "'" << endl;
    cout << "MediaNr: " << p->medianr () << endl;
    cout << "Keywords: " << strlist2string(p->keywords()) << endl;
    PkgDu dudata;
    cout << "DU: " << p->du(dudata) << endl;
    cout << "========" << endl;
    return;
}


void
show_pmselection (PMSelectionPtr s)
{
    show_pmobject ((PMObjectPtr)s);

    char *locales[] = { "", "default", "de", "en", "cs", "es", "fr", "gl", "hu", "it", "ja", "nl", "pt", "sv", "tr", "ru", "cz", 0 };
//    char *locales[] = { "en", "de", "fr", 0};
    int pos = 0;
    cout << "Category: " << s->category () << endl;
    cout << "IsBase: " << s->isBase () << endl;
    cout << "Visible: " << s->visible () << endl;
    cout << "Order: " << s->order() << endl;
    cout << "Recommends: " << strlist2string(s->recommends()) << endl;
    cout << "RecommendsPtrs: " << sellist2string(s->recommends_ptrs()) << endl;
    cout << "Suggests: " << strlist2string(s->suggests()) << endl;
    cout << "SuggestsPtrs: " << sellist2string(s->suggests_ptrs(), ", ") << endl;
    cout << "Archivesize: " << s->archivesize() << endl;
    while (locales[pos] != 0)
    {
	cout << "Summary[" << locales[pos] << "]: " << s->summary(LangCode(locales[pos])) << endl;
	cout << "InsPacks[" << locales[pos] << "]: " << strlist2string(s->inspacks(LangCode(locales[pos]))) << endl;
	cout << "InsPacksPtrs[" << locales[pos] << "]: (";
	cout << slclist2string(s->inspacks_ptrs(LangCode(locales[pos])), ", ") << ")" << endl;
	cout << "DelPacks[" << locales[pos] << "]: (";
	cout << strlist2string(s->delpacks(LangCode(locales[pos]))) << ")" << endl;
	cout << "DelPacksPtrs[" << locales[pos] << "]: (";
	cout << slclist2string(s->delpacks_ptrs(LangCode(locales[pos])), ", ") << ")" << endl;
	pos++;
    }
    cout << "========" << endl;
    return;
}

