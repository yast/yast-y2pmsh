/*
  show_pm.h

  cout << PMSolvable
  cout << PMObject
  cout << PMPackage

*/

#ifndef SHOW_PM_H
#define SHOW_PM_H

#include <string>
#include <y2pm/PMPackage.h>
#include <y2pm/PMPackagePtr.h>
#include <y2pm/PMSelection.h>
#include <y2pm/PMSelectionPtr.h>


std::string strlist2string (const std::list<std::string>& slist, const char *sep = " ");
std::string pkglist2string (const std::list<PMPackagePtr>& plist, const char *sep = " ");
std::string sellist2string (const std::list<PMSelectionPtr>& slist, const char *sep = " ");
void show_pmsolvable (PMSolvablePtr p);
void show_pmobject (PMObjectPtr p, bool only_cached = false);
void show_pmpackage (PMPackagePtr p, bool only_cached = false);
void show_pmselection (PMSelectionPtr s);

#endif //SHOW_PM_H

