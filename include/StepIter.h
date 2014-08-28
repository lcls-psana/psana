#ifndef PSANA_STEPITER_H
#define PSANA_STEPITER_H

//--------------------------------------------------------------------------
// File and Version Information:
//     $Id$
//
// Description:
//     Class StepIter.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <boost/shared_ptr.hpp>

//----------------------
// Base Class Headers --
//----------------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "psana/Step.h"

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------
namespace psana {
class EventLoop;
}

//             ---------------------
//             -- Class Interface --
//             ---------------------

namespace psana {

/// @addtogroup psana

/**
 *  @ingroup psana
 *
 *  @brief Class representing iterator over steps (calib cycles)
 *
 *
 *  This software was developed for the LCLS project.  If you use all or 
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @version $Id$
 *
 *  @author Andy Salnikov
 */

class StepIter  {
public:

  typedef Step value_type;
    
  /// Default constructor makes invalid iterator
  StepIter () ;

  /**
   *  @brief Constructor takes event loop instance and "stop event type".
   *
   *  Do not use EventLoop::Event for stop type, first it does not make
   *  sense, second this iterator uses it for special purpose.
   */
  StepIter (const boost::shared_ptr<EventLoop>& evtLoop, EventLoop::EventType stopType);

  // Destructor
  ~StepIter () ;

  /// get next step, when done returns object which is convertible to "false"
  value_type next();

  /// get next step and corresonding event. When done returns object which is convertible to "false"
  std::pair<value_type, boost::shared_ptr<PSEvt::Event> > nextWithEvent();

protected:

private:

  // Data members
  boost::shared_ptr<EventLoop> m_evtLoop;
  EventLoop::EventType m_stopType;

};

} // namespace psana

#endif // PSANA_STEPITER_H
