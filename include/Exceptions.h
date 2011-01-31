#ifndef PSANA_EXCEPTIONS_H
#define PSANA_EXCEPTIONS_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class Exceptions.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------

//----------------------
// Base Class Headers --
//----------------------
#include "ErrSvc/Issue.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------

//------------------------------------
// Collaborating Class Declarations --
//------------------------------------

//		---------------------
// 		-- Class Interface --
//		---------------------

namespace psana {

/**
 *  Exception classes for psana package.
 *
 *  This software was developed for the LCLS project.  If you use all or 
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @see AdditionalClass
 *
 *  @version $Id$
 *
 *  @author Andrei Salnikov
 */

class Exception : public ErrSvc::Issue {
public:

  /// Constructor takes the reason for an exception
  Exception ( const ErrSvc::Context& ctx, const std::string& what ) ;

};

// exception thrown when module name is incorrect
class ExceptionModuleName : public Exception {
public:

  /// Constructor takes the name of the module
  ExceptionModuleName ( const ErrSvc::Context& ctx, const std::string& module ) ;

};

class ExceptionErrno : public Exception {
public:

  /// Constructor takes the reason for an exception
  ExceptionErrno ( const ErrSvc::Context& ctx, const std::string& what ) ;

};

class ExceptionDlerror : public Exception {
public:

  /// Constructor takes the reason for an exception
  ExceptionDlerror ( const ErrSvc::Context& ctx, const std::string& what ) ;

};

} // namespace psana

#endif // PSANA_EXCEPTIONS_H
