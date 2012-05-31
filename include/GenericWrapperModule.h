#ifndef PSANA_GENERICWRAPPERMODULE_H
#define PSANA_GENERICWRAPPERMODULE_H

#include "psana/Module.h"
#include "GenericWrapper/GenericWrapper.h"

namespace psana {

class GenericWrapperModule : public Module {
public:

  GenericWrapperModule(GenericWrapper* wrapper) : Module("") { m_wrapper = wrapper; }
  ~GenericWrapperModule() {}
  void beginJob(Event& evt, Env& env) { return m_wrapper->beginJob(evt, env); }
  void beginRun(Event& evt, Env& env) { return m_wrapper->beginRun(evt, env); }
  void beginCalibCycle(Event& evt, Env& env) { return m_wrapper->beginCalibCycle(evt, env); }
  void event(Event& evt, Env& env) { return m_wrapper->event(evt, env); }
  void endCalibCycle(Event& evt, Env& env) { return m_wrapper->endCalibCycle(evt, env); }
  void endRun(Event& evt, Env& env) { return m_wrapper->endRun(evt, env); }
  void endJob(Event& evt, Env& env) { return m_wrapper->endJob(evt, env); }
private:
  GenericWrapper* m_wrapper;
};
} // namespace psana

#endif // PSANA_GENERICWRAPPERMODULE_H
