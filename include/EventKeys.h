#ifndef PSANA_EVENTKEYS_H
#define PSANA_EVENTKEYS_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class EventKeys.
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
 *  @brief Example module class for psana which dumps the list of keys in event
 *
 *  This software was developed for the LCLS project.  If you use all or 
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @see AdditionalClass
 *
 *  @version \$Id$
 *
 *  @author Andrei Salnikov
 */

class EventKeys : public Module {
public:

  // Default constructor
  EventKeys (const std::string& name) ;

  // Destructor
  virtual ~EventKeys () ;

  /// Method which is called once at the beginning of the job
  virtual void beginJob(Event& evt, Env& env);
  
  /// Method which is called at the beginning of the run
  virtual void beginRun(Event& evt, Env& env);
  
  /// Method which is called at the beginning of the calibration cycle
  virtual void beginCalibCycle(Event& evt, Env& env);

  /// Method which is called with event data
  virtual void event(Event& evt, Env& env);

  /// Method which is called at the end of the calibration cycle
  virtual void endCalibCycle(Event& evt, Env& env);

  /// Method which is called at the end of the run
  virtual void endRun(Event& evt, Env& env);

  /// Method which is called once at the end of the job
  virtual void endJob(Event& evt, Env& env);

protected:

private:

  /// Flag: true - print in event() info about env.configStore().keys()
  bool m_print_cfg_in_evt;

  /// Flag: true - print in event() info about env.calibStore().keys()
  bool m_print_clb_in_evt;
};

} // namespace psana

#endif // PSANA_EVENTKEYS_H
