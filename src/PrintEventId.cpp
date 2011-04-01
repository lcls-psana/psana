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
  MsgLog(name(), info, "in beginJob()");
}

/// Method which is called at the beginning of the run
void 
PrintEventId::beginRun(Env& env)
{
  MsgLog(name(), info, "in beginRun()");
}

/// Method which is called at the beginning of the calibration cycle
void 
PrintEventId::beginCalibCycle(Env& env)
{
  MsgLog(name(), info, "in beginCalibCycle()");
}

/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
PrintEventId::event(Event& evt, Env& env)
{
  // get event ID
  shared_ptr<EventId> eventId = evt.get();
  if (not eventId.get()) {
    MsgLog(name(), info, "event ID not found");
  } else {
    MsgLog(name(), info, "event ID: " << *eventId);
  }
}
  
/// Method which is called at the end of the calibration cycle
void 
PrintEventId::endCalibCycle(Env& env)
{
  MsgLog(name(), info, "in endCalibCycle()");
}

/// Method which is called at the end of the run
void 
PrintEventId::endRun(Env& env)
{
  MsgLog(name(), info, "in endRun()");
}

/// Method which is called once at the end of the job
void 
PrintEventId::endJob(Env& env)
{
  MsgLog(name(), info, "in endJob()");
}

} // namespace psana
