#ifndef HELPSCREEN_H
#define HELPSCREEN_H

#include <list>
#include <string>

/**
 * Parameter for HelpScreen
 * @param s short option, eg. -a
 * @param l long option, eg. --add
 * @param h help text, eg. add foo
 * @see HelpScreen
 * */
class HelpScreenParameter
{
    public:
	HelpScreenParameter(std::string s, std::string l, std::string h);
	const std::string& s() const { return _s; };
	const std::string& l() const { return _l; };
	const std::string& h() const{ return _h; };
    private:
	std::string _s;
	std::string _l;
	std::string _h;
};

/**
 * Assemble some standard help screen
 *
 * Example:
 * HelpScreen h("source");
 * h.Parameter(HelpScreenParameter("-s", "--show", "show known sources"));
 * cout << h;
 * */
class HelpScreen
{
    public:
	HelpScreen(std::string commandname);
	void Parameter(const HelpScreenParameter& parameter);

	// write [OPTIONS] instead of OPTIONS
	bool optionaloptions(bool yes = true);
	// name of additional parameters, e.g. FILES
	void additionalparams(std::string name);

	std::ostream & dumpOn( std::ostream & str ) const;

    private:
	typedef std::list<HelpScreenParameter> ParamList;
	ParamList _parameters;
	std::string _commandname;
	std::string _nameofparams;
	bool _optionaloptions;
};

std::ostream& operator<<( std::ostream& str, const HelpScreen& h );

#endif
