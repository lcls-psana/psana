//--------------------------------------------------------------------------
// File and Version Information:
//  $Id$
//
// Description:
//  Class ScanIter...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "ScanIter.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "Scan.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  // type-specific methods
  PyObject* ScanIter_iter(PyObject* self);
  PyObject* ScanIter_iternext(PyObject* self);

  char typedoc[] = "";

}

//    ----------------------------------------
//    -- Public Function Member Definitions --
//    ----------------------------------------

void
psana::pyext::ScanIter::initType(PyObject* module)
{
  PyTypeObject* type = BaseType::typeObject() ;
  type->tp_doc = ::typedoc;
  type->tp_iter = ScanIter_iter;
  type->tp_iternext = ScanIter_iternext;

  BaseType::initType("ScanIter", module);
}

namespace {

PyObject*
ScanIter_iter(PyObject* self)
{
  Py_XINCREF(self);
  return self;
}

PyObject*
ScanIter_iternext(PyObject* self)
{
  psana::pyext::ScanIter* py_this = static_cast<psana::pyext::ScanIter*>(self);
  psana::Scan scan = py_this->m_obj.next();
  if (scan) {
    return psana::pyext::Scan::PyObject_FromCpp(scan);
  } else {
    // stop iteration
    PyErr_SetNone( PyExc_StopIteration );
    return 0;
  }
}

}
