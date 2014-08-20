#ifndef PSANA_INDEX_H
#define PSANA_INDEX_H

#include <boost/cstdint.hpp>
#include <vector>

#include "EventTime.h"

namespace psana {

/// @addtogroup psana

/**
 *  @ingroup psana
 *
 *  @brief Interface to allow XTC file random access.
 *
 *  @version $Id: Index.h 7696 2014-02-27 00:40:59Z cpo@SLAC.STANFORD.EDU $
 *
 *  @author Christopher O'Grady
 */

class Index {
public:
  typedef std::vector<EventTime>::const_iterator EventTimeIter;
  Index() {}
  virtual ~Index() {}
  virtual int      jump(EventTime t)                            = 0;
  virtual void     setrun(int run)                              = 0;
  virtual void     end()                                        = 0;
  virtual unsigned nsteps()                                     = 0;
  virtual void     times(EventTimeIter& begin, EventTimeIter& end) = 0;
  virtual void     times(unsigned step, EventTimeIter& begin, EventTimeIter& end) = 0;
  virtual const    std::vector<unsigned>& runs()                = 0;
};

}
#endif // PSANA_INDEX_H
