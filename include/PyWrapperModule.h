#ifndef PSANA_PYWRAPPERMODULE_H
#define PSANA_PYWRAPPERMODULE_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class PyWrapperModule.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include "python/Python.h"

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

/// @addtogroup psana

/**
 *  @ingroup psana
 *
 *  @brief Psana module class which wraps a Python class
 *
 *  This software was developed for the LCLS project.  If you use all or 
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @version \$Id$
 *
 *  @author Andy Salnikov
 */

class PyWrapperModule : public Module {
public:

  // Default constructor
  PyWrapperModule (const std::string& name, PyObject* instance) ;

  // Destructor
  virtual ~PyWrapperModule () ;

  /// Method which is called once at the beginning of the job
  virtual void beginJob(Event& evt, Env& env);
  
  /// Method which is called at the beginning of the run
  virtual void beginRun(Event& evt, Env& env);
  
  /// Method which is called at the beginning of the calibration cycle
  virtual void beginCalibCycle(Event& evt, Env& env);
  
  /// Method which is called with event data, this is the only required 
  /// method, all other methods are optional
  virtual void event(Event& evt, Env& env);
  
  /// Method which is called at the end of the calibration cycle
  virtual void endCalibCycle(Event& evt, Env& env);

  /// Method which is called at the end of the run
  virtual void endRun(Event& evt, Env& env);

  /// Method which is called once at the end of the job
  virtual void endJob(Event& evt, Env& env);

protected:

  // call specific method
  void call(PyObject* method, Event& evt, Env& env);

private:

  PyObject* m_instance;   // Instance of Python class
  PyObject* m_beginJob;
  PyObject* m_beginRun;
  PyObject* m_beginCalibCycle;
  PyObject* m_event;
  PyObject* m_endCalibCycle;
  PyObject* m_endRun;
  PyObject* m_endJob;
};

} // namespace psana

#endif // PSANA_PYWRAPPERMODULE_H
