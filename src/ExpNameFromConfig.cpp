//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class ExpNameFromConfig...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/ExpNameFromConfig.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace psana {

//----------------
// Constructors --
//----------------
ExpNameFromConfig::ExpNameFromConfig (const std::string& instr, const std::string& exp)
  : IExpNameProvider()
  , m_instr(instr)
  , m_exp(exp)
{
}

//--------------
// Destructor --
//--------------
ExpNameFromConfig::~ExpNameFromConfig ()
{
}

} // namespace psana
