/*---------------------------------------------------------------------\
|                                                                      |
|                      __   __    ____ _____ ____                      |
|                      \ \ / /_ _/ ___|_   _|___ \                     |
|                       \ V / _` \___ \ | |   __) |                    |
|                        | | (_| |___) || |  / __/                     |
|                        |_|\__,_|____/ |_| |_____|                    |
|                                                                      |
|                               core system                            |
|                                                     (C) 2002 SuSE AG |
\----------------------------------------------------------------------/

   File:       testinstall.cc
   Purpose:    test dependency solver
   Author:     Ludwig Nussel <lnussel@suse.de>
   Maintainer: Ludwig Nussel <lnussel@suse.de>

/-*/

#undef Y2SLOG
#define Y2SLOG "y2pmsh"

#include <cstdlib> //atoi
#include <fstream>
#include <sstream>

#include <y2util/timeclass.h>
#include <y2util/stringutil.h>
#include <y2util/Y2SLog.h>
#include <Y2PM.h>
#include <y2pm/InstallOrder.h>
#include <y2pm/RpmDb.h>
#include <y2pm/PkgDep.h>
#include <y2pm/PMPackage.h>
#include <y2pm/InstTarget.h>
#include <y2pm/InstTargetError.h>
#include <y2pm/PMPackageManager.h>
#include <y2pm/PMSelectionManager.h>
#include <y2pm/InstSrcDescr.h>
#include <y2pm/RpmDbCallbacks.h>
#include <y2pm/PkgArch.h>

#include <stdint.h>
#include <malloc.h>
#include <locale.h>

#include "variable.h"
#include "variables.h"
#include "y2pmsh.h"
#include "helpscreen.h"
#include "solverresults.h"
#include "build.h"
#include "instsrc.h"
#include "search.h"
#include "summary.h"

using namespace std;

static bool _keep_running = true;

static vector<string> nullvector;

int install(vector<string>& argv);
int consistent(vector<string>& argv);
int help(vector<string>& argv);
int enablesources(vector<string>& argv);
int show(vector<string>& argv);
int remove(vector<string>& argv);
int rpminstall(vector<string>& argv);
int instlog(vector<string>& argv);
int deselect(vector<string>& argv);
int solve(vector<string>& argv);
int rebuilddb(vector<string>& argv);
int df(vector<string>& argv);
int selshow(vector<string>& argv);
int setsel(vector<string>& argv);
int delsel(vector<string>& argv);
int solvesel(vector<string>& argv);
int setappl(vector<string>& argv);
int order(vector<string>& argv);
int upgrade(vector<string>& argv);
int commit(vector<string>& argv);
int summary(vector<string>& argv);
int mem(vector<string>& argv);

int testset(vector<string>& argv);
int testmediaorder(vector<string>& argv);

int cdattach(vector<string>& argv);

int products(vector<string>& argv);

Y2PMSH y2pmsh;

inline int init(vector<string>& argv)
{
    return y2pmsh.init(argv);
}

static ostream& operator<<( ostream& os, const struct mallinfo& i )
{
    os << "Memory from system: " << FSize(i.arena) << ", used: " << FSize(i.uordblks);
    return os;
}

static int solveandcommit(int ret)
{
    if(ret == 0 && !y2pmsh.shellmode())
    {
	vector<string> nothing;
	ret = solve(nothing);

	if(ret == 0)
	    ret = commit(nothing);
    }

    return ret;
}

void progresscallback(const ProgressData & prg, int& lastprogress)
{
    int p = (int)((prg.max()-prg.min())/1.0*prg.val());
    if(p<0) p = 0;
    if(p>100) p = 100;

    if(p==0) lastprogress = 0;

    int num = (long)60*p/100;
    for(int i=0; i < num-lastprogress; i++)
    {
	cout << ".";
    }
    cout.flush();
    lastprogress = num;
//    if(p == 100) cout << endl;
}

