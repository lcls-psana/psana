//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class StepIter...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/StepIter.h"

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
StepIter::StepIter ()
  : m_evtLoop()
{
}

/// Constructor takes event loop instance.
StepIter::StepIter (const boost::shared_ptr<EventLoop>& evtLoop, EventLoop::EventType stopType)
  : m_evtLoop(evtLoop)
  , m_stopType(stopType)
{
}

//--------------
// Destructor --
//--------------
StepIter::~StepIter ()
{
}

/// get next Step, when done returns object which is convertible to "false"
StepIter::value_type
StepIter::next()
{
  StepIter::value_type result;
  if (m_stopType == EventLoop::Event) {
    // means iteration already finished
    return result;
  }

  while (true) {
    EventLoop::value_type nxt = m_evtLoop->next();
    if (nxt.first == EventLoop::None) {
      // no events left
      m_stopType = EventLoop::Event;
      break;
    } else if (nxt.first == m_stopType) {
      // we stop here, return event back to the stream, someone else
      // may be interested in it
      m_evtLoop->putback(nxt);
      m_stopType = EventLoop::Event;
      break;
    } else if (nxt.first == EventLoop::BeginCalibCycle) {
      // found it
      result = StepIter::value_type(m_evtLoop);
      break;
    }
  }
  return result;
}

} // namespace psana
