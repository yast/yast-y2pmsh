#include "helpscreen.h"
#include <fstream>
#include <y2util/stringutil.h>

using namespace std;

HelpScreenParameter::HelpScreenParameter(string s, string l, string h)
{
    _s = s;
    _l = l;
    _h = h;
}

HelpScreen::HelpScreen(string commandname) : _optionaloptions(false)
{
    _commandname = commandname;
}

void HelpScreen::Parameter(const HelpScreenParameter& parameter)
{
    _parameters.insert(_parameters.end(),parameter);
}


bool HelpScreen::optionaloptions(bool yes)
{
    _optionaloptions = yes;
    return _optionaloptions;
}

void HelpScreen::additionalparams(std::string name)
{
    _nameofparams = name;
}

std::ostream& HelpScreen::dumpOn( std::ostream & str ) const
{
    unsigned maxshort = 0, maxlong = 0;
    if(_optionaloptions)
	str << stringutil::form("Usage: %s [OPTIONS]", _commandname.c_str());
    else
	str << stringutil::form("Usage: %s OPTIONS", _commandname.c_str());

    if(_nameofparams.length())
	str << " " << _nameofparams;

    str << endl;

    str << "  OPTIONS:" << endl;

    for(ParamList::const_iterator cit = _parameters.begin();
	cit != _parameters.end();
	++cit)
    {
	if((*cit).s().length() > maxshort) maxshort =(*cit).s().length();
	if((*cit).l().length() > maxlong) maxlong =(*cit).l().length();
    }

    unsigned maxlinelen = 2+maxshort+2+maxlong;
    maxlinelen+=8-maxlinelen%8;
    for(ParamList::const_iterator cit = _parameters.begin();
	cit != _parameters.end();
	++cit)
    {
	unsigned slen = cit->s().length();
	unsigned llen = cit->l().length();
	unsigned linelen = 3 + slen; // leading spaces + coma
	str << "  " << cit->s() << (slen?',':' ');
	for(unsigned i = 0; i < maxshort-slen+1; ++i)
	{
	    str << ' ';
	    ++linelen;
	}
	str << cit->l() << '\t';
	linelen+=llen;
	linelen+=8-linelen%8;

	for(unsigned i = 0; i < (maxlinelen-linelen)%8; ++i)
	    str << '\t';

	str << cit->h();

	str << endl;
    }

    return str;
}

std::ostream& operator<<( std::ostream& str, const HelpScreen& h )
{
    return h.dumpOn(str);
}

// vim:sw=4