class ConvertDbCallback : public RpmDbCallbacks::ConvertDbCallback
{
    int lastprogress;
    int _failed;
    int _ignored;
    int _alreadyInV4;
    virtual void start( const Pathname & v3db )
    {
	lastprogress = 0;
	_failed = _ignored = _alreadyInV4 = 0;
	cout << "converting database " << v3db << " ";
    }
    virtual void progress( const ProgressData & prg,
			   unsigned failed, unsigned ignored, unsigned alreadyInV4 )
    {
	progresscallback(prg, lastprogress);
	_failed = failed;
	_ignored = ignored;
	_alreadyInV4 = alreadyInV4;
    }
    virtual void dbInV4( const std::string & pkg )
    {
	cout << endl << "package "<< pkg << " already in V4 Database" << endl;
    }
    /**
     * PROCEED: Continue to see if more errors occur, but discard new db.
     * SKIP:    Ignore error and continue.
     * CANCEL:  Stop conversion and discard new db.
     **/
    virtual CBSuggest dbReadError( int offset )
    {
	return CBSuggest::PROCEED;
    }
    virtual void dbWriteError( const std::string & pkg )
    {
	cout << endl << "write error at package " << pkg << endl;
    }
    virtual void stop( PMError error )
    {
	if(error != PMError::E_ok)
	    cout << error << endl;
	else
	    cout << " ok" << endl;

	cout << _failed << " failed" << endl;
	cout << _ignored << " ignored" << endl;
	cout << _alreadyInV4 << " already in V4 format" << endl;
    };
};

class InstallPkgCallback : public RpmDbCallbacks::InstallPkgCallback
{
    int lastprogress;
    virtual void start( const Pathname & filename )
    {
	lastprogress = 0;
	cout << "installing " << filename.basename() << " ";
    };
    virtual void progress( const ProgressData & prg )
    {
	progresscallback(prg, lastprogress);
    };
    virtual void stop( PMError error )
    {
	if(error != PMError::E_ok)
	    cout << "## ERROR ##" << error << endl;
	else
	    cout << " ok" << endl;
    };
};

class RemovePkgCallback : public RpmDbCallbacks::RemovePkgCallback
{
    int lastprogress;
    virtual void start( const std::string & label )
    {
	lastprogress = 0;
	cout << "removing " << label << " ";
    };
    virtual void progress( const ProgressData & prg )
    {
	progresscallback(prg, lastprogress);
    };
    virtual void stop( PMError error )
    {
	if(error != PMError::E_ok)
	    cout << error << endl;
	else
	    cout << " ok" << endl;
    };
};

class RebuildDbCallback : public RpmDbCallbacks::RebuildDbCallback
{
    int lastprogress;
    virtual void start()
    {
	lastprogress = 0;
	cout << "rebuilding rpm database ";
    };
    virtual void progress( const ProgressData & prg )
    {
	progresscallback(prg, lastprogress);
    };
    virtual void stop( PMError error )
    {
	if(error != PMError::E_ok)
	    cout << error << endl;
	else
	    cout << " ok" << endl;
    };
#ifndef SUSE90COMPAT
    virtual void notify( const string& msg )
    {
	cout << msg << endl;
    }
#endif
};

static InstallPkgCallback installpkgcallback;
static RebuildDbCallback rebuilddbcallback;
static ConvertDbCallback convertdbcallback;
static RemovePkgCallback removepkgcallback;

#if 0
void providestartcallback(const std::string& name, const FSize& s, bool, void*)
{
    cout << stringutil::form("Downloading %s (%s)",name.c_str(), s.asString().c_str()) << endl;
}

std::string providedonecallback(PMError error, const std::string& reason, const std::string&, void*)
{
    if(lastprogress != 100)
	progresscallback(100,NULL);
    lastprogress = 0;

    if(error)
    {
	cout << error << ": " << reason << endl;
	return "I";
    }
    else
    {
	cout << "ok" << endl;
	return "";
    }
}

std::string packagedonecallback(PMError error, const std::string& reason, void*)
{
    return providedonecallback(error,reason,"",NULL);
}

