#ifndef PSANA_PRINTEVENTID_H
#define PSANA_PRINTEVENTID_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class PrintEventId.
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
 *  @brief Example module class for psana
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

class PrintEventId : public Module {
public:

  // Default constructor
  PrintEventId (const std::string& name) ;

  // Destructor
  virtual ~PrintEventId () ;

  /// Method which is called once at the beginning of the job
  virtual void beginJob(Env& env);
  
  /// Method which is called at the beginning of the run
  virtual void beginRun(Env& env);
  
  /// Method which is called at the beginning of the calibration cycle
  virtual void beginCalibCycle(Env& env);
  
  /// Method which is called with event data, this is the only required 
  /// method, all other methods are optional
  virtual void event(Event& evt, Env& env);
  
  /// Method which is called at the end of the calibration cycle
  virtual void endCalibCycle(Env& env);

  /// Method which is called at the end of the run
  virtual void endRun(Env& env);

  /// Method which is called once at the end of the job
  virtual void endJob(Env& env);

protected:

private:
};

} // namespace psana

#endif // PSANA_PRINTEVENTID_H
