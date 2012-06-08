//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class EventLoop...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/EventLoop.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <iostream>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "psana/Exceptions.h"
#include "psana/InputModule.h"
#include "PSEvt/ProxyDict.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace {

  const char* logger = "EventLoop";

}


//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace psana {

//----------------
// Constructors --
//----------------
EventLoop::EventLoop (const boost::shared_ptr<InputModule>& inputModule,
    const std::vector<boost::shared_ptr<Module> >& modules,
    const boost::shared_ptr<PSEnv::Env>& env)
  : m_inputModule(inputModule)
  , m_modules(modules)
  , m_env(env)
  , m_finished(false)
  , m_state(StateNone)
  , m_values()
{
  m_newStateMethods[StateNone] = 0;
  m_newStateMethods[StateConfigured] = &Module::beginJob;
  m_newStateMethods[StateRunning] = &Module::beginRun;
  m_newStateMethods[StateScanning] = &Module::beginCalibCycle;

  m_newStateEventType[StateNone] = None;
  m_newStateEventType[StateConfigured] = None;
  m_newStateEventType[StateRunning] = BeginRun;
  m_newStateEventType[StateScanning] = BeginCalibCycle;

  m_closeStateMethods[StateNone] = 0;
  m_closeStateMethods[StateConfigured] = &Module::endJob;
  m_closeStateMethods[StateRunning] = &Module::endRun;
  m_closeStateMethods[StateScanning] = &Module::endCalibCycle;

  m_closeStateEventType[StateNone] = None;
  m_closeStateEventType[StateConfigured] = None;
  m_closeStateEventType[StateRunning] = EndRun;
  m_closeStateEventType[StateScanning] = EndCalibCycle;
}

//--------------
// Destructor --
//--------------
EventLoop::~EventLoop ()
{
  // close all transitions
  if (m_state != StateNone) {
    EventPtr evt = boost::make_shared<PSEvt::Event>(boost::make_shared<PSEvt::ProxyDict>());
    m_inputModule->endJob(*evt, *m_env);
    unwind(StateNone, evt, true);
  }
}

/**
 *  Method that runs one iteration and returns event type,
 *  and event object.
 */
EventLoop::value_type
EventLoop::next()
{
  value_type result(None, boost::shared_ptr<PSEvt::Event>());
  if (m_finished) return result;

  if (m_state == StateNone) {
    // run beginJob for all modules
    EventPtr evt = boost::make_shared<PSEvt::Event>(boost::make_shared<PSEvt::ProxyDict>());
    m_inputModule->beginJob(*evt, *m_env);
    Module::Status stat = newState(StateConfigured, evt);
    if (stat != Module::OK) {
      // if anything fails in beginJob don't try to continue
      return result;
    }
  }

  // if we don't have any saved transitions on stack then get next
  // transition from input module, decide what to do with it
  while (m_values.empty()) {

    // Create event object
    EventPtr evt = boost::make_shared<PSEvt::Event>(boost::make_shared<PSEvt::ProxyDict>());

    // run input module to populate event
    InputModule::Status istat = m_inputModule->event(*evt, *m_env);
    MsgLog(logger, debug, "input.event() returned " << istat);

    // check input status
    if (istat == InputModule::Skip) continue;
    if (istat == InputModule::Stop) break;
    if (istat == InputModule::Abort) {
      MsgLog(logger, info, "Input module requested abort");
      throw ExceptionAbort(ERR_LOC, "Input module requested abort");
    }

    // dispatch event to particular method based on event type
    if (istat == InputModule::DoEvent) {

      Module::Status stat = callModuleMethod(&Module::event, *evt, *m_env, false);
      if (stat == Module::Abort) throw ExceptionAbort(ERR_LOC, "User module requested abort");
      if (stat == Module::Stop) break;
      m_values.push(value_type(Event, evt));

    } else {

      State unwindTo = StateNone;
      State newState = StateNone;
      if (istat == InputModule::BeginRun) {
        unwindTo = StateConfigured;
        newState = StateRunning;
      } else if (istat == InputModule::BeginCalibCycle) {
        unwindTo = StateRunning;
        newState = StateScanning;
      } else if (istat == InputModule::EndCalibCycle) {
        unwindTo = StateRunning;
      } else if (istat == InputModule::EndRun) {
        unwindTo = StateConfigured;
      }

      Module::Status stat = unwind(unwindTo, evt);
      if (stat == Module::Abort) throw ExceptionAbort(ERR_LOC, "User module requested abort");
      if (stat == Module::Stop) break;
      if (newState != StateNone) {
        stat = this->newState(newState, evt);
        if (stat == Module::Abort) throw ExceptionAbort(ERR_LOC, "User module requested abort");
        if (stat == Module::Stop) break;
      }

    }
  }

  if (m_values.empty()) {
    // means we reached the end, time to call endJob
    EventPtr evt = boost::make_shared<PSEvt::Event>(boost::make_shared<PSEvt::ProxyDict>());
    m_inputModule->endJob(*evt, *m_env);
    unwind(StateNone, evt, true);
    m_finished = true;
  }

  // return first transition in the queue
  if (not m_values.empty()) {
    result = m_values.front();
    m_values.pop();
  }

  return result;
}

