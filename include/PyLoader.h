#ifndef PSANA_PYLOADER_H
#define PSANA_PYLOADER_H

#include <string>
//#include <boost/shared_ptr.hpp>
//#include "psana/Module.h"
#include <GenericWrapper/GenericWrapper.h>

namespace psana {

extern GenericWrapper* X_loadWrapper(const std::string& name);

} // namespace psana

#endif // PSANA_PYLOADER_H
