#ifndef PSANA_EVENTTIME_H
#define PSANA_EVENTTIME_H

#include <boost/cstdint.hpp>

namespace psana {

/// @addtogroup psana

/**
 *  @ingroup psana
 *
 *  @brief Object used to jump to events when using Indexing.
 *
 *  @version $Id: EventTime.h 7696 2014-02-27 00:40:59Z cpo@SLAC.STANFORD.EDU $
 *
 *  @author Christopher O'Grady
 */

class EventTime {
public:
  EventTime() {}
  EventTime(uint64_t time, uint32_t fiducial) :
    _time(time),_fiducial(fiducial) {}
  uint64_t time()        const {return _time;}
  uint32_t seconds()     const {return (_time&0xffffffff00000000)>>32;}
  uint32_t nanoseconds() const {return _time&0xffffffff;}
  uint32_t fiducial()    const {return _fiducial;}
private:
  uint64_t _time;
  uint32_t _fiducial;
};

}
#endif // PSANA_EVENTTIME_H
