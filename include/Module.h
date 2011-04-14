#ifndef PSANA_MODULE_H
#define PSANA_MODULE_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class Module.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <string>
#include <boost/utility.hpp>

//----------------------
// Base Class Headers --
//----------------------
#include "psana/Configurable.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "PSEnv/Env.h"
#include "PSEvt/Event.h"

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------

// this is not nice thing to do but we want to simplify user's life
// and provide bunch of simple interfaces to our system

namespace psana {}
using namespace psana;
using namespace PSEnv;
using namespace PSEvt;
using RootHistoManager::AxisDef;
using boost::shared_ptr;

#if defined(PSANACAT2_)
#undef PSANACAT2_
#endif
#define PSANACAT2_(a,b) a ## b
#define PSANA_MODULE_FACTORY(UserModule) \
  extern "C" \
  psana::Module* \
  PSANACAT2_(_psana_module_,UserModule)(const std::string& name) {\
    return new UserModule(name);\
  }

//		---------------------
// 		-- Class Interface --
//		---------------------


namespace psana {

/**
 *  @brief Base class for user modules in PSANA
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

class Module : boost::noncopyable, protected Configurable {
public:

  /// Event processing status
  enum Status { OK, 
                Skip,   ///< skip all remaining modules for this event
                Stop,   ///< finish with the events
                Abort   ///< abort immediately, no finalization
  };
  
  // Destructor
  virtual ~Module () ;

  /// get the name of the module
  using Configurable::name;
  
  /// get the class name of the module
  using Configurable::className;
  
  /// Method which is called once at the beginning of the job
  virtual void beginJob(Env& env);
  
  /// Method which is called at the beginning of the run
  virtual void beginRun(Env& env);
  
  /// Method which is called at the beginning of the calibration cycle
  virtual void beginCalibCycle(Env& env);
  
  /// Method which is called with event data
  virtual void event(Event& evt, Env& env) = 0;
  
  /// Method which is called at the end of the calibration cycle
  virtual void endCalibCycle(Env& env);

  /// Method which is called at the end of the run
  virtual void endRun(Env& env);

  /// Method which is called once at the end of the job
  virtual void endJob(Env& env);

  /// reset module status
  void reset() { m_status = OK; }
  
  /// get status
  Status status() const { return m_status; }
  
protected:

  // constructor needs module name
  Module (const std::string& name) ;

  /// change the status
  void skip() { m_status = Skip; }
  void stop() { m_status = Stop; }
  void abort() { m_status = Abort; }

private:

  // Data members
  Status m_status;

};

// formatting for enum
std::ostream&
operator<<(std::ostream& out, Module::Status stat);

} // namespace psana

#endif // PSANA_MODULE_H
