#ifndef PSANA_DYNLOADER_H
#define PSANA_DYNLOADER_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class DynLoader.
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
#include "psana/Module.h"
#include "psana/InputModule.h"

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------

//		---------------------
// 		-- Class Interface --
//		---------------------

namespace psana {

/**
 *  Class which can load modules from dynamic libraries.
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

class DynLoader  {
public:

  /**
   *  Load one user module. The name of the module has a format 
   *  Package.Class[:name]
   */
  Module* loadModule(const std::string& name) const;
  
  /**
   *  Load one input module. The name of the module has a format 
   *  Package.Class[:name]
   */
  InputModule* loadInputModule(const std::string& name) const;
  
protected:

  /**
   *  Load the library for a package 
   *  
   *  @param[in] packageName  Package name.
   *  @return Library handle.
   */
  void* loadPackageLib(const std::string& packageName) const;
  
  /**
   *  Load the library and find factory symbol
   *  
   *  @param[in] name String in the same format as accepted by loadModule().
   *  @param[in] factory Prefix for factory function name, like "_psana_module_"
   *  @return pointer to factory function.
   *  
   *  @throw ExceptionModuleName
   *  @throw ExceptionDlerror
   */
  void* loadFactoryFunction(const std::string& name, const std::string& factory) const;
  
private:

};

} // namespace psana

#endif // PSANA_DYNLOADER_H