bool pkgstartcallback(const std::string& name, const std::string& summary, const FSize& size, bool is_delete, void*)
{
    if(is_delete)
    {
	cout << stringutil::form("Deleting %s",name.c_str()) << endl;
    }
    else
    {
	cout << stringutil::form("Installing %s (%s) - %s ",name.c_str(),size.asString().c_str(),summary.c_str()) << endl;
    }
    return true;
}

static void sourcechangecallback(InstSrcManager::ISrcId srcid, int medianr, void*)
{
    cout << "switching to source " << srcid << ", Media Nr. " << medianr << endl;
}
#endif

class PackageCompleter : public CmdLineIface::Completer
{
    private:
	string _name;
    public:
	PackageCompleter(const string name) : _name(name) {};
	virtual const std::string command() { return _name; };
	virtual std::vector<std::string> completions(const std::string word)
	{
	    vector<string> v;
	    if(!y2pmsh.initialized())
		return v;

	    PMManager::PMSelectableVec::iterator it;

	    for (it = Y2PM::packageManager().begin();
		it != Y2PM::packageManager().end(); ++it)
	    {
		string name = (*it)->name();
		if(name.substr(0, word.length()) == word)
		{
		    v.push_back(name);
		}
	    }

	    return v;
	}
};

void init_commands()
{
#define newcmd(a,b,c,d) y2pmsh.cmd.add(new Command(a,b,c,d))
#define newpkgcmd(a,b,c,d) newcmd(a,b,c,d); y2pmsh.cli().registerCompleter(new PackageCompleter(a));
// flags: 1 = need init, 2 = hidden, 4 = advanced
    newpkgcmd("install",	install, 1, "select packages for installation");
    newpkgcmd("deselect",	deselect, 1, "deselect packages marked for installation/removal");
    newpkgcmd("remove",	remove, 1, "select package for removal");
    newcmd("solve",	solve, 1, "solve dependencies");
    newpkgcmd("state",	state, 1, "show state of package(s)");
    newcmd("newer",	newer, 1, "show packages with newer candiate available");
    newcmd("rpminstall",	rpminstall, 3, "install rpm files");
    newcmd("consistent",	consistent, 3, "check consistency");
    newcmd("set",		varset, 0, "set or show variable");
    newcmd("unset",	varunset, 0, "unset variable");
    newpkgcmd("show",	show, 1, "show package info");
    newcmd("search",	search, 1, "search for packages");
    newcmd("whatprovides",	whatprovides, 1, "search for package provides");
    newcmd("whatrequires",	whatrequires, 1, "search for package requirement");
    newpkgcmd("whatdependson",	whatdependson, 1, "search for depending packages");
    newpkgcmd("depends",	depends, 1, "search for depending packages");
    newcmd("alternatives",	alternatives, 5, "search for depending packages");
//    newcmd("query",     query, 5, "query packagemanager");
    newcmd("source",	source, 0, "manage installation sources");
    newcmd("enablesources", enablesources, 3, "enable all sources");
    newcmd("buildsolve",	buildsolve, 3, "solve dependencies like the build script");
    newcmd("rebuilddb",	rebuilddb, 3, "rebuild rpm db");
    newcmd("df",		df, 1, "display disk space forecast");
    newcmd("selstate",	selstate, 1,  "show state of selection");
    newcmd("setappl",	setappl, 5, "set package to installed like a selection would do");
    newcmd("order",	order, 3, "compute installation order");
    newcmd("upgrade",	upgrade, 1, "compute upgrade");
    newcmd("summary",	summary, 1, "display summary about what would be done on commit");
    newcmd("commit",	commit, 1, "commit changes. actually performs installation");
    newcmd("selshow",	selshow, 1, "show selection info");
    newcmd("selinstall",	setsel, 1, "mark selection for installation, need to call solvesel");
    newcmd("selremove",	delsel, 1, "mark selection for removal, need to call solvesel");
    newcmd("selsolve",	solvesel, 1, "solve selection dependencies and apply state to packages");
    newcmd("cdattach",	cdattach, 2, "cdattach");
    newcmd("mem",		mem, 2, "memory statistics");
    newcmd("testset",	testset, 2, "test memory consumption of PkgSet");
    newcmd("testmediaorder", testmediaorder, 3, "test media order");
    newcmd("init",	init, 1, "initialize packagemanager (happens automatically if needed)");
    newcmd("help",	help, 0, "this screen");
    newcmd("products", products, 1, "show installed products");
#undef newpkgcmd
#undef newcmd
}

