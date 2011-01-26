#ifndef PSANA_INPUTMODULE_H
#define PSANA_INPUTMODULE_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class InputModule.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <string>
#include <iosfwd>
#include <boost/utility.hpp>

//----------------------
// Base Class Headers --
//----------------------
#include "psana/Configurable.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "PsEnv/Env.h"
#include "PsEvt/Event.h"

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------

// this is not nice thing to do but we want to simplify user's life
// and provide bunch of simple interfaces to our system

namespace psana {}
using namespace psana;
using namespace PsEnv;
using namespace PsEvt;

#if defined(PSANACAT2_)
#undef PSANACAT2_
#endif
#define PSANACAT2_(a,b) a ## b
#define PSANA_INPUT_MODULE_FACTORY(UserModule) \
  extern "C" \
  psana::InputModule* \
  PSANACAT2_(_psana_input_module_,UserModule)(const std::string& name) {\
    return new UserModule(name);\
  }

//		---------------------
// 		-- Class Interface --
//		---------------------

namespace psana {

/**
 *  @brief Base class for PSANA input modules.
 *
 *  This software was developed for the LCLS project.  If you use all or 
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @see AdditionalClass
 *
 *  @version $Id$
 *
 *  @author Andrei Salnikov
 */

class InputModule : boost::noncopyable, protected Configurable {
public:

  /// Event processing status
  enum Status { BeginRun,
                BeginCalibCycle,
                DoEvent,
                EndCalibCycle,
                EndRun,
                Skip,   ///< skip all remaining modules for this event
                Stop,   ///< finish with the events
                Abort   ///< abort immediately, no finalization
  };

  // Destructor
  virtual ~InputModule () ;

  /// get the name of the module
  using Configurable::name;
  
  /// get the class name of the module
  using Configurable::className;
  
  /// Method which is called once at the beginning of the job
  virtual void beginJob(Env& env);
  
  /// Method which is called with event data
  virtual Status event(Event& evt, Env& env) = 0;
  
  /// Method which is called once at the end of the job
  virtual void endJob(Env& env);
  
protected:

  // Standard constructor
  InputModule (const std::string& name) ;

private:

  // Data members
  
};

// formatting for enum
std::ostream&
operator<<(std::ostream& out, InputModule::Status stat);

} // namespace psana

#endif // PSANA_INPUTMODULE_H
