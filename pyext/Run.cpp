//--------------------------------------------------------------------------
// File and Version Information:
//  $Id$
//
// Description:
//  Class Run...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "Run.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "EventIter.h"
#include "ScanIter.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  // type-specific methods
  PyObject* Run_scans(PyObject* self, PyObject*);
  PyObject* Run_events(PyObject* self, PyObject*);
  PyObject* Run_nonzero(PyObject* self, PyObject*);

  PyMethodDef methods[] = {
    { "scans",       Run_scans,     METH_NOARGS, "self.scans() -> iterator\n\nReturns iterator for contained scans" },
    { "events",      Run_events,    METH_NOARGS, "self.events() -> iterator\n\nReturns iterator for contained events" },
    { "__nonzero__", Run_nonzero,   METH_NOARGS, "self.__nonzero__() -> bool\n\nReturns true for non-null object" },
    {0, 0, 0, 0}
   };

  char typedoc[] = "";

}

//    ----------------------------------------
//    -- Public Function Member Definitions --
//    ----------------------------------------

void
psana::pyext::Run::initType(PyObject* module)
{
  PyTypeObject* type = BaseType::typeObject() ;
  type->tp_doc = ::typedoc;
  type->tp_methods = ::methods;

  BaseType::initType("Run", module);
}

namespace {

PyObject*
Run_scans(PyObject* self, PyObject* )
{
  psana::pyext::Run* py_this = static_cast<psana::pyext::Run*>(self);
  return psana::pyext::ScanIter::PyObject_FromCpp(py_this->m_obj.scans());
}

PyObject*
Run_events(PyObject* self, PyObject* )
{
  psana::pyext::Run* py_this = static_cast<psana::pyext::Run*>(self);
  return psana::pyext::EventIter::PyObject_FromCpp(py_this->m_obj.events());
}

PyObject*
Run_nonzero(PyObject* self, PyObject* )
{
  psana::pyext::Run* py_this = static_cast<psana::pyext::Run*>(self);
  return PyBool_FromLong(long(bool(py_this->m_obj)));
}

}
