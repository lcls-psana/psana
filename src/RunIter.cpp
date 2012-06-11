//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class RunIter...
//
// Author List:
//      Andy Salnikov
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
RunIter::RunIter ()
  : m_evtLoop()
{
}

/// Constructor takes event loop instance.
RunIter::RunIter (const boost::shared_ptr<EventLoop>& evtLoop)
  : m_evtLoop(evtLoop)
{
}

//--------------
// Destructor --
//--------------
RunIter::~RunIter ()
{
}

/// get next scan, when done returns object which is convertible to "false"
RunIter::value_type 
RunIter::next()
{
  RunIter::value_type result;

  // Go to a BeginRun transition
  while (true) {
    EventLoop::value_type nxt = m_evtLoop->next();
    if (nxt.first == EventLoop::None) {
      // nothing left there
      break;
    } else if (nxt.first == EventLoop::BeginRun) {
      // found it
      result = RunIter::value_type(m_evtLoop);
      break;
    }
  }

  return result;
}

} // namespace psana
