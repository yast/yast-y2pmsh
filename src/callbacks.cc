#include <iostream>

#include <y2pm/InstSrc.h>
#include <y2pm/InstSrcDescr.h>

#include <y2pm/PMPackage.h>

#include <y2pm/RpmDbCallbacks.h>
#include <y2pm/Y2PMCallbacks.h>

#ifndef SUSE90COMPAT
#include <y2pm/MediaCallbacks.h>
#endif

#include "y2pmsh.h"
#include "callbacks.h"

using namespace std;
using namespace stringutil;

ProgressCounter y2pmshCB::toinstallcounter; // should go into pkgmanager
ProgressCounter y2pmshCB::todeletecounter; // should go into pkgmanager

/** print dots according to terminal size */
class DotPainter
{
    private:
	ProgressCounter _pc;
	int _cols; // terminal width
	int _dots; // number of already painted dots
	string _ok; // string to display in case of success
	unsigned _updatepercent; // dots are painted after this number of percentage has changed

    protected:
	virtual void paint()
	{
	    if(_cols - _dots)
	    {
		int needdots = _cols * _pc.percent() / 100;
		if(needdots > _dots)
		{
		    for(int i = 0; i < needdots - _dots; ++i)
		    {
			cout << '.';
		    }
		    cout << flush;
		    _dots = needdots;
		}
	    }
	}

    public:
	DotPainter(const std::string ok = " ok") : _ok(ok)
	{
	}

	virtual ~DotPainter() {};

	virtual void start( const string& msg )
	{
	    _pc.reset();
	    _cols = _dots = 0;

	    _cols = y2pmsh.tty_width();

	    _cols = _cols - msg.length() - _ok.length();
	    
	    if(_cols <= 0)
		_cols = 60;
	    
	    cout << msg;
	};
	virtual void progress( const ProgressData & prg )
	{
	    _pc = prg;
	    if(_pc.updateIfNewPercent(_updatepercent) && _pc.percent())
	    {
		paint();
	    }
	};
	virtual void stop( PMError error )
	{
	    if(error != PMError::E_ok)
		cout << "\n *** failed: " << error << endl;
	    else
	    {
		_pc.set(100);
		paint();

		cout << _ok << endl;
	    }
	};
	void updatePercent(unsigned percent)
	{
	    _updatepercent = percent;
	}
};

class ConvertDbCallback : public RpmDbCallbacks::ConvertDbCallback
{
    private:
	int _failed;
	int _ignored;
	int _alreadyInV4;
	DotPainter _dp;

    public:
	ConvertDbCallback()
	{
		RpmDbCallbacks::convertDbReport.redirectTo(*this);
	}

	virtual void start( const Pathname & v3db )
	{
	    _failed = _ignored = _alreadyInV4 = 0;
	    _dp.start(form("converting database %s ", v3db.asString().c_str()));
	}
	virtual void progress( const ProgressData & prg,
			       unsigned failed, unsigned ignored, unsigned alreadyInV4 )
	{
	    _dp.progress(prg);
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
	    _dp.stop(error);
	    cout << _failed << " failed" << endl;
	    cout << _ignored << " ignored" << endl;
	    cout << _alreadyInV4 << " already in V4 format" << endl;
	};
};

class InstallPkgCallback : public RpmDbCallbacks::InstallPkgCallback
{
    private:
	DotPainter& _dp;

    public:
	InstallPkgCallback(DotPainter& dp) : _dp(dp)
	{
	    RpmDbCallbacks::installPkgReport.redirectTo( this );
	}
	virtual void start( const Pathname & filename )
	{
	};
	virtual void progress( const ProgressData & prg )
	{
	    _dp.progress(prg);
	};
	virtual void stop( PMError error )
	{
	};
};

#ifndef SUSE90COMPAT
class DownloadCallback : public MediaCallbacks::DownloadProgressCallback
{
    private:
	DotPainter& _dp;
    public:
	DownloadCallback(DotPainter& dp) : _dp(dp)
	{
	    MediaCallbacks::downloadProgressReport.redirectTo( this );
	}
	virtual void start( const Url & url_r, const Pathname & localpath_r )
	{
	}
	virtual void progress( const ProgressData & prg )
	{
	    _dp.progress(prg);
	}
	virtual void stop( PMError error )
	{
	}
};
#endif

class RemovePkgCallback : public RpmDbCallbacks::RemovePkgCallback
{
    private:
	DotPainter& _dp;
    public:
	RemovePkgCallback(DotPainter& dp) : _dp(dp)
	{
	    RpmDbCallbacks::removePkgReport.redirectTo( this );
	}
	virtual void start( const string& label )
	{
	};
	virtual void progress( const ProgressData & prg )
	{
	    _dp.progress(prg);
	};
	virtual void stop( PMError error )
	{
	};
};

class RebuildDbCallback : public RpmDbCallbacks::RebuildDbCallback
{
    private:
	DotPainter _dp;

    public:
	RebuildDbCallback()
	{
	    RpmDbCallbacks::rebuildDbReport.redirectTo(*this);
	}
	
