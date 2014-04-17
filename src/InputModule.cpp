//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class InputModule...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/InputModule.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "psana/Exceptions.h"

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
InputModule::InputModule (const std::string& name)
  : Configurable(name)
{
}

//--------------
// Destructor --
//--------------
InputModule::~InputModule ()
{
}

Index& InputModule::index() {
  throw ExceptionAbort(ERR_LOC, "Index not supported by this input module");
}

// formatting for enum
std::ostream&
operator<<(std::ostream& out, InputModule::Status stat)
{
  const char* str = "???";
  switch (stat) {
  case InputModule::BeginRun:
    str = "BeginRun";
    break;
  case InputModule::BeginCalibCycle:
    str = "BeginCalibCycle";
    break;
  case InputModule::DoEvent:
    str = "DoEvent";
    break;
  case InputModule::EndCalibCycle:
    str = "EndCalibCycle";
    break;
  case InputModule::EndRun:
    str = "EndRun";
    break;
  case InputModule::Skip:
    str = "Skip";
    break;
  case InputModule::Stop:
    str = "Stop";
    break;
  case InputModule::Abort:
    str = "Abort";
    break;
  }
  return out << str;
}

} // namespace psana