int df(vector<string>& argv)
{
    const PkgDuMaster &dum = Y2PM::packageManager().updateDu();
    const set<PkgDuMaster::MountPoint> &mp = dum.mountpoints();

    cout << " Available      Used    Needed      Free  Mountpoint" << endl;
    for(set<PkgDuMaster::MountPoint>::iterator it = mp.begin();
	    it != mp.end(); ++it)
    {
	cout.width(10);
	cout << it->total();
	cout.width(10);
	cout << it->initial_used();
	cout.width(10);
	cout << it->pkg_diff();
	cout.width(10);
	cout << it->pkg_available();
	cout << "  ";
	cout << it->mountpoint() << endl;
    }
    return 0;
}

int rebuilddb(vector<string>& argv)
{
    cout << "rebuilding database ... " << endl;

    PMError err = Y2PM::instTarget().bringIntoCleanState();
    if(err != PMError::E_ok)
    {
	cout << "failed: " << err << endl;
	return 1;
    }
    else
	cout << "done" << endl;

    return 0;
}

/********************/

static void mediareleasefunc()
{
    PMError err = Y2PM::instSrcManager().releaseAllMedia();
    if(err)
	cerr << "Failed to release media: " << err << endl;
}

bool Y2PMSH::ReleaseMediaAtExit()
{
    if(!_releasemediainstalled)
    {
	if(!atexit(mediareleasefunc))
	{
	    _releasemediainstalled = true;
	}
    }

    return _releasemediainstalled;
}

int help(vector<string>& argv)
{
    unsigned filtermask = 6;
    if(argv.size()>1)
    {
	if(argv[1]=="all")
	    filtermask = 0;
	else if(argv[1]=="hidden")
	    filtermask = 4;
	else if(argv[1]=="advanced")
	    filtermask = 2;
    }

    unsigned max = 0;

    for(unsigned c = 0; c < 2; ++c)
    {
	for(Commands::const_iterator cit = y2pmsh.cmd.begin();
		cit != y2pmsh.cmd.end(); ++cit)
	{
	    if(cit->second->flags() && cit->second->flags()&filtermask) continue;

	    size_t len = cit->second->name().length();

	    if(c)
	    {
		for(unsigned space = 0; space < max - len; ++space)
		    cout << ' ';

		cout << cit->second->name();
		cout << "  ";
		cout << cit->second->help() << endl;
	    }
	    else
	    {
		if(len > max) max = len;
	    }
	}
    }

    return 0;
}

static int install_internal(PMManager& manager, vector<string>& argv, bool appl=false)
{
    PMManager::PMSelectableVec::iterator begin, end;
    PMManager::PMSelectableVec vec;
    int failed = 0;

    if(argv.size()>1 && argv[1]=="-a")
    {
	begin = Y2PM::packageManager().begin();
	end = Y2PM::packageManager().end();
    }
    else
    {
	for (vector<string>::iterator it = ++argv.begin();
		it != argv.end(); ++it)
	{
	    string pkg = stringutil::trim(*it);

	    if(pkg.empty()) continue;

	    PMSelectablePtr selp = manager.getItem(pkg);
	    if(!selp || !selp->has_candidate())
	    {
		failed = 1;
		std::cout << "package " << pkg << " is not available.\n";
		continue;
	    }
	    vec.insert(selp);
	}
	begin=vec.begin();
	end=vec.end();
    }

    for(PMManager::PMSelectableVec::iterator it=begin; it!=end;++it)
    {
	bool ok;

	if(appl)
	    ok = (*it)->appl_set_install();
	else
	    ok = (*it)->user_set_install();

	if(!ok)
	{
	    cout << stringutil::form("could not mark %s for installation/update",
		    (*it)->theObject()->name()->c_str());
	    failed = 2;
	}
    }

    return failed;
}