	virtual void start()
	{
	    _dp.start("rebuilding RPM database ");
	};
	virtual void progress( const ProgressData & prg )
	{
	    _dp.progress(prg);
	};
	virtual void stop( PMError error )
	{
	    _dp.stop(error);
	};
#ifndef SUSE90COMPAT
	virtual void notify( const string& msg )
	{
	    cout << msg << endl;
	}
#endif
};

class CommitCallback : public Y2PMCallbacks::CommitCallback
{
    public:
	CommitCallback()
	{
	    Y2PMCallbacks::commitReport.redirectTo(*this);
	}

	virtual void advanceToMedia( constInstSrcPtr srcptr, unsigned mediaNr )
	{
	    constInstSrcDescrPtr descr = srcptr->descr();
	    string label = descr?descr->content_label():"???";
	    cout << "Installing from '" << label << "', media nr. " << mediaNr << endl;
	}
};

class CommitInstallCallback : public Y2PMCallbacks::CommitInstallCallback
{
    private:
	DotPainter _dp;
	InstallPkgCallback installpkgcallback;

	string package;

    public:
	CommitInstallCallback() : installpkgcallback(_dp)
	{
	    Y2PMCallbacks::commitInstallReport.redirectTo(*this);
	}
	virtual void start( constPMPackagePtr pkg, bool sourcepkg, const Pathname & path )
	{
	    y2pmshCB::toinstallcounter.incr();
	    package = pkg->nameEd() + " (" + pkg->arch().asString() + ") ";
	}
	virtual CBSuggest attempt( unsigned cnt )
	{
	    _dp.start(form("installing [%3d%%] ", y2pmshCB::toinstallcounter.percent()) + package );
	    return CBSuggest::PROCEED;
	}
	virtual CBSuggest result( PMError error )
	{
	    _dp.stop(error);
	    if(error)
		return CBSuggest::CANCEL;
	    else
		return CBSuggest::PROCEED;
	}
	virtual void stop( PMError error )
	{
	    if(error)
		cout << error << endl;
	}
};

class CommitRemoveCallback : public Y2PMCallbacks::CommitRemoveCallback
{
    private:
	DotPainter _dp;
	RemovePkgCallback removepkgcallback;

	string package;

    public:
	CommitRemoveCallback() : removepkgcallback(_dp)
	{
	    Y2PMCallbacks::commitRemoveReport.redirectTo(*this);
	}
	virtual void start( constPMPackagePtr pkg )
	{
	    y2pmshCB::todeletecounter.incr();
	    package = pkg->nameEd() + " (" + pkg->arch().asString() + ") ";
	}
	virtual CBSuggest attempt( unsigned cnt )
	{
	    _dp.start(stringutil::form("removing [%3d%%] ", y2pmshCB::todeletecounter.percent()) + package);
	    return CBSuggest::PROCEED;
	}
	virtual CBSuggest result( PMError error )
	{
	    _dp.stop(error);
	    if(error)
		return CBSuggest::CANCEL;
	    else
		return CBSuggest::PROCEED;
	}
	virtual void stop( PMError error )
	{
	    if(error)
		cout << error << endl;
	}
};

class CommitProvideCallback : public Y2PMCallbacks::CommitProvideCallback
{
    private:
	DotPainter _dp;
	DownloadCallback downloadcallback;

	string package;

    public:
	CommitProvideCallback() : downloadcallback(_dp)
	{
	    Y2PMCallbacks::commitProvideReport.redirectTo(*this);
	}
	virtual void start( constPMPackagePtr pkg, bool sourcepkg )
	{
	    y2pmshCB::todeletecounter.incr();
	    package = pkg->name().asString() + " (" + pkg->archivesize().asString() + ") ";
	}
	virtual CBSuggest attempt( unsigned cnt )
	{
	    _dp.start(string("downloading ") + package );
	    return CBSuggest::PROCEED;
	}
	virtual CBSuggest result( PMError error, const Pathname & localpath )
	{
	    _dp.stop(error);
	    if(error)
		return CBSuggest::CANCEL;
	    else
		return CBSuggest::PROCEED;
	}
	virtual void stop( PMError error, const Pathname & localpath )
	{
	    if(error)
		cout << error << endl;
	}
};

class ScanDbCallback : RpmDbCallbacks::ScanDbCallback
{
    private:
	DotPainter _dp;
    public:
	ScanDbCallback()
	{
	    RpmDbCallbacks::scanDbReport.redirectTo(*this);
	}
	virtual void start()
	{
	    _dp.start("reading RPM database ");
	}
	virtual void progress( const ProgressData & prg )
	{
	    _dp.progress(prg);
	}
	virtual void stop( PMError error )
	{
	    _dp.stop(error);
	}
};


#ifndef SUSE90COMPAT
static CommitProvideCallback commitprovidecallback;
#endif
static RebuildDbCallback rebuilddbcallback;
static ConvertDbCallback convertdbcallback;

static CommitCallback commitcallback;
static CommitInstallCallback commitinstallcallback;
static CommitRemoveCallback commitremovecallback;

static ScanDbCallback scandbcallback;
