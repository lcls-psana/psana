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
#include "SITConfig/SITConfig.h"

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
  // get package name and module class name
  std::string::size_type p1 = name.find('.');
  if (p1 == std::string::npos) throw ExceptionModuleName(name);
  std::string package(name, 0, p1);
  std::string className;
  std::string::size_type p2 = name.find(':', p1+1);
  if (p2 == std::string::npos) {
    className = name.substr(p1+1);
  } else {
    className = name.substr(p1+1, p2-p1-1);
  }

  // build library name
  std::string lib = "lib" + package + ".so";
  
  // load the library
  MsgLog(logger, trace, "loading library " << lib);
  void* ldh = dlopen(lib.c_str(), RTLD_NOW | RTLD_GLOBAL);
  if ( not ldh ) {
    throw ExceptionDlerror("failed to load dynamic library "+lib);
  }
  
  // find the symbol
  std::string symname = "_psana_module_" + className;
  void* sym = dlsym(ldh, symname.c_str());
  if ( not sym ) {
    throw ExceptionDlerror("failed to locate symbol "+symname);
  }
  
  // call factory function
  ::mod_factory factory = (::mod_factory)sym;
  return factory(name);
}


} // namespace psana
