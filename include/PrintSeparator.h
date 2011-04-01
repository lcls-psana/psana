#ifndef PSANA_PRINTSEPARATOR_H
#define PSANA_PRINTSEPARATOR_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class PrintSeparator.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------

//----------------------
// Base Class Headers --
//----------------------
#include "psana/Module.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------

//		---------------------
// 		-- Class Interface --
//		---------------------

namespace psana {

/**
 *  @brief Simple psana module that only prints separator line on every event
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

class PrintSeparator : public Module {
public:

  // Default constructor
  PrintSeparator (const std::string& name) ;

  // Destructor
  virtual ~PrintSeparator () ;

  /// Method which is called with event data, this is the only required 
  /// method, all other methods are optional
  virtual void event(Event& evt, Env& env);
  
protected:

private:

  std::string m_separator;

};

} // namespace psana

#endif // PSANA_PRINTSEPARATOR_H
