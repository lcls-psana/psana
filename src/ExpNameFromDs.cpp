//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class ExpNameFromDs...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/ExpNameFromDs.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <stdlib.h>
#include <fstream>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "ExpNameDb/ExpNameDatabase.h"
#include "IData/Dataset.h"
#include "MsgLogger/MsgLogger.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

using namespace std;

namespace {

  const char* logger = "ExpNameFromDs";

}

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace psana {

//----------------
// Constructors --
//----------------
ExpNameFromDs::ExpNameFromDs (const std::vector<std::string>& files)
  : IExpNameProvider()
  , m_instr()
  , m_exp()
  , m_expNum(0)
{
  // extract exp number for every file name, they all must be the same
  for (vector<string>::const_iterator it = files.begin(); it != files.end(); ++ it) {
    
    IData::Dataset ds(*it);
    
    if (m_expNum == 0) {
      m_expNum = ds.expID(); // can be zero for new 2020 filenaming - cpo
      m_instr = ds.instrument();
      m_exp = ds.experiment();
    } else {
      if (ds.expID() != m_expNum) {
        WithMsgLog(logger, warning, out ) {
          out << "ExpNameFromDs: datasets belong to different experiments:";
          for (vector<string>::const_iterator it = files.begin(); it != files.end(); ++ it) {
            out << "\n    " << *it;
          }
        }
        break;
      }
    }
    
  }
}

//--------------
// Destructor --
//--------------
ExpNameFromDs::~ExpNameFromDs ()
{
}

} // namespace psana
