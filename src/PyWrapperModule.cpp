//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class PyWrapperModule...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

// Set this to 1 to use (and create dependency on) the psddl_pypsana package.
#define PSDDL_PYPSANA_SUPPORT 1

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/PyWrapperModule.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "psana/Exceptions.h"
#include "PSEvt/EventId.h"
#if PSDDL_PYPSANA_SUPPORT
#include "psddl_pypsana/PyWrapper.h"
#endif

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  const char logger[] = "PyWrapperModule";

  struct PyRefDelete {
    void operator()(PyObject* obj) { Py_CLEAR(obj); }
  };
  typedef boost::shared_ptr<PyObject> PyObjPtr;

}

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace psana {

//----------------
// Constructors --
//----------------
PyWrapperModule::PyWrapperModule (const std::string& name, PyObject* instance)
  : Module(name)
  , m_instance(instance)
  , m_beginJob(0)
  , m_beginRun(0)
  , m_beginCalibCycle(0)
  , m_event(0)
  , m_endCalibCycle(0)
  , m_endRun(0)
  , m_endJob(0)
{
  // get all methods
  m_beginJob = PyObject_GetAttrString(m_instance, "beginJob");
  m_beginRun = PyObject_GetAttrString(m_instance, "beginRun");
  m_beginCalibCycle = PyObject_GetAttrString(m_instance, "beginCalibCycle");
  m_event = PyObject_GetAttrString(m_instance, "event");
  m_endCalibCycle = PyObject_GetAttrString(m_instance, "endCalibCycle");
  m_endRun = PyObject_GetAttrString(m_instance, "endRun");
  m_endJob = PyObject_GetAttrString(m_instance, "endJob");

#if PSDDL_PYPSANA_SUPPORT
  if (! m_beginJob) m_beginJob = PyObject_GetAttrString(m_instance, "beginjob");
  if (! m_beginRun) m_beginRun = PyObject_GetAttrString(m_instance, "beginrun");
  if (! m_beginCalibCycle) m_beginCalibCycle = PyObject_GetAttrString(m_instance, "begincalibcycle");
  if (! m_endCalibCycle) m_endCalibCycle = PyObject_GetAttrString(m_instance, "endcalibcycle");
  if (! m_endRun) m_endRun = PyObject_GetAttrString(m_instance, "endrun");
  if (! m_endJob) m_endJob = PyObject_GetAttrString(m_instance, "endjob");
  Psana::createWrappers();
#endif
}

//--------------
// Destructor --
//--------------
PyWrapperModule::~PyWrapperModule ()
{
  Py_CLEAR(m_instance);
  Py_CLEAR(m_beginJob);
  Py_CLEAR(m_beginRun);
  Py_CLEAR(m_beginCalibCycle);
  Py_CLEAR(m_event);
  Py_CLEAR(m_endCalibCycle);
  Py_CLEAR(m_endRun);
  Py_CLEAR(m_endJob);
}

/// Method which is called once at the beginning of the job
void 
PyWrapperModule::beginJob(Event& evt, Env& env)
{
  call(m_beginJob, evt, env);
}

/// Method which is called at the beginning of the run
void 
PyWrapperModule::beginRun(Event& evt, Env& env)
{
  call(m_beginRun, evt, env);
}

/// Method which is called at the beginning of the calibration cycle
void 
PyWrapperModule::beginCalibCycle(Event& evt, Env& env)
{
  call(m_beginCalibCycle, evt, env);
}

/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
PyWrapperModule::event(Event& evt, Env& env)
{
  call(m_event, evt, env);
}
  
/// Method which is called at the end of the calibration cycle
void 
PyWrapperModule::endCalibCycle(Event& evt, Env& env)
{
  call(m_endCalibCycle, evt, env);
}

/// Method which is called at the end of the run
void 
PyWrapperModule::endRun(Event& evt, Env& env)
{
  call(m_endRun, evt, env);
}

/// Method which is called once at the end of the job
void 
PyWrapperModule::endJob(Event& evt, Env& env)
{
  call(m_endJob, evt, env);
}

// call specific method
void
PyWrapperModule::call(PyObject* method, Event& evt, Env& env)
{
  if (not method) return;

#if PSDDL_PYPSANA_SUPPORT
  PyObjPtr res(Psana::call(method, evt, env));
#else
  PyObjPtr args(PyTuple_New(2), PyRefDelete());
  PyTuple_SET_ITEM(args.get(), 0, PyDict_New());
  PyTuple_SET_ITEM(args.get(), 1, PyDict_New());
  PyObjPtr res(PyObject_Call(method, args.get(), NULL), PyRefDelete());
#endif
  if (not res) {
    PyErr_Print();
    throw ExceptionGenericPyError(ERR_LOC, "exception raised while calling Python method");
  }
}


} // namespace psana
