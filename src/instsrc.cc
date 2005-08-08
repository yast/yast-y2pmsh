#include <y2util/Url.h>

#include <Y2PM.h>
#include <y2pm/InstSrcDescr.h>
#include <y2pm/InstSrcManager.h>
#include <y2pm/InstTarget.h>

#include "y2pmsh.h"
#include "instsrc.h"
#include "helpscreen.h"

using namespace std;

int enablesources(vector<string>& argv)
{
    InstSrcManager::ISrcIdList isrclist;

    cout << "initializing installation sources ..." << endl;

    Y2PM::instSrcManager().getSources(isrclist);

    for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
	it != isrclist.end(); ++it)
    {
	constInstSrcDescrPtr descr = (*it)->descr();
	if(!descr) continue;
	if(descr->default_activate())
	    Y2PM::instSrcManager().enableSource(*it);
    }

    return 0;
}


#ifdef SUSE93COMPAT
/** this is a hack since y2pm does not allow updating sources. we remember the
 * url and position of the source and delete it. then we add a new source and
 * adjust the rank until the sources's position matches the previously
 * remembered one */
static int source_update(const vector<string>& argv, unsigned parampos)
{
    while(parampos < argv.size())
    {
	InstSrcManager::ISrcIdList isrclist;
	Y2PM::instSrcManager().getSources(isrclist);

	int num = atoi(argv[parampos++].c_str());
	if(num < 0 || static_cast<unsigned>(num) >= isrclist.size())
	{
	    cout << "invalid number" << endl;
	    return 1;
	}

	int srcnr = 0;
	constInstSrcPtr oldsrc;

	for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
	    it != isrclist.end(); ++it, ++srcnr)
	{
	    if(srcnr == num)
	    {
		oldsrc = *it;
		break;
	    }
	}

	if(srcnr != num)
	{
	    cout << "source number " << srcnr << " does not exist" << endl;
	    return 1;
	}

	Url url = oldsrc->descr()->url();

	/** first delete existing source */
	PMError err = Y2PM::instSrcManager().deleteSource(oldsrc);
	if( err != PMError::E_ok)
	{
	    cout << "Can't delete source: " << err << endl;
	    return 1;
	}

	/** now scan if we can construct a new soure on old url */
	InstSrcManager::ISrcIdList newsrclist;

	cout << "scan new source " << url << ": " << flush;
	err = Y2PM::instSrcManager().scanMedia(newsrclist, url);
	cout << err << endl;
	if(err)
	{
	    return 1;
	}
	if(newsrclist.size()>1)
	{
	    cout << "found more than one source, abort" << endl;
	    return 1;
	}

	cout << "enabling source ... " << flush;
	constInstSrcPtr newsrc = *newsrclist.begin();
	err = Y2PM::instSrcManager().enableSource(newsrc);
	cout << err << endl;
	if(err)
	{
	    return 1;
	}

	cout << "adjust ranking ... " << flush;

	Y2PM::instSrcManager().getSources(isrclist);
	int newsrcnr = isrclist.size();
	--newsrcnr;
	err = PMError::E_ok;
	while(newsrcnr != srcnr && newsrcnr >= 0)
	{
	    --newsrcnr;
	    err = Y2PM::instSrcManager().rankUp(newsrc);
	    if(err)
	    {
		cout << err << endl;
		return 1;
	    }
	}
	err = Y2PM::instSrcManager().setNewRanks();
	if(err)
	{
	    cout << err << endl;
	    return 1;
	}

	cout << err << endl;
    }
    return 0;
}
#else
static int source_update(const vector<string>& argv, unsigned parampos)
{
    while(parampos < argv.size())
    {
	InstSrcManager::ISrcIdList isrclist;
	Y2PM::instSrcManager().getSources(isrclist);

	unsigned num = atoi(argv[parampos++].c_str());
	if(num >= isrclist.size())
	{
	    cout << "invalid number" << endl;
	    return 1;
	}

	unsigned count = 0;
	for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
		it != isrclist.end(); ++it, count++)
	{
	    if(count != num)
		continue;

	    PMError err = Y2PM::instSrcManager().refreshSource(*it, true);
	    if( err != PMError::E_ok)
	    {
		cout << "Error: " << err << endl;
		return 1;
	    }
	}
    }

    return 0;
}
#endif

static int source_set_autorefresh(bool yes, const vector<string>& argv, unsigned parampos)
{
#ifndef SUSE93COMPAT
    while(parampos < argv.size())
    {
	InstSrcManager::ISrcIdList isrclist;
	Y2PM::instSrcManager().getSources(isrclist);

	unsigned num = atoi(argv[parampos++].c_str());
	if(num >= isrclist.size())
	{
	    cout << "invalid number" << endl;
	    return 1;
	}

	unsigned count = 0;
	for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
		it != isrclist.end(); ++it, count++)
	{
	    if(count != num)
		continue;

	    PMError err = Y2PM::instSrcManager().setAutorefresh(*it, yes);
	    if( err != PMError::E_ok)
	    {
		cout << "Error: " << err << endl;
		return 1;
	    }
	}
    }
