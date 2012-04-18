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
  // make class name and module name, use psana for package name if not given
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
  PyObjPtr args(PyTuple_New(1), PyRefDelete());
  PyTuple_SET_ITEM(args.get(), 0, PyString_FromString(fullName.c_str()));
  PyObject* instance = PyObject_Call(cls.get(), args.get(), NULL);
  if (not instance) {
    throw ExceptionPyLoadError(ERR_LOC, "error making an instance of class " + className + ": " + ::pyExcStr());
  }

  // check that instance has at least an event() method
  if (not PyObject_HasAttrString(instance, "event")) {
    Py_CLEAR(instance);
    throw ExceptionPyLoadError(ERR_LOC, "Python class " + className + " does not define event() method");
  }

  return boost::make_shared<PyWrapperModule>(fullName, instance);
}

} // namespace psana
