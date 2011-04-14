//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class psana...
//
// Author List:
//      Andrei Salnikov
//
//------------------------------------------------------------------------

//-----------------
// C/C++ Headers --
//-----------------
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include "boost/filesystem.hpp"

//----------------------
// Base Class Headers --
//----------------------
#include "AppUtils/AppBase.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "AppUtils/AppCmdArgList.h"
#include "AppUtils/AppCmdOpt.h"
#include "AppUtils/AppCmdOptList.h"
#include "ConfigSvc/ConfigSvc.h"
#include "ConfigSvc/ConfigSvcImplFile.h"
#include "MsgLogger/MsgFormatter.h"
#include "MsgLogger/MsgLogger.h"
#include "psana/DynLoader.h"
#include "PSEnv/Env.h"
#include "PSEvt/Event.h"
#include "PSEvt/ProxyDict.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace psana {

//
//  Application class
//
class psanaapp : public AppUtils::AppBase {
public:

  // Constructor
  explicit psanaapp ( const std::string& appName ) ;

  // destructor
  ~psanaapp () ;

protected :

  /**
   *  Method called before runApp, can be overriden in subclasses.
   *  Usually if you override it, call base class method too.
   */
  virtual int preRunApp () ;

  /**
   *  Main method which runs the whole application
   */
  virtual int runApp () ;

private:

  // more command line options and arguments
  AppUtils::AppCmdOpt<std::string> m_configOpt ;
  AppUtils::AppCmdOpt<std::string> m_jobNameOpt ;
  AppUtils::AppCmdOptList<std::string>  m_modulesOpt;
  AppUtils::AppCmdArgList<std::string>  m_files;


};

//----------------
// Constructors --
//----------------
psanaapp::psanaapp ( const std::string& appName )
  : AppUtils::AppBase( appName )
  , m_configOpt( 'c', "config", "path", "configuration file, def: psana.cfg", "" )
  , m_jobNameOpt( 'j', "job-name", "string", "job name, def: from input files", "" )
  , m_modulesOpt( 'm', "module", "name", "module name, more than one possible" )
  , m_files( "data-file",   "file name(s) with input data", std::list<std::string>() )
{
  addOption( m_configOpt ) ;
  addOption( m_jobNameOpt ) ;
  addOption( m_modulesOpt ) ;
  addArgument( m_files ) ;
}

//--------------
// Destructor --
//--------------
psanaapp::~psanaapp ()
{
}

/**
 *  Method called before runApp, can be overriden in subclasses.
 *  Usually if you override it, call base class method too.
 */
int 
psanaapp::preRunApp () 
{
  AppBase::preRunApp();

  // use different formatting for messages
  const char* fmt = "[%(level):%(logger)] %(message)" ;
  const char* errfmt = "[%(level):%(time):%(file):%(line)] %(message)" ;
  const char* trcfmt = "[%(level):%(time):%(logger)] %(message)" ;
  const char* dbgfmt = errfmt ;
  MsgLogger::MsgFormatter::addGlobalFormat ( fmt ) ;
  MsgLogger::MsgFormatter::addGlobalFormat ( MsgLogger::MsgLogLevel::debug, dbgfmt ) ;
  MsgLogger::MsgFormatter::addGlobalFormat ( MsgLogger::MsgLogLevel::trace, trcfmt ) ;
  MsgLogger::MsgFormatter::addGlobalFormat ( MsgLogger::MsgLogLevel::warning, errfmt ) ;
  MsgLogger::MsgFormatter::addGlobalFormat ( MsgLogger::MsgLogLevel::error, errfmt ) ;
  MsgLogger::MsgFormatter::addGlobalFormat ( MsgLogger::MsgLogLevel::fatal, errfmt ) ;
  
  return 0;
}

/**
 *  Main method which runs the whole application
 */
