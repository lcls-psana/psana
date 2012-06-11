//--------------------------------------------------------------------------
// File and Version Information:
//  $Id$
//
// Description:
//  Class RunIter...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "RunIter.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "Run.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  // type-specific methods
  PyObject* RunIter_iter(PyObject* self);
  PyObject* RunIter_iternext(PyObject* self);

  char typedoc[] = "";

}

//    ----------------------------------------
//    -- Public Function Member Definitions --
//    ----------------------------------------

void
psana::pyext::RunIter::initType(PyObject* module)
{
  PyTypeObject* type = BaseType::typeObject() ;
  type->tp_doc = ::typedoc;
  type->tp_iter = RunIter_iter;
  type->tp_iternext = RunIter_iternext;

  BaseType::initType("RunIter", module);
}

namespace {

PyObject*
RunIter_iter(PyObject* self)
{
  Py_XINCREF(self);
  return self;
}

PyObject*
RunIter_iternext(PyObject* self)
{
  psana::pyext::RunIter* py_this = static_cast<psana::pyext::RunIter*>(self);
  psana::Run run = py_this->m_obj.next();
  if (run) {
    return psana::pyext::Run::PyObject_FromCpp(run);
  } else {
    // stop iteration
    PyErr_SetNone( PyExc_StopIteration );
    return 0;
  }
}

}
