#ifndef PSANA_PYLOADER_H
#define PSANA_PYLOADER_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class PyLoader.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------

//----------------------
// Base Class Headers --
//----------------------
#include <string>
#include <boost/shared_ptr.hpp>

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

/// @addtogroup psana

/**
 *  @ingroup psana
 *
 *  @brief Class that loads Python psana modules.
 *
 *  This software was developed for the LCLS project.  If you use all or 
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @version $Id$
 *
 *  @author Andy Salnikov
 */

class PyLoader  {
public:

  /**
   *  @brief Load one user module. The name of the module has a format
   *  [Package.]Class[:name]
   */
  boost::shared_ptr<Module> loadModule(const std::string& name) const;

protected:

private:

};

} // namespace psana

#endif // PSANA_PYLOADER_H
