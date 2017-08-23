#ifndef PSANA_RANDOMACCESS_H
#define PSANA_RANDOMACCESS_H

#include <boost/cstdint.hpp>
#include <vector>
#include <string>

namespace psana {

/// @addtogroup psana

/**
 *  @ingroup psana
 *
 *  @brief Interface to allow XTC file random access.
 *
 *  @version $Id: RandomAccess.h 7696 2017-08-18 00:40:59Z eslaught@SLAC.STANFORD.EDU $
 *
 *  @author Elliott Slaughter
 */

class RandomAccess {
public:
  RandomAccess() {}
  virtual ~RandomAccess() {}
  virtual int      jump(const std::vector<std::string>& filenames, const std::vector<int64_t> &offsets, const std::string &lastBeginCalibCycleDgram, uintptr_t runtime, uintptr_t ctx)  = 0;
  virtual void     setrun(int run)                                    = 0;
};

}
#endif // PSANA_RANDOMACCESS_H
