#ifndef Y2PMSH_SEARCH_H
#define Y2PMSH_SEARCH_H

#include <string>
#include <vector>

int search(std::vector<std::string>& argv);
int whatrequires(std::vector<std::string>& argv);
int whatdependson(std::vector<std::string>& argv);
int newer(std::vector<std::string>& argv);
//int query(std::vector<std::string>& argv);
int alternatives(std::vector<std::string>& argv);

#endif
