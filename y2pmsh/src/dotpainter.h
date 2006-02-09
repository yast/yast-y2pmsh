#ifndef Y2PMSH_DOTPAINTER_H
#define Y2PMSH_DOTPAINTER_H

#include "tty.h"

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
			std::cout << '.';
		    }
		    std::cout << std::flush;
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
	    _pc.range(0, 100); // default range in case progress() never gets called
	    _cols = _dots = 0;

	    _cols = TTY::width();

	    _cols = _cols - msg.length() - _ok.length();
	    
	    if(_cols <= 0)
		_cols = 60;
	    
	    std::cout << msg;
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
		std::cout << "\n *** failed: " << error << std::endl;
	    else
	    {
		_pc.toMax();
		paint();

		std::cout << _ok << std::endl;
	    }
	};
	void updatePercent(unsigned percent)
	{
	    _updatepercent = percent;
	}
};

#endif
