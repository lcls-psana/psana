//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Python module _psana...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------

//-----------------
// C/C++ Headers --
//-----------------
#include "python/Python.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "DataSource.h"
#include "EventIter.h"
#include "PSAna.h"
#include "Run.h"
#include "RunIter.h"
#include "Scan.h"
#include "ScanIter.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------


//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

// Module entry point
extern "C"
PyMODINIT_FUNC init_psana()
{
  // Initialize the module
  PyObject* module = Py_InitModule3( "_psana", 0, "The Python module for psana" );
  psana::pyext::DataSource::initType( module );
  psana::pyext::EventIter::initType( module );
  psana::pyext::PSAna::initType( module );
  psana::pyext::Run::initType( module );
  psana::pyext::RunIter::initType( module );
  psana::pyext::Scan::initType( module );
  psana::pyext::ScanIter::initType( module );
}
