#include <fstream>

#include <y2util/Y2SLog.h>
#include <y2util/PathInfo.h>
#include <Y2PM.h>
#include <y2pm/RpmDb.h>
#include <y2pm/InstTarget.h>

#include "variable.h"
#include "variables.h"

using namespace std;

#ifdef SUSE90COMPAT
#define set_log_filename Y2Logging::setLogfileName
#define get_log_filename Y2Logging::getLogfileName
#endif

VariableMap variables;
static map<string,string> vardesc;

static inline void printvar(ostream& os, string name, const Variable& v)
{
    os << name /*<< "[" << v.typestr() << "]" */ << " = " << v;
    if(vardesc.count(name))
	os << "\n\t" << vardesc[name];
    os << endl;
}

int varset(std::vector<std::string>& argv)
{
    if(argv.size()<2)
    {
	for(VariableMap::iterator it = variables.begin();
		it != variables.end(); ++it)
	{
	    printvar(cout,it->first,it->second);
	}
	return 0;
    }
    else if(argv.size()<3)
    {
	if(!variables.count(argv[1]))
	{
	    cout << argv[1] << " not set" << endl;
	    return 0;
	}
	else
	{
	    Variable& v = variables[argv[1]];
	    printvar(cout,argv[1],v);
	}
    }
    else
    {
	const char* msg = Variable::assign(variables[argv[1]], argv[2].c_str());
	if(msg)
	{
	    cout << "unable to set requested value, " << msg << endl;
	    return 1;
	}
    }

    return 0;
}

int varunset(vector<string>& argv)
{
    if(argv.size()<2)
    {
	cout << "unset <variable name>" << endl;
	return 1;
    }
    
    if(!variables.count(argv[1]))
    {
	    cout << argv[1] << " not set" << endl;
	    return 0;
    }

    if(!variables[argv[1]].canUnset())
    {
	cout << argv[1] << " is protected" << endl;
	return 1;
    }
	
    variables.erase(argv[1]);

    return 0;
}

static const char* setdebug(const Variable& v)
{
    if(!v.isBool()) return "must be bool";

    if(v.getBool())
    {
	cout << "debug enabled" << endl;
	Y2SLog::dbg_enabled_bm = true;
    }
    else
    {
	cout << "debug disabled" << endl;
	Y2SLog::dbg_enabled_bm = false;
    }

    return NULL;
}

static const char* setroot(const Variable& v)
{
    if(y2pmsh.initialized())
    {
	cout << "closeing insttarget ... " << flush;
	PMError err = Y2PM::instTargetClose();
	cout << err << endl;
	if(err)
	{
	    return "can't change root directory";
	}
    }

    if(!v.isString()) return "must be String";

    PathInfo p(v.getString());
    if(p.isExist() && !p.isDir()) return "not a directory";

    cout << "root dir set to " << v.getString() << endl;

    if(y2pmsh.initialized())
	y2pmsh.targetinit();

    return NULL;
}

static const char* timestat(const Variable& v)
{
    if(!v.isBool()) return "must be bool";

    cout << "time statistics " << (v.getBool()?"enabled":"disabled") << endl;

    return NULL;
}

static const char* logfilevalidate(const Variable& v)
{
    if(!v.isString()) return "must be String";
    set_log_filename(v.getString());
    return NULL;
}

static const char* pkginstflags2stings(const unsigned flags)
{
    string str;
    for(unsigned i = RpmDb::RPMINST_NOSCRIPTS;
	    i <= RpmDb::RPMINST_JUSTDB; i<<=1)
    {
	if(!(flags&i)) continue;
	if(i==RpmDb::RPMINST_NONE) continue;

	if(str.length()) str+='|';
	switch(i)
	{
	    case RpmDb::RPMINST_NODOCS:
		str+="NODOCS"; break;
	    case RpmDb::RPMINST_NOSCRIPTS:
		str+="NOSCRIPTS"; break;
	    case RpmDb::RPMINST_FORCE:
		str+="FORCE"; break;
	    case RpmDb::RPMINST_NODEPS:
		str+="NODEPS"; break;
	    case RpmDb::RPMINST_IGNORESIZE:
		str+="IGNORESIZE"; break;
	    case RpmDb::RPMINST_JUSTDB:
		str+="JUSTDB"; break;
	    case RpmDb::RPMINST_NONE: break;
	}
    }
    return str.c_str();
}

