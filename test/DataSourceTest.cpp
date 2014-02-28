//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Test suite case for the DataSourceTest.
//
//------------------------------------------------------------------------

//---------------
// C++ Headers --
//---------------
#include <boost/make_shared.hpp>
#include <algorithm>
#include <iterator>
#include <iostream>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "psana/DataSource.h"
#include "psana/InputModule.h"
#include "PSEnv/Env.h"

using namespace psana ;

#define BOOST_TEST_MODULE DataSourceTest
#include <boost/test/included/unit_test.hpp>

/**
 * Simple test suite for module DataSourceTest.
 * See http://www.boost.org/doc/libs/1_36_0/libs/test/doc/html/index.html
 */

namespace {

// Implementation of the InputModulle which generates predefined sequences of events
class TestInputModule: public InputModule {
public:
  
  TestInputModule(const InputModule::Status states[], int nstates) : InputModule("TestInputModule")
  {
    std::copy(states, states+nstates, std::back_inserter(m_states));
  }
  
  virtual void beginJob(Event& evt, Env& env) {}

  virtual Status event(Event& evt, Env& env) {
    InputModule::Status state = InputModule::Stop;
    if (not m_states.empty()) {
      state = m_states.front();
      m_states.pop_front();
    }
    return state;
  }
  
  virtual void endJob(Event& evt, Env& env) {}

private:
  
  std::deque<psana::InputModule::Status> m_states;  
};

struct Fixture {
  
  Fixture(const InputModule::Status states[], int nstates) 
  {
    boost::shared_ptr<AliasMap> amap = boost::make_shared<AliasMap>();
    boost::shared_ptr<PSEnv::IExpNameProvider> expNameProvider;
    boost::shared_ptr<PSEnv::Env> env = boost::make_shared<PSEnv::Env>("", expNameProvider, "", amap, 0);
    boost::shared_ptr<InputModule> input = boost::make_shared<TestInputModule>(states, nstates);
    const std::vector<boost::shared_ptr<Module> > modules;
    dataSrc = boost::make_shared<DataSource>(input, modules, env);
  }
  
  boost::shared_ptr<DataSource> dataSrc;
};


}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_1 )
{
  InputModule::Status states[] = {
  };

  Fixture f(states, sizeof states/sizeof states[0]);
  
  BOOST_CHECK(not f.dataSrc->empty());
  
  DataSource src;
  BOOST_CHECK(src.empty());
}
// ==============================================================

BOOST_AUTO_TEST_CASE( test_events_1 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  EventIter iter = f.dataSrc->events();
  boost::shared_ptr<PSEvt::Event> evt;
  
  evt = iter.next();
  BOOST_CHECK(evt);
  evt = iter.next();
  BOOST_CHECK(evt);
  evt = iter.next();
  BOOST_CHECK(not evt);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_events_2 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  EventIter iter = f.dataSrc->events();
  boost::shared_ptr<PSEvt::Event> evt;
  
  evt = iter.next();
  BOOST_CHECK(evt);
  evt = iter.next();
  BOOST_CHECK(evt);
  evt = iter.next();
  BOOST_CHECK(evt);
  evt = iter.next();
  BOOST_CHECK(evt);
  evt = iter.next();
  BOOST_CHECK(not evt);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_events_3 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  EventIter iter = f.dataSrc->events();
  boost::shared_ptr<PSEvt::Event> evt;
  
  evt = iter.next();
  BOOST_CHECK(evt);
  evt = iter.next();
  BOOST_CHECK(evt);
  evt = iter.next();
  BOOST_CHECK(evt);
  evt = iter.next();
  BOOST_CHECK(evt);
  evt = iter.next();
  BOOST_CHECK(not evt);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_steps_1 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  StepIter iter = f.dataSrc->steps();
  Step step;
  BOOST_CHECK(not step);
  
  step = iter.next();
  BOOST_CHECK(step);
  step = iter.next();
  BOOST_CHECK(not step);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_steps_2 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  StepIter iter = f.dataSrc->steps();
  Step step;
  BOOST_CHECK(not step);
  
  step = iter.next();
  BOOST_CHECK(step);
  step = iter.next();
  BOOST_CHECK(step);
  step = iter.next();
  BOOST_CHECK(not step);
}
// ==============================================================

