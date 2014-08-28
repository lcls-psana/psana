//--------------------------------------------------------------------------
// File and Version Information:
//     $Id$
//
// Description:
//     Class RunIter...
//
// Author List:
//     Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/RunIter.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "PSEvt/EventId.h"
#include "psana/Exceptions.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

//             ----------------------------------------
//             -- Public Function Member Definitions --
//             ----------------------------------------

namespace psana {

//----------------
// Constructors --
//----------------
RunIter::RunIter ()
  : m_evtLoop()
{
}

/// Constructor takes event loop instance.
RunIter::RunIter (const boost::shared_ptr<EventLoop>& evtLoop)
  : m_evtLoop(evtLoop)
{
  try {
    _runIter = m_evtLoop->index().runs().begin();
  }
  catch (ExceptionAbort e) {
  }
}

//--------------
// Destructor --
//--------------
RunIter::~RunIter ()
{
}

/// get next run, when done returns object which is convertible to "false"
std::pair<RunIter::value_type, boost::shared_ptr<PSEvt::Event> > 
RunIter::nextWithEvent()
{
  std::pair<RunIter::value_type, boost::shared_ptr<PSEvt::Event> > result;

  try {
    if (_runIter == m_evtLoop->index().runs().end()) return result;
    m_evtLoop->index().setrun(*_runIter);
    _runIter++;
  }
  catch (ExceptionAbort e) {
  }

  // Go to a BeginRun transition
  while (true) {
    EventLoop::value_type nxt = m_evtLoop->next();
    if (nxt.first == EventLoop::None) {
      // nothing left there
      break;
    } else if (nxt.first == EventLoop::BeginRun) {
      // found it, try to get run number from current event
      boost::shared_ptr<PSEvt::EventId> eid = nxt.second->get();
      int run = eid ? eid->run() : -1 ;
      result = std::pair<RunIter::value_type, 
        boost::shared_ptr<PSEvt::Event> >(RunIter::value_type(m_evtLoop, run), nxt.second);
      break;
    }
  }

  return result;
}

RunIter::value_type
RunIter::next()
{
  return nextWithEvent().first;
}

} // namespace psana
