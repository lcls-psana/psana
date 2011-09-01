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
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

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
#include "psana/Exceptions.h"
#include "psana/ExpNameFromConfig.h"
#include "psana/ExpNameFromXtc.h"
#include "PSEvt/ProxyDict.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace fs = boost::filesystem ;

namespace {

  enum FileType { Unknown=-1, Mixed=0, XTC, HDF5 };

  // Function which tries to guess input data type from file name extensions
  template <typename Iter>
  FileType guessType(Iter begin, Iter end) {
    
    FileType type = Unknown;
    
    for ( ; begin != end; ++ begin) {
      
      std::string ext = fs::path(*begin).extension();
      FileType ftype = Unknown;
      if (ext == ".h5") {
        ftype = HDF5;
      } else if (ext == ".xtc") {
        ftype = XTC;
      }
      
      if (ftype == Unknown) return ftype;
      if (type == Unknown) {
        type = ftype;
      } else if (type == XTC or type == HDF5) {
        if (ftype != type) return Mixed;
      }      
    }

    return type;
  }
  
}
    
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
  AppUtils::AppCmdOpt<std::string> m_calibDirOpt ;
  AppUtils::AppCmdOpt<std::string> m_configOpt ;
  AppUtils::AppCmdOpt<std::string> m_expNameOpt ;
  AppUtils::AppCmdOpt<std::string> m_jobNameOpt ;
  AppUtils::AppCmdOptList<std::string>  m_modulesOpt;
  AppUtils::AppCmdOpt<unsigned> m_maxEventsOpt ;
  AppUtils::AppCmdOpt<unsigned> m_skipEventsOpt ;
  AppUtils::AppCmdArgList<std::string>  m_files;


};