int install(vector<string>& argv)
{
    int ret = install_internal(Y2PM::packageManager(), argv);

    return solveandcommit(ret);
}

int setappl(vector<string>& argv)
{
    return install_internal(Y2PM::packageManager(), argv, true);
}

int setsel(vector<string>& argv)
{
    return install_internal(Y2PM::selectionManager(), argv);
}

static int remove_internal(vector<string>& argv, PMManager& manager)
{
    int failed = 0;
    for (unsigned i=1; i < argv.size() ; i++)
    {
	string name = stringutil::trim(argv[i]);

	if(name.empty()) continue;

	PMSelectablePtr selp = manager.getItem(name);
	if(!selp)
	{
	    std::cout << name << " is not available.\n";
	    failed = 1;
	    continue;
	}

	if(!selp->user_set_offSystem())
	{
	    ++failed;
	    cout << stringutil::form("coult not mark %s for deletion", name.c_str()) << endl;
	}
    }
    return failed;
}

int remove(vector<string>& argv)
{
    int ret = remove_internal(argv, Y2PM::packageManager());

    return solveandcommit(ret);
}

int delsel(vector<string>& argv)
{
    return remove_internal(argv, Y2PM::selectionManager());
}

static bool solve_internal(PMManager& manager, vector<string>& argv)
{
    int numinst=0,numbad=0;
    bool success = false;
    bool filter = false;

    for(vector<string>::iterator it= argv.begin(); it != argv.end();++it)
    {
	if(*it == "-u")
	{
	    filter = true;
	    cout << "filtering" << endl;
	}
    }

    PkgDep::ResultList good;
    PkgDep::ErrorResultList bad;

    success = manager.solveInstall(good, bad, filter);


    numbad = printbadlist(bad);
    numinst = printgoodlist(good);

    cout << "***" << endl;
    cout << numbad << " bad, " << numinst << " to install" << endl;

    return success;
}

int solve(vector<string>& argv)
{
    bool ok = solve_internal(Y2PM::packageManager(), argv);
    return ok?0:1;
}

int solvesel(vector<string>& argv)
{
    bool ok = solve_internal(Y2PM::selectionManager(), argv);
    if(ok)
    {
	cout << "solve ok, activation selection" << endl;
	Y2PM::selectionManager().activate(Y2PM::packageManager());
    }
    return ok?0:1;
}

int order(vector<string>& argv)
{
    PkgSet toinstall;
    PkgSet installed;
    for (PMManager::PMSelectableVec::iterator it = Y2PM::packageManager().begin();
	it != Y2PM::packageManager().end(); ++it )
    {
	if((*it)->has_installed())
	{
	    installed.add((*it)->installedObj());
	}
	if((*it)->to_install())
	{
	    toinstall.add((*it)->candidateObj());
	}
    }

    InstallOrder order(toinstall,installed);

    InstallOrder::SolvableList nowinstall;

    unsigned count=0;

    cout << "Installation order:" << endl;
    for(nowinstall = order.computeNextSet(); !nowinstall.empty(); nowinstall = order.computeNextSet())
    {
	count++;
	for(InstallOrder::SolvableList::const_iterator cit = nowinstall.begin();
	    cit != nowinstall.end(); ++cit)
	{
	    cout << (*cit)->name() << " ";
	}
	cout << endl;
	{
	    string filename=stringutil::form("/tmp/graph.run%d",count);
	    ofstream fs(filename.c_str());
	    if(fs)
	    {
		order.printAdj(fs,false);
		fs.close();
	    }
	    else
	    {
		cout << "unable to open " << filename << endl;
	    }
	}
	{
	    string filename=stringutil::form("/tmp/rgraph.run%d",count);
	    ofstream fs(filename.c_str());
	    if(fs)
	    {
		order.printAdj(fs,true);
		fs.close();
	    }
	    else
	    {
		cout << "unable to open " << filename << endl;
	    }
	}
	order.setInstalled(nowinstall);
    }

    return 0;
}

