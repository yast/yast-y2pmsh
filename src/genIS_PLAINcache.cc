#include <iostream>
#include <string>

#include <y2util/Y2SLog.h>
#include <y2util/PathInfo.h>
#include <y2util/stringutil.h>

#include <y2pm/RpmHeaderCache.h>
#include <y2pm/RpmHeader.h>

using namespace std;

#undef  Y2LOG
#define Y2LOG "genIS_PLAINcache"

/******************************************************************
**
**
**	FUNCTION NAME : usage
**	FUNCTION TYPE : void
*/
void usage()
{
  cout << "Usage: genIS_PLAINcache [-c CACHEFILE] [-f] [-r] [-v] PKGROOTDIR" << endl;
  cout << "   or: genIS_PLAINcache -h" << endl;
  cout << "" << endl;
  cout << "Scan PKGROOTDIR for RPM packages and store header information in a CACHEFILE" << endl;
  cout << "used by YaST for a plain RPM InstSource. A cachefile named 'IS_PLAINcache' (or" << endl;
  cout << "'IS_PLAINcache.gz' if zipped) is mandatory for plain RPM InstSource accessed" << endl;
  cout << "via FTP/HTTP." << endl;
  cout << "CACHEFILE must not exist (unless -f is used). Symlinks are always ignored." << endl;
  cout << "" << endl;
  cout << "   -c CACHEFILE    Name of the cachefile to create." << endl;
  cout << "                   (default is 'PKGROOTDIR/IS_PLAINcache')" << endl;
  cout << "   -f              Unlink already existing cachefile." << endl;
  cout << "   -r              Scan PKGROOTDIR recursively." << endl;
  cout << "   -v              Explain what is being done." << endl;
  cout << "   -h              Show this." << endl;

  ERR << "Exit: -> 1" << endl;
  exit( 1 );
}

/******************************************************************
**
**
**	FUNCTION NAME : main
**	FUNCTION TYPE : int
*/
int main( int argc, char *argv[] )
{
  set_log_filename("/dev/null");
  Pathname pkgrootdir;
  Pathname cachefile;
  bool     force = false;
  RpmHeaderCache::buildOpts options;

  if ( argc <= 1 ) {
    cerr << "genIS_PLAINcache: missing operand" << endl;
    usage(); // does not return
  }

  int c = -1;
  while ( (c = getopt( argc, argv, "c:fhrv" )) >= 0 ) {
    switch ( c ) {
    case 'c':
      cachefile = optarg;
      break;
    case 'f':
      force = true;
      break;
    case 'h':
      usage();// does not return
      break;
    case 'r':
      options.recurse = true;
      break;
    case 'v':
      set_log_filename("-");
      break;

    default:
      usage(); // does not return
      break;
    }
  }

  if ( optind != argc - 1 ) {
    if ( optind >= argc ) {
      cerr << "genIS_PLAINcache: missing operand after '" << argv[argc-1] << "'" << endl;
      usage(); // does not return
    }
    cerr << "genIS_PLAINcache: extra operand '" << argv[optind+1] << "'" << endl;
    usage(); // does not return
  }

  pkgrootdir = argv[optind];

  if ( cachefile.empty() ) {
    cachefile = pkgrootdir + "IS_PLAINcache";
  }

  MIL << "Start " << argv[0] << endl;
  MIL << "pkgrootdir " << pkgrootdir << endl;
  MIL << "cachefile  " << cachefile << endl;
  MIL << "force      " << force << endl;
  MIL << "recurse    " << options.recurse << endl;

  PathInfo pi( pkgrootdir );
  if ( !pi.isDir() ) {
    cerr << "genIS_PLAINcache: '" << pkgrootdir << "' is not a directory" << endl;
    ERR << "Exit: -> 2 " << pkgrootdir << endl;
    exit( 2 );
  }

  pi.lstat( cachefile );
  if ( pi.isExist() ) {
    if ( force ) {
      int res = PathInfo::unlink( cachefile );
      if ( res ) {
	cerr << "genIS_PLAINcache: can't remove existing '" << cachefile << "' (errno " << res << ")" << endl;
	ERR << "Exit: -> 3 " << cachefile << " (errno " << res << ")" << endl;
	exit( 3 );
      }
    } else {
      cerr << "genIS_PLAINcache: '" << cachefile << "' exists (use -f to remove it)" << endl;
      ERR << "Exit: -> 3 " << cachefile << endl;
      exit( 3 );
    }
  }

  MIL << "Start package scan in " << pkgrootdir << endl;
  int res = RpmHeaderCache::buildHeaderCache( cachefile, pkgrootdir, options );
  if ( res < 0 ) {
    cerr << "genIS_PLAINcache: failed to create cache '" << cachefile << "' (" << res << ")" << endl;
    ERR << "Exit: -> 4 " << cachefile << " (" << res << ")" << endl;
    PathInfo::unlink( cachefile );
    exit( 4 );
  }
  MIL << "Created cache for " << res << " packages found." << endl;

  pi(); // restat
  cout << res << " packages cached in " << pi << endl;

  MIL << "Exit: -> 0 " << argv[0] << endl;
  return 0;
}
