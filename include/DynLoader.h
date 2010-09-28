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
  
protected:

private:

};

} // namespace psana

#endif // PSANA_DYNLOADER_H