//----------------
// Constructors --
//----------------
psanaapp::psanaapp ( const std::string& appName )
  : AppUtils::AppBase( appName )
  , m_calibDirOpt( 'b', "calib-dir", "path", "calibration directory name, may include {exp} and {instr}", "" )
  , m_configOpt( 'c', "config", "path", "configuration file, def: psana.cfg", "" )
  , m_expNameOpt( 'e', "experiment", "string", "experiment name, format: XPP:xpp12311 or xpp12311", "" )
  , m_jobNameOpt( 'j', "job-name", "string", "job name, def: from input files", "" )
  , m_modulesOpt( 'm', "module", "name", "module name, more than one possible" )
  , m_maxEventsOpt( 'n', "num-events", "number", "maximum number of events to process, def: all", 0U )
  , m_skipEventsOpt( 's', "skip-events", "number", "number of events to skip, def: 0", 0U )
  , m_files( "data-file",   "file name(s) with input data", std::list<std::string>() )
{
  addOption( m_calibDirOpt ) ;
  addOption( m_configOpt ) ;
  addOption( m_expNameOpt ) ;
  addOption( m_jobNameOpt ) ;
  addOption( m_modulesOpt ) ;
  addOption( m_maxEventsOpt ) ;
  addOption( m_skipEventsOpt ) ;
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

  // set instrument and experiment names if specified
  if (not m_expNameOpt.value().empty()) {
      std::string instrName;
      std::string expName = m_expNameOpt.value();
      std::string::size_type pos = expName.find(':');
      if (pos == std::string::npos) {
          instrName = expName.substr(0, 3);
          boost::to_upper(instrName);
      } else {
          instrName = expName.substr(0, pos);
          expName.erase(0, pos+1);
      }
      MsgLogRoot(debug, "cmd line: instrument = " << instrName << " experiment = " << expName);
      cfgsvc.put("psana", "instrument", instrName);
      cfgsvc.put("psana", "experiment", expName);
  }

  // set event numbers
  if (m_maxEventsOpt.value()) {
      cfgsvc.put("psana", "events", boost::lexical_cast<std::string>(m_maxEventsOpt.value()));
  }
  if (m_skipEventsOpt.value()) {
      MsgLogRoot(warning, "skip-events options is not supported at the moment");
      cfgsvc.put("psana", "skip-events", boost::lexical_cast<std::string>(m_skipEventsOpt.value()));
  }

  // set calib dir name if specified
  if (not m_calibDirOpt.value().empty()) {
    cfgsvc.put("psana", "calib-dir", m_calibDirOpt.value());
  }

  // get list of modules to load
  std::list<std::string> moduleNames = cfgsvc.getList("psana", "modules");
  
  // max number of events
  unsigned maxEvents = cfgsvc.get("psana", "events", 0xFFFFFFFFU);

  // list of files could come from config file and overriden by command line
  std::list<std::string> files(m_files.begin(), m_files.end());
  if (files.empty()) {
	cfgsvc.getList("psana", "files", std::list<std::string>());
  }
  if (files.empty()) {
    MsgLogRoot(error, "no input data specified");
    return 2;
  }
  
  DynLoader loader;

  // Load input module, by default use XTC input even if cannot correctly 
  // guess types of the input files
  std::string iname = "PSXtcInput.XtcInputModule";
  FileType ftype = ::guessType(files.begin(), files.end());
  if (ftype == Mixed) {
    MsgLogRoot(error, "Mixed input file types");
    return -1;
  } else if (ftype == HDF5) {
    iname = "PSHdf5Input.Hdf5InputModule";
  }
  psana::InputModule* input = loader.loadInputModule(iname);
  MsgLogRoot(trace, "Loaded input module " << iname);

  // pass file names to the configuration so that input module can find them
  typedef AppUtils::AppCmdArgList<std::string>::const_iterator FileIter;
  std::string flist;
  for (FileIter it = files.begin(); it != files.end(); ++it ) {
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
  if (jobName.empty() and not files.empty()) {
    boost::filesystem::path path = files.front();
    jobName = path.stem();
  }
  MsgLogRoot(debug, "job name = " << jobName);
  
  // get calib directory name
  std::string calibDir = cfgsvc.getStr("psana", "calib-dir", "/reg/d/psdm/{instr}/{exp}/calib");

  // instantiate experiment name provider
  boost::shared_ptr<PSEnv::IExpNameProvider> expNameProvider;
  if(not cfgsvc.getStr("psana", "experiment", "").empty()) {
    const std::string& instr = cfgsvc.getStr("psana", "instrument", "");
    const std::string& exp = cfgsvc.getStr("psana", "experiment", "");
    expNameProvider.reset(new ExpNameFromConfig (instr, exp));
  } else if (ftype == XTC) {
    expNameProvider.reset(new ExpNameFromXtc(files));
  } else {
    expNameProvider.reset(new ExpNameFromConfig ("", ""));
  }

  // Setup environment
  PSEnv::Env env(jobName, expNameProvider, calibDir);
  MsgLogRoot(debug, "instrument = " << env.instrument() << " experiment = " << env.experiment());
  MsgLogRoot(debug, "calibDir = " << env.calibDir());
  
  // Start with beginJob for everyone
  {
    input->beginJob(env);
    boost::shared_ptr<PSEvt::ProxyDict> dict(new PSEvt::ProxyDict);
    Event evt(dict);
    for (std::vector<Module*>::iterator it = modules.begin() ; it != modules.end() ; ++it) {
      (*it)->beginJob(evt, env);
    }
  }
    
  // event loop
  bool stop = false ;
  while ( maxEvents > 0 and not stop) {
    
    // Create event object
    boost::shared_ptr<PSEvt::ProxyDict> dict(new PSEvt::ProxyDict);
    Event evt(dict);
    
    // run input module to populate event
    InputModule::Status istat = input->event(evt, env);
    MsgLogRoot(debug, "input.event() returned " << istat);
    
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
      
      // dispatch event to particular method based on event type
      if (istat == InputModule::DoEvent) {
        mod.event(evt, env);
      } else if (istat == InputModule::BeginRun) {
        mod.beginRun(evt, env);
      } else if (istat == InputModule::BeginCalibCycle) {
        mod.beginCalibCycle(evt, env);
      } else if (istat == InputModule::EndCalibCycle) {
        mod.endCalibCycle(evt, env);
      } else if (istat == InputModule::EndRun) {
        mod.endRun(evt, env);
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
  {
    input->endJob(env);
    boost::shared_ptr<PSEvt::ProxyDict> dict(new PSEvt::ProxyDict);
    Event evt(dict);
    for (std::vector<Module*>::iterator it = modules.begin() ; it != modules.end() ; ++it) {
      (*it)->endJob(evt, env);
    }
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