BOOST_AUTO_TEST_CASE( test_steps_3 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  StepIter iter = f.dataSrc->steps();
  Step step;
  BOOST_CHECK(not step);
  
  step = iter.next();
  BOOST_CHECK(step);
  step = iter.next();
  BOOST_CHECK(step);
  step = iter.next();
  BOOST_CHECK(not step);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_runs_1 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  RunIter iter = f.dataSrc->runs();
  Run run;
  BOOST_CHECK(not run);
  
  run = iter.next();
  BOOST_CHECK(run);
  run = iter.next();
  BOOST_CHECK(not run);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_runs_2 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  RunIter iter = f.dataSrc->runs();
  Run run;
  BOOST_CHECK(not run);
  
  run = iter.next();
  BOOST_CHECK(run);
  run = iter.next();
  BOOST_CHECK(run);
  run = iter.next();
  BOOST_CHECK(not run);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_steps_nest_1 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  StepIter sit = f.dataSrc->steps();
  Step step;
  boost::shared_ptr<PSEvt::Event> evt;  
  BOOST_CHECK(not step);
  
  step = sit.next();
  BOOST_CHECK(step);

  EventIter eit = step.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
    
  step = sit.next();
  BOOST_CHECK(not step);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_steps_nest_2 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  StepIter sit = f.dataSrc->steps();
  Step step;
  boost::shared_ptr<PSEvt::Event> evt;  
  
  step = sit.next();
  BOOST_CHECK(step);

  EventIter eit = step.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
    
  step = sit.next();
  BOOST_CHECK(step);

  eit = step.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
    
  step = sit.next();
  BOOST_CHECK(not step);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_runs_nest_1 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  RunIter rit = f.dataSrc->runs();
  Run run;
  boost::shared_ptr<PSEvt::Event> evt;  
  
  run = rit.next();
  BOOST_CHECK(run);

  EventIter eit = run.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
    
  run = rit.next();
  BOOST_CHECK(not run);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_runs_nest_2 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  RunIter rit = f.dataSrc->runs();
  Run run;
  boost::shared_ptr<PSEvt::Event> evt;  
  
  run = rit.next();
  BOOST_CHECK(run);

  EventIter eit = run.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
    
  run = rit.next();
  BOOST_CHECK(run);

  eit = run.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
    
  run = rit.next();
  BOOST_CHECK(not run);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_runs_nest_3 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  RunIter rit = f.dataSrc->runs();
  Run run;
  Step step;
  boost::shared_ptr<PSEvt::Event> evt;  
  
  run = rit.next();
  BOOST_CHECK(run);

  StepIter sit = run.steps();

  step = sit.next();
  BOOST_CHECK(step);

  EventIter eit = step.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
  
  step = sit.next();
  BOOST_CHECK(not step);

  run = rit.next();
  BOOST_CHECK(not run);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_runs_nest_4 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  RunIter rit = f.dataSrc->runs();
  Run run;
  Step step;
  boost::shared_ptr<PSEvt::Event> evt;  
  
  run = rit.next();
  BOOST_CHECK(run);

  StepIter sit = run.steps();
  step = sit.next();
  BOOST_CHECK(step);

  EventIter eit = step.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
  
  step = sit.next();
  BOOST_CHECK(step);

  eit = step.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
  
  step = sit.next();
  BOOST_CHECK(not step);

  run = rit.next();
  BOOST_CHECK(not run);
}

// ==============================================================

BOOST_AUTO_TEST_CASE( test_runs_nest_5 )
{
  InputModule::Status states[] = {
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
      InputModule::BeginRun,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::BeginCalibCycle,
      InputModule::DoEvent,
      InputModule::DoEvent,
      InputModule::EndCalibCycle,
      InputModule::EndRun,
  };

  Fixture f(states, sizeof states/sizeof states[0]);

  RunIter rit = f.dataSrc->runs();
  Run run;
  Step step;
  boost::shared_ptr<PSEvt::Event> evt;  
  
  run = rit.next();
  BOOST_CHECK(run);

  StepIter sit = run.steps();
  step = sit.next();
  BOOST_CHECK(step);

  EventIter eit = step.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
  
  step = sit.next();
  BOOST_CHECK(step);

  eit = step.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
  
  step = sit.next();
  BOOST_CHECK(not step);

  run = rit.next();
  BOOST_CHECK(run);

  sit = run.steps();
  step = sit.next();
  BOOST_CHECK(step);

  eit = step.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
  
  step = sit.next();
  BOOST_CHECK(step);

  eit = step.events();
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(evt);
  evt = eit.next();
  BOOST_CHECK(not evt);
  
  step = sit.next();
  BOOST_CHECK(not step);

  run = rit.next();
  BOOST_CHECK(not run);
}

// ==============================================================

