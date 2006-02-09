#ifndef VARIABLE_H
#define VARIABLE_H

#include <ostream>

class Variable
{
    public:
	enum Type { t_string=0, t_bool, t_int };

	// return NULL on sucess, error message otherwise
	typedef const char* (*ValidateFunc)(const Variable& v);

    private:
	Type _type;
	union {
	    char* _strval;
	    bool _boolval;
	    long _intval;
	};

	bool _can_unset;
	ValidateFunc _valid;
	
    public:
	Variable();
	Variable(const char* value, bool can_unset = true, ValidateFunc valid = NULL);
	Variable( const Variable& );
	~Variable();
	bool isBool() const;
	bool isInt() const;
	bool isString() const;
	long getInt() const;
	bool getBool() const;
	const char* getString() const;
	bool canUnset() const;
	void dumpOn(std::ostream& os) const;
	const char* typestr() const;

	// return NULL on sucess, error message otherwise
	static const char* assign(Variable& v, const char* value);

	// does not call validate function
	Variable& operator=( const Variable& );

};

std::ostream& operator<<(std::ostream& os, const Variable& v);

#endif