int upgrade(vector<string>& argv)
{
    PMUpdateStats stats;
    stats.delete_unmaintained = false;

    for(vector<string>::iterator it= argv.begin(); it != argv.end();++it)
    {
	if(*it == "-u")
	{
	    stats.delete_unmaintained = true;
	    cout << "delete unmaintained" << endl;
	}
    }

    Y2PM::packageManager().doUpdate(stats);
    cout << stats << endl;
    return 0;
}

int commit(vector<string>& argv)
{
    std::list<std::string> errors_r;
    std::list<std::string> remaining_r;
    std::list<std::string> srcremaining_r;

    Pathname etc(variables["root"].getString());
    etc+="/etc";
    PathInfo::assert_dir(etc);

#if 0
    PathInfo passwd(etc+"passwd");
    if(!passwd.isExist())
    {
	ofstream os;
	os.open(passwd.asString().c_str());
	os << "root:x:0:0:root:/root:/bin/bash" << endl;
	os.close();
    }

    passwd = etc+"group";
    if(!passwd.isExist())
    {
	ofstream os;
	os.open(passwd.asString().c_str());
	os << "root:x:0:root" << endl;
	os.close();
    }
#endif

    Y2PM::commitPackages (0,errors_r, remaining_r, srcremaining_r);

    int failed = 0;
    if(!remaining_r.empty())
    {
	failed = 1;
	cout << "failed packages:" << endl;
	for(list<string>::iterator it=remaining_r.begin(); it!=remaining_r.end();++it)
	{
	    cout << *it << endl;
	}
    }

    if(variables["quitoncommit"].getBool())
    {
	_keep_running = false;
    }
    else
    {
	PMError err;
	cout << "rereading installed packages ... " << flush;
	err = Y2PM::instTargetUpdate();
	cout << err << endl;
	if(err)
	    cout << endl << "System is in an undefined state now, please quit" << endl;
    }

    return failed;
}

int testmediaorder(vector<string>& argv)
{
    std::list<PMPackagePtr> dellist;
    std::list<PMPackagePtr> inslist;
    std::list<PMPackagePtr> srclist;

    Y2PM::packageManager().getPackagesToInsDel (dellist, inslist, srclist);

    unsigned int current_src_media = 0;
    constInstSrcPtr current_src_ptr = 0;

    for (std::list<PMPackagePtr>::iterator it = inslist.begin();
	it != inslist.end(); ++it)
    {
	unsigned int pkgmedianr = 0;
	pkgmedianr = (*it)->medianr();

	if (((*it)->source() != current_src_ptr)	// source or media change
	    || (pkgmedianr != current_src_media))
	{
	    constInstSrcDescrPtr descr = (*it)->source()->descr();
	    if(current_src_ptr!=0)
		cout << endl;
	    cout << "Installing from " << descr->content_label()
		<< " (" << descr->url() << ")"
		<< " Media Nr. " << pkgmedianr << ":";

	    current_src_ptr = (*it)->source();
	    current_src_media = pkgmedianr;

	}

	cout << ' ' << (*it)->name();
    }
    cout << endl;

    return 0;
}

int consistent(vector<string>& argv)
{
    int numbad=0;
    bool success = false;

    PkgDep::ErrorResultList bad;

    success = Y2PM::packageManager().solveConsistent(bad);

    numbad = printbadlist(bad);

    cout << "***" << endl;
    cout << numbad << " bad" << endl;

    return success?0:1;
}

