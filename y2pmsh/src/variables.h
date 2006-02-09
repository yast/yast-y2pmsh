#ifndef Y2PMSH_VARIABLES_H
#define Y2PMSH_VARIABLES_H

#include <string>
#include <map>

#include "y2pmsh.h"
#include "variable.h"

typedef std::map<std::string,Variable> VariableMap;

extern VariableMap variables;

void init_variables();

int varset(std::vector<std::string>& argv);
int varunset(std::vector<std::string>& argv);

#endif
