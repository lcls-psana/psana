//--------------------------------------------------------------------------
// File and Version Information:
//     $Id$
//
// Description:
//     Class Step...
//
// Author List:
//     Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/Step.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

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
Step::Step ()
  : m_evtLoop()
{
}

// Constructor takes event loop object
Step::Step(const boost::shared_ptr<EventLoop>& evtLoop)
  : m_evtLoop(evtLoop)
{
}

//--------------
// Destructor --
//--------------
Step::~Step ()
{
}

/// Get environment object, cannot be called for "null" source
PSEnv::Env&
Step::env() const
{
  return m_evtLoop->env();
}


} // namespace psana
