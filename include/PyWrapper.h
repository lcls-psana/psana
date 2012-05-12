#ifndef PSANA_PYWRAPPER_H
#define PSANA_PYWRAPPER_H 1

#include <string>
#include <boost/python.hpp>
#include <boost/python/class.hpp>
#include <numpy/arrayobject.h>
#include "PSEnv/Env.h"
#include "PSEvt/Event.h"
#include "psana/Configurable.h"

namespace psana {
  class EvtGetter {
  public:
    virtual std::string getTypeName() = 0;
    virtual boost::python::api::object get(PSEvt::Event& evt, PSEvt::Source& src) = 0;
    virtual ~EvtGetter() {}
  };

  class EnvGetter {
  public:
    virtual std::string getTypeName() = 0;
    virtual boost::python::api::object get(PSEnv::Env& env, PSEvt::Source& src) = 0;
    virtual ~EnvGetter() {}
  };

  extern PyObject* ndConvert(void* data, const unsigned* shape, const unsigned ndim, char* ctype);
  extern std::map<std::string, EvtGetter*> eventGetter_map;
  extern std::map<std::string, EnvGetter*> environmentGetter_map;
}

#define ND_CONVERT(value, ctype, ndim) const ndarray<ctype, ndim>& a = value; return psana::ndConvert((void *) a.data(), a.shape(), ndim, #ctype)
#define EVT_GETTER(x) {psana::EvtGetter *g = new x ## _EvtGetter(); psana::eventGetter_map[g->getTypeName()] = g;}
#define ENV_GETTER(x) {psana::EnvGetter *g = new x ## _EnvGetter(); psana::environmentGetter_map[g->getTypeName()] = g;}

#endif // PSANA_PYWRAPPER_H
