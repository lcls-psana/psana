//--------------------------------------------------------------------------
// File and Version Information:
//  $Id$
//
// Description:
//  Class EventIter...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "EventIter.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  // type-specific methods
  PyObject* EventIter_iter(PyObject* self);
  PyObject* EventIter_iternext(PyObject* self);

  char typedoc[] = "Class which supports iteration over events contained in a "
      "particular :py:class:`DataSource`, :py:class:`Run`, or :py:class:`Scan` "
      "instance. Iterator returns event objects which contain all experimental "
      "data for particular event.";

}

//    ----------------------------------------
//    -- Public Function Member Definitions --
//    ----------------------------------------

void
psana::pyext::EventIter::initType(PyObject* module)
{
  PyTypeObject* type = BaseType::typeObject() ;
  type->tp_doc = ::typedoc;
  type->tp_iter = EventIter_iter;
  type->tp_iternext = EventIter_iternext;

  BaseType::initType("EventIter", module);
}

namespace {

PyObject*
EventIter_iter(PyObject* self)
{
  Py_XINCREF(self);
  return self;
}

PyObject*
EventIter_iternext(PyObject* self)
{
  psana::pyext::EventIter* py_this = static_cast<psana::pyext::EventIter*>(self);
  boost::shared_ptr<PSEvt::Event> evt = py_this->m_obj.next();
  if (evt) {
    Py_RETURN_NONE;
  } else {
    // stop iteration
    PyErr_SetNone( PyExc_StopIteration );
    return 0;
  }
}

}
