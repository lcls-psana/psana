#ifndef PSANA_GENERICLOADER_H
#define PSANA_GENERICLOADER_H

#include <string>
#include <boost/shared_ptr.hpp>
#include "psana/Module.h"

namespace psana {

class GenericLoader  {
public:
  boost::shared_ptr<Module> loadModule(const std::string& name) const;
};
} // namespace psana

#endif // PSANA_GENERICLOADER_H
