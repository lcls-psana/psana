//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class Module...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------
#include "SITConfig/SITConfig.h"

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/Module.h"

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
Module::Module (const std::string& name)
  : m_name(name)
  , m_className(name)
{
  // get class name from module name
  std::string::size_type p = m_className.find(':');
  if (p != std::string::npos) {
    m_className.erase(p);
  }
}

//--------------
// Destructor --
//--------------
Module::~Module ()
{
}

/// get the name of the module
const std::string& 
Module::name() const 
{
  return m_name;
}

/// get the class name of the module
const std::string& 
Module::className() const 
{
  return m_className;
}

} // namespace psana
