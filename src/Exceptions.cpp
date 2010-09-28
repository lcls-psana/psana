//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class Exceptions...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------
#include "SITConfig/SITConfig.h"

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/Exceptions.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <cerrno>
#include <string.h>
#include <dlfcn.h>

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

Exception::Exception( const std::string& what )
  : std::runtime_error( "psana::Exception: " + what )
{
}

ExceptionModuleName::ExceptionModuleName ( const std::string& module )
  : Exception( "invalid module name: " + module)
{  
}

ExceptionErrno::ExceptionErrno ( const std::string& what )
  : Exception( what + ": " + strerror(errno) )
{
}

ExceptionDlerror::ExceptionDlerror ( const std::string& what )
  : Exception( what + ": " + dlerror() )
{
}

} // namespace psana
