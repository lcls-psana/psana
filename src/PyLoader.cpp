//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class PyLoader...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/PyLoader.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <boost/make_shared.hpp>
#include "python/Python.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "psana/Exceptions.h"
#include "psana/PyWrapperModule.h"
#include "MsgLogger/MsgLogger.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  const char logger[] = "PyLoader";

  // return string describing Python exception
  std::string pyExcStr()
  {
    PyObject *ptype;
    PyObject *pvalue;
    PyObject *ptraceback;

    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyObject* errstr = PyObject_Str(pvalue);
    std::string msg = PyString_AsString(errstr);

    Py_CLEAR(errstr);
    Py_CLEAR(ptype);
    Py_CLEAR(pvalue);
    Py_CLEAR(ptraceback);

    return msg;
  }

  struct PyRefDelete {
    void operator()(PyObject* obj) { Py_CLEAR(obj); }
  };
  typedef boost::shared_ptr<PyObject> PyObjPtr;

}

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace psana {

// Load one user module. The name of the module has a format [Package.]Class[:name]
boost::shared_ptr<Module>
PyLoader::loadModule(const std::string& name) const
{
  // Make class name and module name. Use psana for package name if not given.
  // Full name should be package name . class name.
  std::string fullName = name;
  std::string moduleName = name;
  std::string::size_type p1 = moduleName.find(':');
  if (p1 != std::string::npos) {
    moduleName.erase(p1);
  }
  std::string className = moduleName;
  p1 = className.find('.');
  if (p1 == std::string::npos) {
    moduleName = "psana." + moduleName;
    fullName = "psana." + fullName;
  } else {
    className.erase(0, p1+1);
  }

  MsgLog(logger, debug, "names: module=" << moduleName << " class=" << className << " full=" << fullName);

  // Make sure python is initialized
  Py_Initialize();

  // try to import module
  PyObjPtr mod(PyImport_ImportModule((char*)moduleName.c_str()), PyRefDelete());
  if (not mod) {
    throw ExceptionPyLoadError(ERR_LOC, "failed to import module " + moduleName + ": " + ::pyExcStr());
  }

  // there must be a class defined with this name
  PyObjPtr cls(PyObject_GetAttrString(mod.get(), (char*)className.c_str()), PyRefDelete());
  if (not cls) {
    throw ExceptionPyLoadError(ERR_LOC, "Python module " + moduleName + " does not define class " + className);
  }

  // make sure class (or whatever else) is callable
  if (not PyCallable_Check(cls.get())) {
    throw ExceptionPyLoadError(ERR_LOC, "Python object " + moduleName + " cannot be instantiated (is not callable)");
  }

  // make an instance

  // Create empty positional args list.
  PyObjPtr args(PyTuple_New(0), PyRefDelete());

  // Create keyword args list.
  PyObjPtr kwargs(PyDict_New(), PyRefDelete());
  ConfigSvc::ConfigSvc cfg;
  list<string> keys = cfg.getKeys(fullName);
  list<string>::iterator it;
  for (it = keys.begin(); it != keys.end(); it++) {
    const string& key = *it;
    const char* value = cfg.getStr(fullName, key).c_str();
    PyDict_SetItemString(kwargs.get(), key.c_str(), PyString_FromString(value));
  }

  // Create the instance by calling the constructor
  PyObject* instance = PyObject_Call(cls.get(), args.get(), kwargs.get());
  if (not instance) {
    throw ExceptionPyLoadError(ERR_LOC, "error making an instance of class " + className + ": " + ::pyExcStr());
  }

  // Set m_className and m_fullName attributes.
  PyObject_SetAttr(instance, PyString_FromString("m_className"), PyString_FromString(className.c_str()));
  PyObject_SetAttr(instance, PyString_FromString("m_fullName"), PyString_FromString(fullName.c_str()));

  // check that instance has at least an event() method
  if (not PyObject_HasAttrString(instance, "event")) {
    Py_CLEAR(instance);
    throw ExceptionPyLoadError(ERR_LOC, "Python class " + className + " does not define event() method");
  }

  return boost::make_shared<PyWrapperModule>(fullName, instance);
}

} // namespace psana
