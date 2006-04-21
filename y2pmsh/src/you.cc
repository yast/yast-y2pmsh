#include <fstream>

#include <algorithm>

#include <y2util/Y2SLog.h>
#include <y2util/stringutil.h>
#include <y2util/ProgressCounter.h>

#include <Y2PM.h>
#include <y2pm/PMPackageManager.h>
#include <y2pm/PMYouPatch.h>
#include <y2pm/PMPackage.h>
#include <y2pm/PMYouPatchInfo.h>
#include <y2pm/PMYouPatchManager.h>
#include <y2pm/MediaAccess.h>
#include <y2pm/InstYou.h>
#include <y2pm/PMYouServers.h>
#include <y2pm/YouError.h>
#include <y2pm/PMYouProduct.h>

#include "y2pmsh.h"
#include "summary.h"
#include "helpscreen.h"
#include "dotpainter.h"

using namespace std;
using namespace stringutil;

#define RIE(err, rv) if(err) { cout << err << endl; return rv; }

class Callbacks : public InstYou::Callbacks
{
    private:
	int total_percent;
	DotPainter _dp;
	string _str;

    public:
	Callbacks() : InstYou::Callbacks(), total_percent(0)
	{
	}
	virtual ~Callbacks()
	{
	}

	virtual bool progress( int percent )
	{
	    total_percent = percent;
	    return true;
	}
	virtual bool patchProgress( int percent, const std::string &str )
	{
	    if(percent == 0)
	    {
		if(!_str.empty() && str != _str)
		{
		    _dp.stop(PMError());
		}
		_dp.start(str);
		_str = str;
	    }
	    else if(percent == 100)
	    {
		_dp.stop(PMError());
		_str.clear();
	    }
	    else
	    {
		_dp.progress(ProgressData(0, 100, percent));
	    }
	    return true;
	}

	virtual PMError showError( const std::string &type,
		const std::string &text,
		const std::string &details )
	{
	    cout << "Error: Type: " << type << ", Text: " << text << endl;
	    if(!details.empty())
		cout << "Details: " << details << endl;
	    return PMError();
	}
	virtual PMError showMessage( const std::string &type,
		const std::list<PMYouPatchPtr> & )
	{
	    cout << "Message: ???" << endl;
	    return PMError();
	}
	virtual void log( const std::string &text )
	{
	    cout << text;
	    if(text[text.length()-1] != '\n')
		cout << endl;
	    else
		cout << flush;
	}

	virtual bool executeYcpScript( const std::string &script )
	{
	    cout << "executeYcpScript unsupported" << endl;
	    return false;
	}
};

// ensure callbacks are reset on destruction
class setYouCallbacks
{
    private:
	InstYou& _you;
	Callbacks callbacks;

    public:
	setYouCallbacks(InstYou& you) : _you(you)
	{
	    _you.setCallbacks(&callbacks);
	}
	~setYouCallbacks()
	{
	    _you.setCallbacks(NULL);
	}
};

int onlineupdate(vector<string>& argv)
{
    string arg_url;
    bool arg_reload = false;
    bool arg_nosigcheck = false;
    bool arg_autoget = false;
    bool arg_show = false;

    vector<string>::iterator arg = argv.begin()+1;
    for(;arg != argv.end() && (*arg)[0] == '-'; ++arg)
    {
	if(*arg == "--")
	{
	    ++arg;
	    break;
	}
	else if(*arg == "-h" || *arg == "--help")
	{
	    HelpScreen h(argv[0]);
	    h.Parameter(HelpScreenParameter("-u URL", "--url URL", "get patches from URL"));
	    h.Parameter(HelpScreenParameter("-r", "--reload", "always reload all patchinfos"));
	    h.Parameter(HelpScreenParameter("-n", "--no-sig-check", "don't check signatures (use only for debugging!)"));
	    h.Parameter(HelpScreenParameter("-g","--download-only", "Only download patches. Do not install."));
	    h.Parameter(HelpScreenParameter("-s","--show", "Only show patches."));
	    cout << h;
	    return 0;
	}
	else if(*arg == "-u" || *arg == "--url")
	{
	    if (++arg == argv.end())
	    {
		cout << "must specify URL" << endl;
		return 1;
	    }
	    arg_url = *arg;
	}
	else if(*arg == "-r" || *arg == "--reload")
	{
	    arg_reload = true;
	}
	else if(*arg == "-n" || *arg == "--no-sig-check")
	{
	    arg_nosigcheck = true;
	}
	else if(*arg == "-g" || *arg == "--download-only")
	{
	    arg_autoget = true;
	}
	else if(*arg == "-s" || *arg == "--show")
	{
	    arg_show = true;
	}
    }

    InstYou you;

    setYouCallbacks callbacks(you);
    
    PMYouServer server;
    if ( arg_url.empty() )
    {
	PMYouServers youServers( you.settings() );
	server = youServers.currentServer();
    }
    else
    {
	Url url( arg_url );
	if ( !url.isValid())
	{
	    cout << form("Invalid URL: %s", arg_url.c_str() ) << endl;
	    return 1;
	}
	server.setUrl( url );
	server.setType( PMYouServer::Custom );
    }

    PMError err;

    err = you.initProduct();
    RIE(err, 1);

    you.settings()->setPatchServer( server );
    you.settings()->setCheckSignatures( !arg_nosigcheck );
    you.settings()->setReloadPatches( arg_reload );
    you.settings()->setGetOnly( arg_autoget );

    cout << "Installing from " << you.settings()->patchServer().toString() << endl;

    err = you.retrievePatchDirectory();
    RIE(err, 1);

    err = you.retrievePatchInfo();
    RIE(err, 1);

    int kinds = 0;
    kinds = PMYouPatch::kind_security | PMYouPatch::kind_recommended | PMYouPatch::kind_patchlevel;

    you.selectPatches( kinds );

    you.showPatches( true );

    if(!arg_show)
    {
	err = you.processPatches();
	RIE(err, 1);
    }

    return 0;
}


// vim: sw=4
