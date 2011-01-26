#ifndef PSANA_CONFIGURABLE_H
#define PSANA_CONFIGURABLE_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class Configurable.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <string>

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

//		---------------------
// 		-- Class Interface --
//		---------------------

namespace psana {

/**
 *  @brief class representing an object that can be configured.
 *  
 *  This object can be used either as a base class or a member of other 
 *  classes. It does not define any virtual methods (even destructor)
 *  so make sure that classes which inherit it do it properly.
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

class Configurable  {
public:

  /**
   *  @brief Constructor takes a name.
   *  
   *  It accept names in the format "ClassName" or "ClassName:InstanceName".
   *  
   *  @param[in] name Name of this configurable.
   */
  Configurable (const std::string& name) ;

  // Destructor
  ~Configurable();

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
  std::string configStr(const std::string& param) const
  {
    ConfigSvc::ConfigSvc cfg;
    try {
      return cfg.getStr(name(), param);
    } catch (const ConfigSvc::ExceptionMissing& ex) {
      return cfg.getStr(className(), param);
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
  std::string configStr(const std::string& param, const std::string& def) const
  {
    ConfigSvc::ConfigSvc cfg;
    try {
      return cfg.getStr(name(), param);
    } catch (const ConfigSvc::ExceptionMissing& ex) {
      return cfg.getStr(className(), param, def);
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

  /// Get the full name of the object including class and instance name.
  const std::string& name() const { return m_name; }
  
  /// Get the class name of the object.
  const std::string& className() const { return m_className; }

protected:

private:

  // Data members
  std::string m_name;
  std::string m_className;

};

} // namespace psana

#endif // PSANA_CONFIGURABLE_H
