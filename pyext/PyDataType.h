#ifndef PSANA_PYDATATYPE_H
#define PSANA_PYDATATYPE_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class PyDataType.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <new>
#include <string>
#include <iostream>
#include <sstream>

//----------------------
// Base Class Headers --
//----------------------
#include "python/Python.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------

//    ---------------------
//    -- Class Interface --
//    ---------------------

namespace psana {
namespace pyext {

/**
 *  This software was developed for the LUSI project.  If you use all or
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @see AdditionalClass
 *
 *  @version $Id$
 *
 *  @author Andrei Salnikov
 */

template <typename ConcreteType, typename CppType>
struct PyDataType : PyObject {

  /// Returns the Python type
  static PyTypeObject* typeObject();

  /// Builds Python object from corresponding C++ type.
  static ConcreteType* PyObject_FromCpp( const CppType& obj );

  // Returns reference to embedded object
  static CppType& cppObject(PyObject* self) {
    PyDataType* py_this = (PyDataType*) self;
    return py_this->m_obj;
  }

  // returns true if object is an instance of this type or subtype
  static bool Object_TypeCheck( PyObject* obj ) {
    PyTypeObject* type = typeObject();
    return PyObject_TypeCheck( obj, type );
  }

  // Dump object info to a stream
  void print(std::ostream& out) const {
    out << "<" << ob_type->tp_name << "(@" << this << ")>";
  }

  // --------------------------------------------------

  CppType m_obj;

protected:

  /// Initialize Python type and register it in a module
  static void initType( const char* name, PyObject* module );

  // standard Python deallocation function
  static void PyDataType_dealloc( PyObject* self );

  // repr() function
  static PyObject* repr( PyObject *self )  {
    std::ostringstream str;
    static_cast<ConcreteType*>(self)->print(str);
    return PyString_FromString( str.str().c_str() );
  }
};

/// stream insertion operator
template <typename ConcreteType, typename CppType>
std::ostream&
operator<<(std::ostream& out, const PyDataType<ConcreteType, CppType>& data) {
  static_cast<const ConcreteType&>(data).print(out);
  return out;
}

/// Returns the Python type object
template <typename ConcreteType, typename CppType>
PyTypeObject*
PyDataType<ConcreteType, CppType>::typeObject()
{
  static PyTypeObject type = {
    PyObject_HEAD_INIT(0)
    0,                       /*ob_size*/
    0,                       /*tp_name*/
    sizeof(ConcreteType),    /*tp_basicsize*/
    0,                       /*tp_itemsize*/
    /* methods */
    PyDataType_dealloc,      /*tp_dealloc*/
    0,                       /*tp_print*/
    0,                       /*tp_getattr*/
    0,                       /*tp_setattr*/
    0,                       /*tp_compare*/
    repr,                    /*tp_repr*/
    0,                       /*tp_as_number*/
    0,                       /*tp_as_sequence*/
    0,                       /*tp_as_mapping*/
    0,                       /*tp_hash*/
    0,                       /*tp_call*/
    repr,                    /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro*/
    0,                       /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,      /*tp_flags*/
    0,                       /*tp_doc*/
    0,                       /*tp_traverse*/
    0,                       /*tp_clear*/
    0,                       /*tp_richcompare*/
    0,                       /*tp_weaklistoffset*/
    0,                       /*tp_iter*/
    0,                       /*tp_iternext*/
    0,                       /*tp_methods*/
    0,                       /*tp_members*/
    0,                       /*tp_getset*/
    0,                       /*tp_base*/
    0,                       /*tp_dict*/
    0,                       /*tp_descr_get*/
    0,                       /*tp_descr_set*/
    0,                       /*tp_dictoffset*/
    0,                       /*tp_init*/
    PyType_GenericAlloc,     /*tp_alloc*/
    0,                       /*tp_new*/
    _PyObject_Del,           /*tp_free*/
    0,                       /*tp_is_gc*/
    0,                       /*tp_bases*/
    0,                       /*tp_mro*/
    0,                       /*tp_cache*/
    0,                       /*tp_subclasses*/
    0,                       /*tp_weaklist*/
  };

  return &type;
}

template <typename ConcreteType, typename CppType>
void
PyDataType<ConcreteType, CppType>::PyDataType_dealloc( PyObject* self )
{
  PyDataType* py_this = (PyDataType*) self;

  // call destructor for embedded object
  py_this->m_obj.~CppType();

  // deallocate ourself
  self->ob_type->tp_free(self);
}

/// Builds Python object from corresponding C++ type.
template <typename ConcreteType, typename CppType>
ConcreteType*
PyDataType<ConcreteType, CppType>::PyObject_FromCpp( const CppType& obj )
{
  ConcreteType* ob = PyObject_New(ConcreteType, typeObject());
  if ( not ob ) {
    PyErr_SetString( PyExc_RuntimeError, "Failed to create PyDataType object." );
    return 0;
  }

  // copy-construct the object
  new(&ob->m_obj) CppType(obj);

  return ob;
}

/// Initialize Python type and register it in a module
template <typename ConcreteType, typename CppType>
void
PyDataType<ConcreteType, CppType>::initType(const char* name, PyObject* module)
{
  static std::string typeName;

  // perfix type name with module name
  const char* modname = PyModule_GetName(module);
  if ( modname ) {
    typeName = modname;
    typeName += '.';
  }
  typeName += name;

  // set the name
  PyTypeObject* type = typeObject();
  type->tp_name = (char*)typeName.c_str();

  // initialize type
  if ( PyType_Ready( type ) < 0 ) return;

  // register it in a module
  PyDict_SetItemString( PyModule_GetDict(module), (char*)name, (PyObject*) type );
}

} // namespace pyext
} // namespace psana

#endif // PSANA_PYDATATYPE_H
