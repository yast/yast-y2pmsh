#ifndef Y2PMSH_SUMMARY_H
#define Y2PMSH_SUMMARY_H

#include <string>
#include <vector>

/** functor to print a selectable in for_each() */
class PrintSelectable
{
    public:
	class Flags
	{
	    public:
		bool onlyinstalled : 1;
		bool onlychanged : 1;
		bool summary : 1;
		bool version : 1;
		bool byauto : 1; // only evaluated when onlychanged == true
		bool byappl : 1; // only evaluated when onlychanged == true
		bool byuser : 1; // only evaluated when onlychanged == true

		Flags();
	};

    private:
	std::ostream& _os;
	Flags _flags;

    public:
	PrintSelectable(std::ostream& os, Flags flags);

	void operator()(PMSelectablePtr sptr);
};

int state(std::vector<std::string>& argv);
int selstate(std::vector<std::string>& argv);
int summary(std::vector<std::string>& argv);
int depstats(std::vector<std::string>& argv);

#endif
