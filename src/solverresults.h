#ifndef Y2PMSH_SOLVERRESULTS_H
#define Y2PMSH_SOLVERRESULTS_H

int printbadlist(PkgDep::ErrorResultList& bad);
int printgoodlist(PkgDep::ResultList& good);
int printremovelist(PkgDep::SolvableList& to_remove);

#endif