Module::Status
EventLoop::newState(State state, const EventPtr& evt)
{
  MsgLog(logger, trace, "newState " << state);

  // make sure that previous state is also in the stack
  if (int(m_state) < int(state-1)) {
    // use different event instance for it
    EventPtr evt = boost::make_shared<PSEvt::Event>(boost::make_shared<PSEvt::ProxyDict>());
    Module::Status stat = newState(State(state-1), evt);
    // someone is trying to stop or abort
    if (stat != Module::OK) return stat;
  }

  // save the state
  m_state = state;

  // call user modules methods
  Module::Status stat = callModuleMethod(m_newStateMethods[state], *evt, *m_env, true);

  // store result
  if (stat == Module::OK and m_newStateEventType[state] != None) {
    m_values.push(value_type(m_newStateEventType[state], evt));
  }

  return stat;
}


Module::Status
EventLoop::closeState(const EventPtr& evt)
{
  MsgLog(logger, trace, "closeState " << m_state);

  Module::Status stat = callModuleMethod(m_closeStateMethods[m_state], *evt, *m_env, true);

  // store result
  if (stat == Module::OK and m_closeStateEventType[m_state] != None) {
    m_values.push(value_type(m_closeStateEventType[m_state], evt));
  }

  // go back to previous state
  m_state = State(m_state-1);

  return stat;
}


Module::Status
EventLoop::unwind(State newState, const EventPtr& evt, bool ignoreStatus)
{
  while (m_state > newState) {
    Module::Status stat = closeState(evt);
    if (not ignoreStatus and stat != Module::OK) return stat;
  }
  return Module::OK;
}

//
// Call given method for all defined modules, ignoreSkip should be set
// to false for event() method, true for everything else
//
Module::Status
EventLoop::callModuleMethod(ModuleMethod method, PSEvt::Event& evt, PSEnv::Env& env, bool ignoreSkip)
{
  Module::Status stat = Module::OK;

  if (ignoreSkip) {

    // call all modules, do not skip any one of them

    for (std::vector<boost::shared_ptr<Module> >::const_iterator it = m_modules.begin() ; it != m_modules.end() ; ++it) {
      boost::shared_ptr<Module> mod = *it;

      // clear module status
      mod->reset();

      // call the method
      ((*mod).*method)(evt, env);

      // check what module wants to tell us
      if (mod->status() == Module::Skip) {
        // silently ignore Skip
      } else if (mod->status() == Module::Stop) {
        // set the flag but continue
        MsgLog(logger, info, "module " << mod->name() << " requested stop");
        stat = Module::Stop;
      } else if (mod->status() == Module::Abort) {
        // abort immediately
        MsgLog(logger, info, "module " << mod->name() << " requested abort");
        stat = Module::Abort;
        break;
      }
    }

  } else {

    // call all modules, respect Skip flag

    for (std::vector<boost::shared_ptr<Module> >::const_iterator it = m_modules.begin() ; it != m_modules.end() ; ++it) {
      boost::shared_ptr<Module> mod = *it;

      // clear module status
      mod->reset();

      // call the method, skip regular modules if skip status is set, but
      // still call special modules which are interested in all events
      if (stat == Module::OK or mod->observeAllEvents()) {
        ((*mod).*method)(evt, env);
      }

      // check what module wants to tell us
      if (mod->status() == Module::Skip) {

        // Set the skip flag but continue as there may be modules interested in every event
        MsgLog(logger, trace, "module " << mod->name() << " requested skip");
        if (stat == Module::OK) stat = Module::Skip;

        // add special flag to event
        if (not evt.exists<int>("__psana_skip_event__")) {
          evt.put(boost::make_shared<int>(1), "__psana_skip_event__");
        }

      } else if (mod->status() == Module::Stop) {
        // stop right here
        MsgLog(logger, info, "module " << mod->name() << " requested stop");
        stat = Module::Stop;
        break;
      } else if (mod->status() == Module::Abort) {
        // abort immediately
        MsgLog(logger, info, "module " << mod->name() << " requested abort");
        stat = Module::Abort;
        break;
      }
    }

  }

  return stat;
}

} // namespace psana