int deselect(vector<string>& argv)
{
    int failed = 0;
    for (unsigned i=1; i < argv.size() ; i++)
    {
	string pkg = stringutil::trim(argv[i]);

	if(pkg.empty()) continue;

	PMSelectablePtr selp = Y2PM::packageManager().getItem(pkg);
	if(!selp)
	{
	    std::cout << "package " << pkg << " is not available.\n";
	    failed = 1;
	    continue;
	}

	selp->setNothingSelected();
    }

    return failed;
}

int rpminstall(vector<string>& argv)
{
    vector<string>::iterator it=argv.begin();
    list<Pathname> pkgs;
    ++it; // first one is function name itself

    for(;it!=argv.end();++it)
    {
	pkgs.push_back (Pathname (*it));
    }

    if(Y2PM::instTargetInit(variables["root"].getString()) != InstTargetError::E_ok)
    {
	cout << "initialization failed" << endl;
	return 1;
    }
    else
	Y2PM::instTarget().installPackages(pkgs);

    return 0;
}

int mem(vector<string>& argv)
{
    cout << mallinfo() << endl;
    return 0;
}

int cdattach(vector<string>& argv)
{
    MediaAccessPtr media = new MediaAccess;

    Url mediaurl_r = (string("cd:///;") + argv[1]);


    PMError err;
    if ( (err = media->open( mediaurl_r, "/tmp/blub" )) )
    {
	ERR << "Failed to open " << mediaurl_r << " " << err << endl;
	return 1;
    }

    DBG << "open ok" << endl;
    bool repeat = true;
    bool notfirst =false;
    do
    {
	if(media->isAttached())
	{
	    DBG << "release medium" << endl;
	    cout << media->release() << endl;
	}
	DBG << "attach medium" << endl;
	if ( (err = media->attach(notfirst)) )
	{
	    ERR << "Failed to attach media: " << err << endl;
	    repeat = false;
	    return 1;
	}
	notfirst = true;
    } while (repeat);

    if(media->isAttached())
    {
	DBG << "release medium final" << endl;
	media->release();
    }

    return 0;
}

int products( vector<string>& )
{
    const std::list<constInstSrcDescrPtr> &products = Y2PM::instTarget().getProducts();

    int i = 0;    
    std::list<constInstSrcDescrPtr>::const_iterator it;
    for( it = products.begin(); it != products.end(); ++it ) {
        cout << i++ << ": " << (*it)->content_product();
        std::string youurl = (*it)->content_youurl();
        if ( !youurl.empty() ) cout << ", YouUrl: " << youurl;
        std::string youpath = (*it)->content_youpath();
        if ( !youpath.empty() ) cout << ", YouPath: " << youpath;
        std::string youtype = (*it)->content_youtype();
        if ( !youtype.empty() ) cout << ", YouType: \"" << youtype << "\"";
        cout << endl;        
    }

    return 0;
}

