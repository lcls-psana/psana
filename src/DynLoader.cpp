//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class DynLoader...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/DynLoader.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <dlfcn.h>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "psana/Exceptions.h"
#include "MsgLogger/MsgLogger.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {
  
  const char logger[] = "DynLoader";
  
  typedef psana::Module* (*mod_factory)(const std::string& name);
  typedef psana::InputModule* (*input_mod_factory)(const std::string& name);
}

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace psana {

/**
 *  Load one user module. The name of the module has a format 
 *  Package.Class[:name]
 */
Module* 
DynLoader::loadModule(const std::string& name) const
{
  // make class name, use psana for package name if not given
  std::string fullName = name;
  std::string className = name;
  std::string::size_type p1 = className.find(':');
  if (p1 != std::string::npos) {
    className.erase(p1);
  }
  if (className.find('.') == std::string::npos) {
    className = "psana." + className;
    fullName = "psana." + fullName;
  }
  
  // Load function
  void* sym = loadFactoryFunction(className, "_psana_module_");
  ::mod_factory factory = (::mod_factory)sym;
  
  // call factory function
  return factory(fullName);
}

/**
 *  Load one input module. The name of the module has a format 
 *  Package.Class[:name]
 */
InputModule* 
DynLoader::loadInputModule(const std::string& name) const
{
  // make class name, use psana for package name if not given
  std::string fullName = name;
  std::string className = name;
  std::string::size_type p1 = className.find(':');
  if (p1 != std::string::npos) {
    className.erase(p1);
  }
  if (className.find('.') == std::string::npos) {
    className = "psana." + className;
    fullName = "psana." + fullName;
  }
  
  // Load function
  void* sym = loadFactoryFunction(className, "_psana_input_module_");
  ::input_mod_factory factory = (::input_mod_factory)sym;
  
  // call factory function
  return factory(fullName);
}

void* 
DynLoader::loadFactoryFunction(const std::string& name, const std::string& factory) const
{
  // get package name and module class name
  std::string::size_type p1 = name.find('.');
  if (p1 == std::string::npos) throw ExceptionModuleName(ERR_LOC, name);
  std::string package(name, 0, p1);
  std::string className(name, p1+1);

  // load the library
  void* ldh = loadPackageLib(package);
  
  // find the symbol
  std::string symname = factory + className;
  void* sym = dlsym(ldh, symname.c_str());
  if ( not sym ) {
    throw ExceptionDlerror(ERR_LOC, "failed to locate symbol "+symname);
  }
  
  return sym;
}

/**
 *  Load the library for a package 
 */
void* 
DynLoader::loadPackageLib(const std::string& packageName) const
{
  // build library name
  std::string lib = "lib" + packageName + ".so";
  
  // load the library
  MsgLog(logger, trace, "loading library " << lib);
  void* ldh = dlopen(lib.c_str(), RTLD_NOW | RTLD_GLOBAL);
  if ( not ldh ) {
    throw ExceptionDlerror(ERR_LOC, "failed to load dynamic library "+lib);
  }
  
  return ldh;
}

} // namespace psana
