#ifndef PSANA_EXPNAMEFROMXTC_H
#define PSANA_EXPNAMEFROMXTC_H

//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class ExpNameFromXtc.
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <string>
#include <list>

//----------------------
// Base Class Headers --
//----------------------
#include "PSEnv/IExpNameProvider.h"

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
 *  @ingroup psana
 *
 *  @brief Experiment name provider which extracts experiment name from XTC
 *  file names.
 *
 *  This software was developed for the LCLS project.  If you use all or 
 *  part of it, please give an appropriate acknowledgment.
 *
 *  @version $Id$
 *
 *  @author Andy Salnikov
 */

class ExpNameFromXtc : public PSEnv::IExpNameProvider {
public:

  /// Constructor takes the list of input file names
  ExpNameFromXtc(const std::list<std::string>& files);

  // Destructor
  virtual ~ExpNameFromXtc();

  /// Returns instrument name
  virtual const std::string& instrument() const;

  /// Returns experiment name
  virtual const std::string& experiment() const;

protected:

  // one-time intilization
  void init() const;

private:
  
  const std::list<std::string> m_files;
  mutable bool m_init;
  mutable std::string m_instr;
  mutable std::string m_exp;

};

} // namespace psana

#endif // PSANA_EXPNAMEFROMXTC_H