int main( int argc, char *argv[] )
{
    bool cliok = true;

    int mainret = 0;

    if(argc > 1)
	y2pmsh.shellmode(false);

    RpmDbCallbacks::installPkgReport.redirectTo(installpkgcallback);
    RpmDbCallbacks::rebuildDbReport.redirectTo(rebuilddbcallback);
    RpmDbCallbacks::convertDbReport.redirectTo(convertdbcallback);
    RpmDbCallbacks::removePkgReport.redirectTo(removepkgcallback);

    init_variables();
    
    init_commands();

    y2pmsh.setenv();

    setlocale(LC_CTYPE, "");

    cout << "Welcome to the YaST2 Package Manager!" << endl;
    cout << "This tool is meant for debugging purpose only." << endl << endl;

    if(y2pmsh.shellmode() && y2pmsh.interactive())
    {
	cout << "type help for help, ^D to exit" << endl << endl;
    }

    // TODO: write real command line parser
    while( cliok && _keep_running)
    {
	vector<vector<string> > cmds;

	if(y2pmsh.shellmode())
	{
	    string inputstr;

	    y2pmsh.cli().lastRetCode(mainret);

	    cliok = y2pmsh.cli().getLine(inputstr);

	    if(!cliok) break;

	    inputstr = stringutil::trim(inputstr);

	    if(inputstr.empty())
		continue;

	    y2pmsh.cli().addToHistory(inputstr);

	    vector<string> tmp;
/*
	    if(stringutil::split(inputstr, tmp, ";") < 1)
	    {
		cout << "invalid input: " << inputstr << endl;
		continue;
	    }

	    for(vector<string>::iterator it = tmp.begin();
		it != tmp.end(); ++it)
	    {
		string cmd = stringutil::trim(*it);
		if(cmd.empty())
		    continue;

		vector<string> argv;
		if(stringutil::split(cmd, argv, " \t\n") < 1)
		{
		    cout << "invalid input: " << cmd << endl;
		    continue;
		}
		cmds.push_back(argv);
	    }
*/
	    y2pmsh.cli().parsewords(inputstr, tmp);
	    cmds.push_back(tmp);
	}
	else
	{
	    vector<string> tmp;
	    for(int i = 1; i < argc; ++i)
	    {
		tmp.push_back(argv[i]);
	    }
	    cmds.push_back(tmp);
	    cliok = false;

//	    Variable::assign(variables["quitoncommit"], "true");
	}

	for(vector<vector<string> >::iterator vit = cmds.begin();
		vit != cmds.end() && _keep_running; ++vit)
	{
	    vector<string> argv = *vit;
	    const Command* cmdptr = NULL;

	    cmdptr = y2pmsh.cmd[argv[0]];

	    if(cmdptr)
	    {
		TimeClass t;
		bool showtimes = variables["timestat"].getBool();
		if((cmdptr->flags()&1) && !y2pmsh.initialized() && cmdptr->func() != init)
		{
		    if(showtimes) t.startTimer();
		    mainret = y2pmsh.init(nullvector);
		    if(showtimes) t.stopTimer();
		    if(showtimes) cout << "time: " << t << endl;
		}

		if(showtimes) t.startTimer();
		mainret = cmdptr->run(argv);
		if(showtimes) t.stopTimer();
		if(showtimes) cout << "time: " << t << endl;
		if(mainret)
		{
		    if(!y2pmsh.shellmode()) // switch to interactive on error
		    {
			y2pmsh.shellmode(true);
			cliok = true;
		    }
		    else if(variables["quitonfail"].getBool())
		    {
			_keep_running = false;
		    }
		}
	    }
	    else
	    {
		cout << "unknown function " << argv[0] << endl;
		mainret = 1;
	    }
	}
    }

    cout << endl;

    if(y2pmsh.shellmode())
	y2pmsh.cli().saveHistory();

    return mainret;
}

int testset(vector<string>& argv)
{
    int i = 0;
    cout << mallinfo() << endl;
    PkgSet* testset = new PkgSet;
    cout << mallinfo() << endl;

    if(!testset)
    {
	ERR << "out of memory" << endl;
	exit(EXIT_FAILURE);
    }

    PMManager::PMSelectableVec::const_iterator it, end;
    it = Y2PM::packageManager().begin();
    end = Y2PM::packageManager().end();
    for (;it!=end;++it)
    {
	PMSelectablePtr selp = *it;
	if(!selp)
	{
	    std::cerr << "invalid pointer" << endl;
	    continue;
	}

	PMSolvablePtr solp;
	if(selp->has_installed())
	{
	    solp = selp->installedObj();
	}
	else if(selp->has_candidate())
	{
	    solp = selp->candidateObj();
	}
	if(!solp)
	{
	    std::cout << "no solvable" << endl;
	    continue;
	}

	testset->add(solp);
	if(i<5)
	{
	    cout << mallinfo() << endl;
	}
	++i;
    }

    cout << mallinfo() << endl;

    delete testset;
    testset = NULL;

    cout << mallinfo() << endl;
    return 0;
}

// vim:sw=4