#else
    cout << "feature not supported" << endl;
#endif
    return 0;
}

static void showsource(constInstSrcPtr src)
{
    if(!src) return;
    constInstSrcDescrPtr descr = src->descr();
    bool on = false;
    if(y2pmsh.initialized())
	on = src->enabled();
    else
	on = descr->default_activate();

    cout << stringutil::form("[%c] %s (%s)",
	on?'x':' ',
	descr->content_label().c_str(),
	descr->url().asString().c_str());
#ifndef SUSE93COMPAT
    if(descr->default_refresh())
	cout << " [autorefresh]";
#endif
    cout << '\n';
}

int source(vector<string>& argv)
{
    InstSrcManager::ISrcIdList isrclist;
    enum {
	src_show,
	src_enable,
	src_disable,
	src_add,
	src_remove,
	src_update,
	src_product,
	src_enable_autorefresh,
	src_disable_autorefresh
    } mode = src_show;
    int ret = 0;

    if(argv.size()<2 || argv[1] == "--help")
    {
	HelpScreen h("source");
	h.Parameter(HelpScreenParameter("-e ID", "--enable ID", "enable source number ID"));
	h.Parameter(HelpScreenParameter("-d ID", "--disable ID", "disable source number ID"));
	h.Parameter(HelpScreenParameter("-a URL", "--add URL", "add a new source at URL"));
	h.Parameter(HelpScreenParameter("-u ID", "--update ID", "updated cached data for ID"));
	h.Parameter(HelpScreenParameter("-R ID", "--remove ID", "remove source number ID"));
	h.Parameter(HelpScreenParameter("-P ID", "--product ID", "install source number ID as product"));
#ifndef SUSE93COMPAT
	h.Parameter(HelpScreenParameter("-A ID", "--autorefresh ID", "enable autorefresh on source number ID"));
	h.Parameter(HelpScreenParameter("", "--noautorefresh ID", "disable autorefresh on source number ID"));
#endif
	h.Parameter(HelpScreenParameter("-s", "--show", "show known sources"));
	cout << h;
	return 0;
    }

    unsigned parampos = 1;
    string param = argv[parampos++];

    if(param == "--enable" || param == "-e")
    {
	if (parampos >= argv.size())
	{
	    cout << "must specify number" << endl;
	    return 1;
	}
	mode = src_enable;
    }
    else if(param == "--disable" || param == "-d")
    {
	if (parampos >= argv.size())
	{
	    cout << "must specify number" << endl;
	    return 1;
	}
	mode = src_disable;
    }
    else if(param == "--add" || param == "-a")
    {
	if (parampos >= argv.size())
	{
	    cout << "must specify url" << endl;
	    return 1;
	}
	mode = src_add;
    }
    else if(param == "--remove" || param == "-R")
    {
	if (parampos >= argv.size())
	{
	    cout << "must specify number" << endl;
	    return 1;
	}
	mode = src_remove;
    }
    else if(param == "--update" || param == "-u")
    {
	if (parampos >= argv.size())
	{
	    cout << "must specify number" << endl;
	    return 1;
	}
	mode = src_update;
    }
    else if(param == "--product" || param == "-P")
    {
	if (parampos >= argv.size())
	{
	    cout << "must specify number" << endl;
	    return 1;
	}
	mode = src_product;
    }
    else if(param == "--autorefresh" || param == "-A")
    {
	if (parampos >= argv.size())
	{
	    cout << "must specify number" << endl;
	    return 1;
	}
	mode = src_enable_autorefresh;
    }
    else if(param == "--noautorefresh")
    {
	if (parampos >= argv.size())
	{
	    cout << "must specify number" << endl;
	    return 1;
	}
	mode = src_disable_autorefresh;
    }
    else if(param == "--show" || param == "-s")
    {
	mode = src_show;
    }

    if(!y2pmsh.initialized()) 
	Y2PM::noAutoInstSrcManager();

    y2pmsh.ReleaseMediaAtExit();

    switch ( mode )
    {
	case src_enable:
	{
	    Y2PM::instSrcManager().getSources(isrclist);

	    while(parampos < argv.size())
	    {
		int num = atoi(argv[parampos++].c_str());
		if(num < 0 || static_cast<unsigned>(num) >= isrclist.size())
		{
		    cout << "invalid number" << endl;
		    return 1;
		}

		int count = 0;
		for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
		    it != isrclist.end(); ++it, count++)
		{
		    if(count != num)
			continue;

		    PMError err = Y2PM::instSrcManager().setAutoenable(*it, true);
		    if( err != PMError::E_ok)
		    {
			cout << "Error: " << err << endl;
			return 1;
		    }

		    err = Y2PM::instSrcManager().enableSource(*it);
		    if( err != PMError::E_ok)
		    {
			cout << "Error: " << err << endl;
			return 1;
		    }
		}
	    }
	} break;
	case src_disable:
	{
	    Y2PM::instSrcManager().getSources(isrclist);

	    while(parampos < argv.size())
	    {
		int num = atoi(argv[parampos++].c_str());
		if(num < 0 || static_cast<unsigned>(num) >= isrclist.size())
		{
		    cout << "invalid number" << endl;
		    return 1;
		}

		int count = 0;
		for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
		    it != isrclist.end(); ++it, count++)
		{
		    if(count != num)
			continue;

		    PMError err = Y2PM::instSrcManager().setAutoenable(*it, false);
		    if( err != PMError::E_ok)
		    {
			cout << "Error: " << err << endl;
			return 1;
		    }

		    err = Y2PM::instSrcManager().disableSource(*it);
		    if( err != PMError::E_ok)
		    {
			cout << "Error: " << err << endl;
			return 1;
		    }
		}
	    }
	} break;

	case src_remove:
	{
	    Y2PM::instSrcManager().getSources(isrclist);

	    while(parampos < argv.size())
	    {
		int num = atoi(argv[parampos++].c_str());
		if(num < 0 || static_cast<unsigned>(num) >= isrclist.size())
		{
		    cout << "invalid number" << endl;
		    return 1;
		}

		int count = 0;
		for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
		    it != isrclist.end(); ++it, count++)
		{
		    if(count != num)
			continue;

		    PMError err = Y2PM::instSrcManager().deleteSource(*it);
		    if( err != PMError::E_ok)
		    {
			cout << "Error: " << err << endl;
			return 1;
		    }
		}
	    }
	} break;
	case src_add:
	{
	    param = argv[parampos++];

	    Url url(param);

	    if(!url.isValid())
	    {
		cout << "URL is invalid" << endl;
		return 1;
	    }

	    PMError err = Y2PM::instSrcManager().scanMedia(isrclist, url);
	    if(err != PMError::E_ok)
	    {
		cout << "failed to detect sources on " << url << endl;
		cout << err << endl;
		return 1;
	    }

	    for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
		it != isrclist.end(); ++it)
	    {
		PMError error = Y2PM::instSrcManager().enableSource(*it);
		if(error)
		{
		    ++ret;
		    cerr << error << endl;
		}
	    }
	} break;
	case src_enable_autorefresh:
	{
	    ret = source_set_autorefresh(true, argv, parampos);
	} break;
	case src_disable_autorefresh:
	{
	    ret = source_set_autorefresh(false, argv, parampos);
	} break;
	case src_show:
	{
	    Y2PM::instSrcManager().getSources(isrclist);

	    cout << "Known sources:" << endl;
	    unsigned count = 0;
	    for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
		it != isrclist.end(); ++it, count++)
	    {
		cout << count << ": ";
		showsource(*it);
	    }
	} break;
	case src_update:
	{
	    ret = source_update(argv, parampos);
	} break;
	case src_product:
	{
	    Y2PM::instSrcManager().getSources(isrclist);

	    while(parampos < argv.size())
	    {
		int num = atoi(argv[parampos++].c_str());
		if(num < 0 || static_cast<unsigned>(num) >= isrclist.size())
		{
		    cout << "invalid number" << endl;
		    return 1;
		}

		int count = 0;
		for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
		    it != isrclist.end(); ++it, ++count)
		{
		    if(count != num)
			continue;

		    PMError err = Y2PM::instTarget().installProduct((*it)->descr());
		    if( err != PMError::E_ok)
		    {
			cout << "Error: " << err << endl;
			return 1;
		    }
		}
	    }
	} break;
    }
    return ret;
}

