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

//----------------------
// Base Class Headers --
//----------------------
#include "AppUtils/AppBase.h"

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "AppUtils/AppCmdArgList.h"
#include "AppUtils/AppCmdOpt.h"
#include "ConfigSvc/ConfigSvc.h"
#include "ConfigSvc/ConfigSvcImplFile.h"
#include "MsgLogger/MsgLogger.h"
#include "psana/DynLoader.h"
#include "PsEnv/Env.h"
#include "PsEvt/Event.h"
#include "PsEvt/ProxyDict.h"

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
   *  Main method which runs the whole application
   */
  virtual int runApp () ;

private:

  // more command line options and arguments
  AppUtils::AppCmdOpt<std::string> m_configOpt ;
  AppUtils::AppCmdArgList<std::string>  m_files;


};

//----------------
// Constructors --
//----------------
psanaapp::psanaapp ( const std::string& appName )
  : AppUtils::AppBase( appName )
  , m_configOpt( 'c', "config", "path", "configuration file, def: psana.cfg", "psana.cfg" )
  , m_files  ( "data-file",   "file name(s) with input data", std::list<std::string>() )
{
  addOption( m_configOpt ) ;
  addArgument( m_files ) ;
}

//--------------
// Destructor --
//--------------
psanaapp::~psanaapp ()
{
}

/**
 *  Main method which runs the whole application
 */
int
psanaapp::runApp ()
{
  // start with reading configuration file
  std::auto_ptr<ConfigSvc::ConfigSvcImplI> cfgImpl ( 
      new ConfigSvc::ConfigSvcImplFile(m_configOpt.value()) );
  // initialize config service
  ConfigSvc::ConfigSvc::init(cfgImpl);
  ConfigSvc::ConfigSvc cfgsvc;
  
  // get list of modules to load
  std::list<std::string> moduleNames = cfgsvc.getList("psana", "modules");
  
  // max number of events
  unsigned maxEvents = cfgsvc.get("psana", "events", 0xFFFFFFFFU);
  
  DynLoader loader;

  // Load input module, fixed name for now
  const std::string& iname = "PsXtcInput.XtcInputModule";
  psana::InputModule* input = loader.loadInputModule(iname);
  MsgLogRoot(info, "Loaded input module " << iname);

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
    MsgLogRoot(info, "Loaded module " << m->name());
  }
  
  // Setup environment
  PsEnv::Env env;
  
  // Start with beginJob for everyone
  input->beginJob(env);
  for (std::vector<Module*>::iterator it = modules.begin() ; it != modules.end() ; ++it) {
    (*it)->beginJob(env);
  }
  
  // event loop
  bool stop = false ;
  while ( maxEvents > 0 and not stop) {
    
    // Create event object
    boost::shared_ptr<PsEvt::ProxyDict> dict(new PsEvt::ProxyDict);
    Event evt(dict);
    
    // run input module to populate event
    InputModule::Status istat = input->event(evt, env);
    MsgLogRoot(debug, "input.event() returned " << istat)
    
    for (std::vector<Module*>::iterator it = modules.begin() ; it != modules.end() ; ++it) {
      Module& mod = *(*it);
      
      // clear module status
      mod.reset();
      
      // dispatch event to particular method based on vent type
      mod.event(evt, env);
      
      // check what module wants to tell us
      switch (mod.status()) {
      case Module::Skip:
        MsgLogRoot(info, "module " << mod.name() << " requested skip");
        break;
      case Module::Stop:
        MsgLogRoot(info, "module " << mod.name() << " requested stop");
        stop = true;
        break;
      case Module::Abort:
        MsgLogRoot(info, "module " << mod.name() << " requested abort");
        ::abort();
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

  // return 0 on success, other values for error (like main())
  return 0 ;
}

} // namespace psana


// this defines main()
APPUTILS_MAIN(psana::psanaapp)
