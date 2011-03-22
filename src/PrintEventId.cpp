//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class PrintEventId...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/PrintEventId.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "PSEvt/EventId.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

using namespace psana;
PSANA_MODULE_FACTORY(PrintEventId)

namespace {
  
  // name of the logger to be used with MsgLogger
  const char* logger = "PrintEventId"; 
  
}

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace psana {

//----------------
// Constructors --
//----------------
PrintEventId::PrintEventId (const std::string& name)
  : Module(name)
{
}

//--------------
// Destructor --
//--------------
PrintEventId::~PrintEventId ()
{
}

/// Method which is called once at the beginning of the job
void 
PrintEventId::beginJob(Env& env)
{
  MsgLog(logger, info, name() << ": in beginJob()");
}

/// Method which is called at the beginning of the run
void 
PrintEventId::beginRun(Env& env)
{
  MsgLog(logger, info, name() << ": in beginRun()");
}

/// Method which is called at the beginning of the calibration cycle
void 
PrintEventId::beginCalibCycle(Env& env)
{
  MsgLog(logger, info, name() << ": in beginCalibCycle()");
}

/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
PrintEventId::event(Event& evt, Env& env)
{
  // get event ID
  shared_ptr<EventId> eventId = evt.get();
  if (not eventId.get()) {
    MsgLog(logger, info, name() << ": event ID not found");    
  } else {
    MsgLog(logger, info, name() << ": event ID: " << *eventId);
  }
}
  
/// Method which is called at the end of the calibration cycle
void 
PrintEventId::endCalibCycle(Env& env)
{
  MsgLog(logger, info, name() << ": in endCalibCycle()");
}

/// Method which is called at the end of the run
void 
PrintEventId::endRun(Env& env)
{
  MsgLog(logger, info, name() << ": in endRun()");
}

/// Method which is called once at the end of the job
void 
PrintEventId::endJob(Env& env)
{
  MsgLog(logger, info, name() << ": in endJob()");
}

} // namespace psana
