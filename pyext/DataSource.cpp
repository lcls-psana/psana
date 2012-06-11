//--------------------------------------------------------------------------
// File and Version Information:
//  $Id$
//
// Description:
//  Class DataSource...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "DataSource.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "EventIter.h"
#include "RunIter.h"
#include "ScanIter.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  // type-specific methods
  PyObject* DataSource_empty(PyObject* self, PyObject*);
  PyObject* DataSource_runs(PyObject* self, PyObject*);
  PyObject* DataSource_scans(PyObject* self, PyObject*);
  PyObject* DataSource_events(PyObject* self, PyObject*);
  PyObject* DataSource_nonzero(PyObject* self, PyObject*);

  PyMethodDef methods[] = {
    { "empty",    DataSource_empty,   METH_NOARGS, "self.empty() -> bool\n\nReturns true for non-empty data source" },
    { "runs",     DataSource_runs,    METH_NOARGS, "self.runs() -> iterator\n\nReturns iterator for contained runs" },
    { "scans",    DataSource_scans,   METH_NOARGS, "self.scans() -> iterator\n\nReturns iterator for contained scans" },
    { "events",   DataSource_events,  METH_NOARGS, "self.events() -> iterator\n\nReturns iterator for contained events" },
    {0, 0, 0, 0}
   };

  char typedoc[] = "";

}

//    ----------------------------------------
//    -- Public Function Member Definitions --
//    ----------------------------------------

void
psana::pyext::DataSource::initType(PyObject* module)
{
  PyTypeObject* type = BaseType::typeObject() ;
  type->tp_doc = ::typedoc;
  type->tp_methods = ::methods;

  BaseType::initType("DataSource", module);
}

namespace {

PyObject*
DataSource_empty(PyObject* self, PyObject* )
{
  psana::pyext::DataSource* py_this = static_cast<psana::pyext::DataSource*>(self);
  return PyBool_FromLong(long(py_this->m_obj.empty()));
}

PyObject*
DataSource_runs(PyObject* self, PyObject* )
{
  psana::pyext::DataSource* py_this = static_cast<psana::pyext::DataSource*>(self);
  return psana::pyext::RunIter::PyObject_FromCpp(py_this->m_obj.runs());
}

PyObject*
DataSource_scans(PyObject* self, PyObject* )
{
  psana::pyext::DataSource* py_this = static_cast<psana::pyext::DataSource*>(self);
  return psana::pyext::ScanIter::PyObject_FromCpp(py_this->m_obj.scans());
}

PyObject*
DataSource_events(PyObject* self, PyObject* )
{
  psana::pyext::DataSource* py_this = static_cast<psana::pyext::DataSource*>(self);
  return psana::pyext::EventIter::PyObject_FromCpp(py_this->m_obj.events());
}

}
