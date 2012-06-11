//--------------------------------------------------------------------------
// File and Version Information:
//  $Id$
//
// Description:
//  Class Scan...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "Scan.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "EventIter.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  // type-specific methods
  PyObject* Scan_events(PyObject* self, PyObject*);
  PyObject* Scan_nonzero(PyObject* self, PyObject*);

  PyMethodDef methods[] = {
    { "events",      Scan_events,    METH_NOARGS, "self.events() -> iterator\n\nReturns iterator for contained events" },
    { "__nonzero__", Scan_nonzero,   METH_NOARGS, "self.__nonzero__() -> bool\n\nReturns true for non-null object" },
    {0, 0, 0, 0}
   };

  char typedoc[] = "";

}

//    ----------------------------------------
//    -- Public Function Member Definitions --
//    ----------------------------------------

void
psana::pyext::Scan::initType(PyObject* module)
{
  PyTypeObject* type = BaseType::typeObject() ;
  type->tp_doc = ::typedoc;
  type->tp_methods = ::methods;

  BaseType::initType("Scan", module);
}

namespace {

PyObject*
Scan_events(PyObject* self, PyObject* )
{
  psana::pyext::Scan* py_this = static_cast<psana::pyext::Scan*>(self);
  return psana::pyext::EventIter::PyObject_FromCpp(py_this->m_obj.events());
}

PyObject*
Scan_nonzero(PyObject* self, PyObject* )
{
  psana::pyext::Scan* py_this = static_cast<psana::pyext::Scan*>(self);
  return PyBool_FromLong(long(bool(py_this->m_obj)));
}

}
