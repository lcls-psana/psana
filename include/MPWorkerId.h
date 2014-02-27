#ifndef PSANA_MPWORKERID_H
#define PSANA_MPWORKERID_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class MPWorkerId.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <sys/types.h>

//----------------------
// Base Class Headers --
//----------------------

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

/// @addtogroup psana

/**
 *  @ingroup psana
 *
 *  @brief Structure which describes worker process (from the master point of view).
 *
 *  @note This software was developed for the LCLS project.  If you use all or 
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @version $Id$
 *
 *  @author Andy Salnikov
 */

class MPWorkerId  {
public:

  // constructors
  MPWorkerId () {}
  MPWorkerId (int workerId, pid_t pid, int fdDataPipe)
    : m_workerId(workerId), m_pid(pid), m_fdDataPipe(fdDataPipe) {}

  /// Return worker identifier, small non-negative number
  int workerId() const { return m_workerId; }

  /// Return worker PID
  pid_t pid() const { return m_pid; }

  /// Return descriptor for master-side (write-only) end of data pipe.
  int fdDataPipe() const { return m_fdDataPipe; }

protected:

private:

  int m_workerId;   ///< worker identifier, small non-negative number
  pid_t m_pid;      ///< worker PID
  int m_fdDataPipe; ///< descriptor for master-side (write-only) end of data pipe
  
};

} // namespace psana

#endif // PSANA_MPWORKERID_H
