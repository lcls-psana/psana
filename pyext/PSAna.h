#ifndef PSANA_PYEXT_PSANA_H
#define PSANA_PYEXT_PSANA_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class BldInfo.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <boost/shared_ptr.hpp>

//----------------------
// Base Class Headers --
//----------------------
#include "PyDataType.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------
#include "psana/PSAna.h"

//    ---------------------
//    -- Class Interface --
//    ---------------------

namespace psana {
namespace pyext {

/**
 *  This software was developed for the LUSI project.  If you use all or
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @version $Id$
 *
 *  @author Andrei Salnikov
 */

class PSAna : public PyDataType<PSAna, boost::shared_ptr<psana::PSAna> > {
public:

  typedef PyDataType<PSAna, boost::shared_ptr<psana::PSAna> > BaseType;

  /// Initialize Python type and register it in a module
  static void initType( PyObject* module );

};

} // namespace pyext
} // namespace psana

#endif // PSANA_PYEXT_PSANA_H
