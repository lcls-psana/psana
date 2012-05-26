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
#include "psddl_pypsana/PyWrapper.h"
#include <cstdio>
#include <cctype>

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
void PyWrapperModule::checkMethodName(char* pyanaMethodName, char* psanaMethodName) {
  if (PyObject_GetAttrString(m_instance, pyanaMethodName)) {
    fprintf(stderr, "Error: module %s defines pyana-style methods (e.g. \"%s\") instead of psana-style methods (e.g. \"%s\").\n", name().c_str(), pyanaMethodName, psanaMethodName);
    exit(1);
  }
}

static PyObject* getMethodByName(PyObject* instance, char* name) {
  PyObject* method = PyObject_GetAttrString(instance, name);
  if (method == NULL) {
    const int size = strlen(name) + 1;
    char lname[size];
    for (int i = 0; i < size; i++) {
      lname[i] = tolower(name[i]);
    }
    method = PyObject_GetAttrString(instance, lname);
  }
  return method;
}

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
#if 0
  // make sure no pyana-style methods are defined
  checkMethodName("beginjob", "beginJob");
  checkMethodName("beginrun", "beginRun");
  checkMethodName("begincalibcycle", "beginCalibCycle");
  checkMethodName("endcalibcycle", "endCalibCycle");
  checkMethodName("endrun", "endRun");
  checkMethodName("endjob", "endJob");

  // get all methods
  m_beginJob = PyObject_GetAttrString(m_instance, "beginJob");
  m_beginRun = PyObject_GetAttrString(m_instance, "beginRun");
  m_beginCalibCycle = PyObject_GetAttrString(m_instance, "beginCalibCycle");
  m_event = PyObject_GetAttrString(m_instance, "event");
  m_endCalibCycle = PyObject_GetAttrString(m_instance, "endCalibCycle");
  m_endRun = PyObject_GetAttrString(m_instance, "endRun");
  m_endJob = PyObject_GetAttrString(m_instance, "endJob");
#else
  m_beginJob = getMethodByName(m_instance, "beginJob");
  m_beginRun = getMethodByName(m_instance, "beginRun");
  m_beginCalibCycle = getMethodByName(m_instance, "beginCalibCycle");
  m_event = getMethodByName(m_instance, "event");
  m_endCalibCycle = getMethodByName(m_instance, "endCalibCycle");
  m_endRun = getMethodByName(m_instance, "endRun");
  m_endJob = getMethodByName(m_instance, "endJob");
#endif

  Psana::createWrappers();
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

  PyObjPtr res(Psana::call(method, evt, env, name(), className()));
  if (not res) {
    PyErr_Print();
    throw ExceptionGenericPyError(ERR_LOC, "exception raised while calling Python method");
  }
}


} // namespace psana