int _deletemediaatexit(vector<string>& argv)
{
	y2pmsh.DeleteMediaAtExit();
	return 0;
}

int sourceorder( vector<string>& argv )
{
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
	    h.optionaloptions(true);
	    h.additionalparams("IDs");
	    h.Parameter(HelpScreenParameter("-d", "--default", "set default order"));
	    cout << h;
	    return 0;
	}
	else if(*arg == "-d" || *arg == "--default")
	{
	    Y2PM::instSrcManager().setDefaultInstOrder();
	    return 0;
	}
    }

    if(arg != argv.end())
    {
	InstSrcManager::ISrcIdList isrclist;
	InstSrcManager::InstOrder order;

	Y2PM::instSrcManager().getSources(isrclist);

	for(;arg != argv.end(); ++arg)
	{
	    unsigned num = atoi((*arg).c_str());
	    unsigned i = 0;

	    for(InstSrcManager::ISrcIdList::iterator it = isrclist.begin();
		it != isrclist.end(); ++it, ++i)
	    {
		if(i == num)
		    order.push_back((*it)->srcID());
	    }
	}
	Y2PM::instSrcManager().setInstOrder(order);
    }

    {
	const InstSrcManager::InstOrder &order = Y2PM::instSrcManager().instOrder();
	cout << "Order:\n";
	for(InstSrcManager::InstOrder::const_iterator it = order.begin();
		it != order.end(); ++it)
	{
	    showsource(Y2PM::instSrcManager().getSourceByID(*it));
	}
	cout << endl;
    }
    return 0;
}
