//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class EventIter...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/EventIter.h"

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
EventIter::EventIter ()
  : m_evtLoop()
  , m_stopType(EventLoop::None)
{
}

EventIter::EventIter (const boost::shared_ptr<EventLoop>& evtLoop, EventLoop::EventType stopType)
  : m_evtLoop(evtLoop)
  , m_stopType(stopType)
{
}

//--------------
// Destructor --
//--------------
EventIter::~EventIter ()
{
}

/// get next event, returns zero pointer when done
boost::shared_ptr<PSEvt::Event>
EventIter::next()
{
  boost::shared_ptr<PSEvt::Event> result;
  if (m_stopType == EventLoop::Event) {
    // means iteration already finished
    return result;
  }

  const EventLoop::value_type& nxt = m_evtLoop->next();
  if (nxt.first == EventLoop::None or nxt.first == m_stopType) {
    // we stop here
    m_stopType = EventLoop::Event;
  } else {
    result = nxt.second;
  }

  return result;
}

} // namespace psana
