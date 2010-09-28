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

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "ConfigSvc/ConfigSvc.h"

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------


// this is not nice thing to do but we want to simplify user's life
// and provide bunch of simple interfaces to our system

namespace psana {}
using namespace psana;

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
 *  Base class for user modules in PSANA
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

class Module : boost::noncopyable {
public:

  // event processing status
  enum Status { OK, 
                Skip,   // skip all remaining modules for this event
                Stop,   // finish with the events
                Abort   // abort immediately, no finalization
  };
  
  // Destructor
  virtual ~Module () ;

  /// get the name of the module
  const std::string& name() const ;
  
  /// get the class name of the module
  const std::string& className() const ;
  
  /// Method which is called with event data
  virtual void event(/*Event& evt, Env& env*/) = 0;
  
  // reset module status
  void reset() { m_status = OK; }
  
  // get status
  Status status() const { return m_status; }
  
protected:

  // constructor needs module name
  Module (const std::string& name) ;

  // module interface to configuration service

  // get the value of a single parameter, will throw if parameter is not there
  ConfigSvc::ConfigSvc::Result config(const std::string& param) const
  {
    ConfigSvc::ConfigSvc cfg;
    try {
      return cfg.get(name(), param);
    } catch (const ConfigSvc::ExceptionMissing& ex) {
      return cfg.get(className(), param);
    }
  }

  // get the value of a single parameter, use default if not there
  template <typename T>
  T config(const std::string& param, const T& def) const
  {
    ConfigSvc::ConfigSvc cfg;
    try {
      return cfg.get(name(), param);
    } catch (const ConfigSvc::ExceptionMissing& ex) {
      return cfg.get<T>(className(), param, def);
    }    
  }

  // get the value of a single parameter as sequence, will throw if parameter is not there
  ConfigSvc::ConfigSvc::ResultList configList(const std::string& param) const
  {
    ConfigSvc::ConfigSvc cfg;
    try {
      return cfg.getList(name(), param);
    } catch (const ConfigSvc::ExceptionMissing& ex) {
      return cfg.getList(className(), param);
    }
  }
  
  // get the value of a single parameter as sequence, or return default value 
  template <typename T>
  std::list<T> configList(const std::string& param, const std::list<T>& def) const
  {
    ConfigSvc::ConfigSvc cfg;
    try {
      return cfg.getList(name(), param);
    } catch (const ConfigSvc::ExceptionMissing& ex) {
      return cfg.getList<T>(className(), param, def);
    }
  }

  // change the status
  void skip() { m_status = Skip; }
  void stop() { m_status = Stop; }
  void abort() { m_status = Abort; }

private:

  // Data members
  std::string m_name;
  std::string m_className;
  Status m_status;

};

} // namespace psana

#endif // PSANA_MODULE_H
