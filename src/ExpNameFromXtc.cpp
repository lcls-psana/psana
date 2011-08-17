//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class ExpNameFromXtc...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/ExpNameFromXtc.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <stdlib.h>
#include <fstream>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "AppUtils/AppDataPath.h"
#include "MsgLogger/MsgLogger.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  const char* logger = "ExpNameFromXtc";

  // get base name
  std::string basename(const std::string& name)
  {
    // remove dirname
    std::string::size_type n = name.rfind('/') ;
    if ( n != std::string::npos ) return std::string(name, n+1) ;
    return name ;
  }

  // extract experiment number from file name which must have format eNN-r.....xtc,
  // return negative number if cannot parse file name
  int expNumber(const std::string& path)
  {
    std::string name = basename(path);

    // strip everything after exp number
    std::string::size_type n = name.find('-') ;
    if (n == std::string::npos) return -1;
    name.erase(n);

    // parse eNN
    if ( name.size() < 2 || name[0] != 'e' ) return -1;
    char *eptr = 0 ;
    int val = strtol ( name.c_str()+1, &eptr, 10 ) ;
    if (*eptr != '\0') return -1;
    return val;
  }

}

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace psana {

//----------------
// Constructors --
//----------------
ExpNameFromXtc::ExpNameFromXtc (const std::list<std::string>& files)
  : IExpNameProvider()
  , m_files(files)
  , m_init(false)
  , m_instr()
  , m_exp()
{
}

//--------------
// Destructor --
//--------------
ExpNameFromXtc::~ExpNameFromXtc ()
{
}

// Returns instrument name
const std::string&
ExpNameFromXtc::instrument() const
{
  if (not m_init) init();

  return m_instr;
}

// Returns experiment name
const std::string&
ExpNameFromXtc::experiment() const
{
  if (not m_init) init();

  return m_exp;
}

// one-time intilization
void
ExpNameFromXtc::init() const
{
  m_init = true;

  // extract exp number for every file name, they all must be the same
  int expNum = -1;
  for (std::list<std::string>::const_iterator it = m_files.begin(); it != m_files.end(); ++ it) {
    int exp = ::expNumber(*it);
    if (exp < 0) {
      MsgLog(logger, warning, "ExpNameFromXtc: file name " << *it << " has no valid experiment number");
      break;
    }
    if (expNum < 0) {
      expNum = exp;
    } else if (exp != expNum) {
      WithMsgLog(logger, warning, out ) {
        out << "ExpNameFromXtc: XTC files belong to different experiments:";
        for (std::list<std::string>::const_iterator it = m_files.begin(); it != m_files.end(); ++ it) {
          out << "\n    " << *it;
        }
      }
      break;
    }
  }
  if (expNum < 0) return;

  // get exp name and instr name from file
  AppUtils::AppDataPath path("psana/experiment-db.dat");
  if (path.path().empty()) {
    MsgLog(logger, warning, "ExpNameFromXtc: failed to find psana/experiment-db.dat file");
    return;
  }

  // open file and read it
  std::ifstream db(path.path().c_str());
  int dbExpNum;
  std::string instrName;
  std::string expName;
  while (db >> dbExpNum >> instrName >> expName) {
    if (dbExpNum == expNum) {
      m_instr = instrName;
      m_exp = expName;
      break;
    }
  }

  if (m_instr.empty()) {
    MsgLog(logger, warning, "ExpNameFromXtc: failed to find expriment number " << expNum << " in " << path.path() << " file");
  }
}

} // namespace psana