static bool string2pkginstflags(string str, unsigned &flags)
{
    vector<string> tokens;
    if(stringutil::split(str, tokens, "|") < 1)
    {
	cout << "invalid input" << endl;
	return false;
    }

    for(vector<string>::iterator it = tokens.begin();
	    it != tokens.end(); ++it)
    {
	if(*it == "NODOCS")
	    flags |= RpmDb::RPMINST_NODOCS;
	else if(*it == "NOSCRIPTS")
	    flags |= RpmDb::RPMINST_NOSCRIPTS;
	else if(*it == "FORCE")
	    flags |= RpmDb::RPMINST_FORCE;
	else if(*it == "NODEPS")
	    flags |= RpmDb::RPMINST_NODEPS;
	else if(*it == "IGNORESIZE")
	    flags |= RpmDb::RPMINST_IGNORESIZE;
	else if(*it == "JUSTDB")
	    flags |= RpmDb::RPMINST_JUSTDB;
	else
	    cout << "invalid flag: " << *it << endl;
    }

    return true;
}

static const char* instflags(const Variable& v)
{
    unsigned flags = 0;
    if(!v.isString()) return "must be String";
    if(!string2pkginstflags(v.getString(), flags))
	return NULL;

    Y2PM::instTarget().setPkgInstFlags(flags);

    cout << "flags set to " << pkginstflags2stings(Y2PM::instTarget().getPkgInstFlags()) << endl;

    return NULL;
}

#if 0 // tbd
static const char* archlist2stings(const std::list<PkgArch>& archs)
{
    std::stringstream str;
    for(std::list<PkgArch>::const_iterator cit = archs.begin();
	    cit != archs.end(); ++cit)
    {
	if(cit != archs.begin())
	    str << ' ';
	str << *cit;
    }
    return str.str().c_str();
}

static bool string2archlist(string str, std::list<PkgArch>& archs)
{
    vector<string> tokens;
    if(RpmDb::tokenize(str, ' ', 0, tokens) < 1)
    {
	cout << "invalid input" << endl;
	return false;
    }

    archs.erase(archs.begin(), archs.end());

    for(vector<string>::iterator it = tokens.begin();
	    it != tokens.end(); ++it)
    {
	archs.push_back(PkgArch(*it));
    }

    return true;
}

static const char* allowedarchs(const Variable& v)
{
    std::list<PkgArch> archs;
    if(!v.isString()) return "must be String";
    if(!string2archlist(v.getString(), archs))
	return NULL;

    Y2PM::setAllowedArchs(archs);

    return NULL;
}
#endif

static const char* instlogvalidate(const Variable& v)
{
    if(!v.isString()) return "must be String";
    if(!Y2PM::instTarget().setInstallationLogfile(v.getString()))
	return "logfile cannot be opened";

    return NULL;
}

static const char* createbackups(const Variable& v)
{
    if(!v.isBool()) return "must be Bool";

    if(y2pmsh.initialized())
	Y2PM::instTarget().createPackageBackups(v.getBool());

    cout << "create backups " << (v.getBool()?"enabled":"disabled") << endl;
    return NULL;
}

static const char* cachetoramdisk(const Variable& v)
{
    if(!v.isBool()) return "must be Bool";

    if(v.getBool() == false)
    {
	Y2PM::setCacheToRamdisk(false);
	Y2PM::setNotRunningFromSystem();
    }

    cout << "cache to ramdisk " << (Y2PM::cacheToRamdisk()?"enabled":"disabled") << endl;
    cout << "running from system " << (Y2PM::runningFromSystem()?"yes":"no") << endl;
    if(v.getBool() == false)
    {
	cout << "ATTENTION: this is a one way ticket. Resetting this to true has not effect!" << endl;
    }
    return NULL;
}

void init_variables()
{
    variables["debug"] = Variable("0",false,setdebug);
    vardesc["debug"] = "whether to enable debug messages";
    variables["verbose"] = Variable("0",false);
    vardesc["verbose"] = "certain commands display more info if >0, >1 etc.";
    variables["root"] = Variable("/",false,setroot);
    vardesc["root"] = "set root directory for operation";
    variables["autosource"] = Variable("1",false);
    vardesc["autosource"] = "automatically enable installation sources on init";
    variables["timestat"] = Variable("0",false,timestat);
    vardesc["timestat"] = "measure how long commands take";
    variables["quitoncommit"] = Variable("0",false);
    vardesc["quitoncommit"] = "quit after packages are commited";
    variables["quitonfail"] = Variable("0",false);
    vardesc["quitonfail"] = "quit if a command failed";

    variables["logfile"] = Variable(get_log_filename().c_str(),false,logfilevalidate);
    vardesc["logfile"] = "set log file";

    variables["instlog"] = Variable("",false,instlogvalidate);
    vardesc["instlog"] = "set installation log file";

    variables["createbackups"] = Variable("0",false,createbackups);
    vardesc["createbackups"] = "create package backups during installation";

    variables["pkginstflags"] = Variable(
	    pkginstflags2stings(Y2PM::instTarget().getPkgInstFlags()),false,instflags);
    vardesc["pkginstflags"] = "rpm parameters used for installation";

    variables["cachetodisk"] =  Variable("1",false,cachetoramdisk);
    vardesc["cachetodisk"] = "do not read instsources. do not save instsources to disk";
}

// vim:sw=4
