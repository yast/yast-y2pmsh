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
static inline void set_log_debug(bool f) {};
static inline bool get_log_debug() { return false; };
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

    set_log_debug(Y2SLog::dbg_enabled_bm);

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

    y2pmsh.setenv(v.getString() != "/");

    if(y2pmsh.initialized())
	y2pmsh.targetinit(v.getString());

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

#ifdef SUSE90COMPAT
#define LASTRPMFLAG RpmDb::RPMINST_JUSTDB
#else
#define LASTRPMFLAG RpmDb::RPMINST_NOSIGNATURE
#endif

static const char* pkginstflags2stings(const unsigned flags)
{
    string str;
    for(unsigned i = RpmDb::RPMINST_NOSCRIPTS;
	    i <= LASTRPMFLAG; i<<=1)
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
#ifndef SUSE90COMPAT
	    case RpmDb::RPMINST_NODIGEST:
		str+="NODIGEST"; break;
	    case RpmDb::RPMINST_NOSIGNATURE:
		str+="NOSIGNATURE"; break;
#endif
	    case RpmDb::RPMINST_NONE: break;
	}
    }
    return str.c_str();
}

#undef LASTRPMFLAG

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
#ifndef SUSE90COMPAT
	else if(*it == "NODIGEST")
	    flags |= RpmDb::RPMINST_NODIGEST;
	else if(*it == "NOSIGNATURE")
	    flags |= RpmDb::RPMINST_NOSIGNATURE;
#endif
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

static const char* runningfromsystem(const Variable& v)
{
    if(!v.isBool()) return "must be Bool";

    if(v.getBool() == false)
    {
	Y2PM::setNotRunningFromSystem();
    }

    cout << "running from system " << (Y2PM::runningFromSystem()?"yes":"no") << endl;
    if(v.getBool() == false)
    {
	cout << "ATTENTION: this is a one way ticket. Resetting this to true has not effect!" << endl;
    }
    return NULL;
}

void init_variables()
{
    // this also initializes logging
    variables["logfile"] = Variable(get_log_filename().c_str(),false,logfilevalidate);
    vardesc["logfile"] = "set log file";

    variables["debug"] = Variable((get_log_debug()?"1":"0"),false,setdebug);
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

    variables["instlog"] = Variable("",false,instlogvalidate);
    vardesc["instlog"] = "set installation log file";

    variables["createbackups"] = Variable("0",false,createbackups);
    vardesc["createbackups"] = "create package backups during installation";

    variables["pkginstflags"] = Variable(
	    pkginstflags2stings(Y2PM::instTarget().getPkgInstFlags()),false,instflags);
    vardesc["pkginstflags"] = "rpm parameters used for installation";

    variables["runningfromsystem"] =  Variable("1",false,runningfromsystem);
    vardesc["runningfromsystem"] = "behave somewhat like an initial installation";

    variables["build::strictrequires"] = Variable("0",false);
    vardesc["build::strictrequires"] = "whether missing build requires are fatal";

    const char* msg = Variable::assign(variables["instlog"], "/var/log/YaST2/y2logRPM");
    if(msg)
	cout << "Err: " << msg << endl;;
}

// vim:sw=4
