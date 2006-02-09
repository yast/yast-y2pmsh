#include "variable.h"

using namespace std;

Variable::Variable()
    : _type(t_bool), _boolval(false), _can_unset(true), _valid(NULL)
{
//    cout << __PRETTY_FUNCTION__ << ':' << __LINE__ << endl;
}

Variable::Variable(const Variable& v)
{
    _type = v._type;
    _valid = v._valid;
    _can_unset = v._can_unset;
    switch(_type)
    {
	case t_bool: _boolval = v._boolval; break;
	case t_int: _intval = v._intval; break;
	case t_string: _strval = strdup(v._strval); break;
    }
}

Variable& Variable::operator=( const Variable& v )
{
    _type = v._type;
    _valid = v._valid;
    _can_unset = v._can_unset;
    switch(_type)
    {
	case t_bool: _boolval = v._boolval; break;
	case t_int: _intval = v._intval; break;
	case t_string: _strval = strdup(v._strval); break;
    }

    return *this;
}

const char* Variable::assign(Variable& v, const char* value)
{
    Variable tmp(value,v._can_unset,v._valid);
    const char* msg = (v._valid?v._valid(tmp):NULL);
    if(msg)
	return msg;

    v=tmp;

    return NULL;
}

Variable::Variable(const char* value, bool can_unset, ValidateFunc valid)
{
    long val = 0;
    char* endptr;
    _can_unset = can_unset;
    _valid = valid;
    val = strtol(value,&endptr, 10);
    if(value != endptr)
    {
	if(val == 0 || val == 1)
	{
	    _type = t_bool;
	    _boolval = val;
	}
	else
	{
	    _type = t_int;
	    _intval = val;
	}
    }
    else if(!strcasecmp(value,"true") || !strcasecmp(value,"yes"))
    {
	_type = t_bool;
	_boolval = true;
    }
    else if(!strcasecmp(value,"false") || !strcasecmp(value,"no"))
    {
	_type = t_bool;
	_boolval = false;
    }
    else
    {
	_type = t_string;
	_strval = strdup(value);
    }
}

Variable::~Variable()
{
    if( _type == t_string && _strval )
    {
	free(_strval);
	_strval = NULL;
    }
}

void Variable::dumpOn(ostream& os) const
{
    switch(_type)
    {
	case t_bool: os << (_boolval?"true":"false") ; break;
	case t_int: os << _intval; break;
	case t_string: os << '"' << _strval << '"'; break;
    }
}

const char* Variable::typestr() const
{
    switch(_type)
    {
	case t_bool: return "bool";
	case t_int: return "int";
	case t_string: return "string";
    }
    return NULL;
}

bool Variable::isBool() const
{
    return ( _type == t_bool );
}

bool Variable::isInt() const
{
    return ( _type == t_int );
}

bool Variable::isString() const
{
    return ( _type == t_string );
}

long Variable::getInt() const
{
    switch(_type)
    {
	case t_bool:
	    return _boolval;
	case t_int:
	    return _intval;
	case t_string:
	    return 0;
    }
    return 0;
}

bool Variable::getBool() const
{
    switch(_type)
    {
	case t_bool:
	    return _boolval;
	case t_int:
	    return ( _intval != 0 );
	case t_string:
	    return 0;
    }
    return false;
}

const char* Variable::getString() const
{
    switch(_type)
    {
	case t_bool:
	    return (_boolval?"true":"false");
	case t_int:
	    return ""; //XXX
	case t_string:
	    return _strval;
    }
    return NULL;
}

bool Variable::canUnset() const
{
    return _can_unset;
}

ostream& operator<<(ostream& os, const Variable& v)
{
    v.dumpOn(os);
    return os;
}
