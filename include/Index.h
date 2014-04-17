#ifndef PSANA_INDEX_H
#define PSANA_INDEX_H

#include <boost/cstdint.hpp>
#include <vector>

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
  Index() {}
  virtual ~Index() {}
  virtual int   jump(uint64_t time)               = 0;
  virtual void  setrun(int run)                   = 0;
  virtual const std::vector<uint64_t>& runtimes() = 0;
  virtual const std::vector<unsigned>& runs()     = 0;
};

}
#endif // PSANA_INDEX_H