int
psanaapp::runApp ()
{
  // if neither -m nor -c specified then try to read psana.cfg
  std::string cfgFile = m_configOpt.value();
  if (cfgFile.empty() and m_modulesOpt.value().empty()) {
    cfgFile = "psana.cfg";
  } 
  
  // start with reading configuration file
  std::auto_ptr<ConfigSvc::ConfigSvcImplI> cfgImpl;
  if (cfgFile.empty()) {
    cfgImpl.reset( new ConfigSvc::ConfigSvcImplFile() );
  } else {
    cfgImpl.reset( new ConfigSvc::ConfigSvcImplFile(cfgFile) );
  }
  
  // initialize config service
  ConfigSvc::ConfigSvc::init(cfgImpl);
  ConfigSvc::ConfigSvc cfgsvc;

  // command-line -m options override config file values
  if (not m_modulesOpt.value().empty()) {
    std::string modlist;
    const std::list<std::string>& modules = m_modulesOpt.value();
    for(std::list<std::string>::const_iterator it = modules.begin(); it != modules.end(); ++ it) {
      if (not modlist.empty()) modlist += ' ';
      modlist += *it;
    }
    MsgLogRoot(trace, "set module list to '" << modlist << "'");
    cfgsvc.put("psana", "modules", modlist);
  } 

  // get list of modules to load
  std::list<std::string> moduleNames = cfgsvc.getList("psana", "modules");
  
  // max number of events
  unsigned maxEvents = cfgsvc.get("psana", "events", 0xFFFFFFFFU);
  
  DynLoader loader;

  // Load input module, fixed name for now
  const std::string& iname = "PSXtcInput.XtcInputModule";
  psana::InputModule* input = loader.loadInputModule(iname);
  MsgLogRoot(trace, "Loaded input module " << iname);

  // pass file names to the configuration so that input module can find them
  typedef AppUtils::AppCmdArgList<std::string>::const_iterator FileIter;
  std::string flist;
  for (FileIter it = m_files.begin(); it != m_files.end(); ++it ) {
    if (not flist.empty()) flist += " ";
    flist += *it;
  }
  cfgsvc.put(iname, "files", flist);
  
  // instantiate all user modules
  std::vector<Module*> modules;
  for ( std::list<std::string>::const_iterator it = moduleNames.begin(); it != moduleNames.end() ; ++ it ) {
    Module* m = loader.loadModule(*it);
    modules.push_back(m);
    MsgLogRoot(trace, "Loaded module " << m->name());
  }
  
  // get/build job name
  std::string jobName = m_jobNameOpt.value();
  if (jobName.empty() and not m_files.empty()) {
    boost::filesystem::path path = *m_files.begin();
    jobName = path.stem();
  }
  MsgLogRoot(debug, "job name = " << jobName);
  
  // Setup environment
  PSEnv::Env env(jobName);
  
  // Start with beginJob for everyone
  input->beginJob(env);
  for (std::vector<Module*>::iterator it = modules.begin() ; it != modules.end() ; ++it) {
    (*it)->beginJob(env);
  }
  
  // event loop
  bool stop = false ;
  while ( maxEvents > 0 and not stop) {
    
    // Create event object
    boost::shared_ptr<PSEvt::ProxyDict> dict(new PSEvt::ProxyDict);
    Event evt(dict);
    
    // run input module to populate event
    InputModule::Status istat = input->event(evt, env);
    MsgLogRoot(debug, "input.event() returned " << istat)
    
    if (istat == InputModule::Skip) continue;
    if (istat == InputModule::Stop) break;
    if (istat == InputModule::Abort) {
      MsgLogRoot(info, "Input module requested abort");
      return 1;
    }
    
    for (std::vector<Module*>::iterator it = modules.begin() ; it != modules.end() ; ++it) {
      Module& mod = *(*it);
      
      // clear module status
      mod.reset();
      
      // dispatch event to particular method based on vent type
      if (istat == InputModule::DoEvent) {
        mod.event(evt, env);
      } else if (istat == InputModule::BeginRun) {
        mod.beginRun(env);
      } else if (istat == InputModule::BeginCalibCycle) {
        mod.beginCalibCycle(env);
      } else if (istat == InputModule::EndCalibCycle) {
        mod.endCalibCycle(env);
      } else if (istat == InputModule::EndRun) {
        mod.endRun(env);
      }
      
      // check what module wants to tell us
      switch (mod.status()) {
      case Module::Skip:
        break;
      case Module::Stop:
        MsgLogRoot(info, "module " << mod.name() << " requested stop");
        stop = true;
        break;
      case Module::Abort:
        MsgLogRoot(info, "module " << mod.name() << " requested abort");
        return 1;
        break;
      case Module::OK:
        break;
      }
      
    }
    
    --maxEvents;
  }
  
  
  // End with endJob for everyone, note that the order is the same
  input->endJob(env);
  for (std::vector<Module*>::iterator it = modules.begin() ; it != modules.end() ; ++it) {
    (*it)->endJob(env);
  }

  // cleanup
  delete input;
  for (std::vector<Module*>::iterator it = modules.begin() ; it != modules.end() ; ++it) {
    delete *it;
  }
  
  // return 0 on success, other values for error (like main())
  return 0 ;
}

} // namespace psana


// this defines main()
APPUTILS_MAIN(psana::psanaapp)
