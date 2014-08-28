//--------------------------------------------------------------------------
// File and Version Information:
//     $Id$
//
// Description:
//     Class PSAnaApp...
//
// Author List:
//     Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/PSAnaApp.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <boost/make_shared.hpp>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgFormatter.h"
#include "MsgLogger/MsgLogger.h"
#include "psana/Exceptions.h"
#include "psana/PSAna.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

//             ----------------------------------------
//             -- Public Function Member Definitions --
//             ----------------------------------------

namespace {

  const std::string dumpConfigFileOption("psana.dump_config_file");

  bool dumpConfigFileOptionSet(const std::map<std::string, std::string> &options) {
    std::map<std::string, std::string>::const_iterator pos = options.find(dumpConfigFileOption);
    if (pos == options.end()) {
      return false;
    }
    return true;
  }

  void dumpConfigFile(const std::string &cfgFile) {
    if (cfgFile.size()==0) return;
    std::cout << "--------- psana config file: " << cfgFile << " ------------" << std::endl;
    std::ifstream cfgFileStream;
    cfgFileStream.open(cfgFile.c_str(), std::ifstream::in);
    if ( (cfgFileStream.rdstate() & std::ifstream::failbit ) == 0 ) {
      char c = cfgFileStream.get();
      while (cfgFileStream.good()) {
        std::cout << c;
        c = cfgFileStream.get();
      }
      cfgFileStream.close();
    } else {
      std::cout << " ** unable to open file ** " << std::endl;
    }
    std::cout << std::endl << "------- end psana config file ---------" << std::endl;
  }

  void removeDumpConfigFileOption(std::map<std::string, std::string> &options) {
    std::map<std::string, std::string>::iterator pos = options.find(dumpConfigFileOption);
    if (pos != options.end()) {
      options.erase(pos);
    }
  }
    
} // local namespace

namespace psana {

//
//  Application class
//

//----------------
// Constructors --
//----------------
PSAnaApp::PSAnaApp ( const std::string& appName )
  : AppUtils::AppBase( appName )
  , m_calibDirOpt( parser(), "b,calib-dir", "path", "calibration directory name, may include {exp} and {instr}, if left empty then do not do calibrations", "" )
  , m_configOpt( parser(), "c,config", "path", "configuration file, by default use psana.cfg if it exists", "" )
  , m_expNameOpt( parser(), "e,experiment", "string", "experiment name, format: XPP:xpp12311 or xpp12311, by default guess it from data", "" )
  , m_jobNameOpt( parser(), "j,job-name", "string", "job name, default is to generate from input file names", "" )
  , m_modulesOpt( parser(), "m,module", "name", "module name, more than one possible" )
  , m_maxEventsOpt( parser(), "n,num-events", "number", "maximum number of events to process, 0 means all", 0U )
  , m_skipEventsOpt( parser(), "s,skip-events", "number", "number of events to skip", 0U )
  , m_parallelOpt( parser(), "p,num-cpu", "number", "number greater than 0 enables multi-processing", 0U )
  , m_optionsOpt( parser(), "o,option", "string", "configuration options, format: module.option[=value]" )
  , m_datasets( parser(), "dataset", "input dataset specification (list of file names or exp=cxi12345:run=123:...)", std::vector<std::string>() )
{
}

//--------------
// Destructor --
//--------------
PSAnaApp::~PSAnaApp ()
{
}

/**
 *  Run the application, accepts arguments as vector of strings which
 *  should contain the same values as regular argv (first element must be
 *  application name).
 */
int
PSAnaApp::run(const std::vector<std::string>& argv)
{
  // copy arguments
  size_t size = 0;
  for (std::vector<std::string>::const_iterator it = argv.begin(); it != argv.end(); ++ it) {
    size += it->size() + 1;
  }

  char* buf = new char[size];

  std::vector<char*> cargv;
  cargv.reserve(argv.size()+1);
  char* p = buf;
  for (std::vector<std::string>::const_iterator it = argv.begin(); it != argv.end(); ++ it) {
    cargv.push_back(p);
    p = std::copy(it->begin(), it->end(), p);
    *p++ = '\0';
  }
  cargv.push_back(0);

  // call standard method
  int code = this->run(argv.size(), &cargv[0]);

  // cleanup
  delete [] buf;

  return code;
}

/**
 *  Method called before runApp, can be overriden in subclasses.
 *  Usually if you override it, call base class method too.
 */
int
PSAnaApp::preRunApp ()
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
PSAnaApp::runApp ()
{
  std::string cfgFile;
  std::map<std::string, std::string> options;
  setConfigFileAndOptions(cfgFile, options);
  
  // Instantiate framework
  PSAna fwk(cfgFile, options);

  // check that we have at least one module
  if (fwk.modules().empty()) {
    MsgLogRoot(error, "no analysis modules specified");
    return 2;
  }

  // list of inputs
  std::vector<std::string> input = inputDataSets();

  // get data source
  DataSource dataSource = fwk.dataSource(input);
  if (dataSource.empty()) return 2;

  // get event iterator
  EventIter iter = dataSource.events();

  // loop from begin to end
  while (boost::shared_ptr<PSEvt::Event> evt = iter.next()) {
    // nothing to do here
  }

  // return 0 on success, other values for error (like main())
  return 0 ;
}

void
PSAnaApp::setConfigFileAndOptions(std::string &cfgFile, std::map<std::string, std::string> &options)
{
  cfgFile.clear();
  options.clear();
  // if -c is not specified the try to read psana.cfg (only if present)
  cfgFile = m_configOpt.value();
  if (not m_configOpt.valueChanged()) {
    if (access("psana.cfg", R_OK) == 0) {
      cfgFile = "psana.cfg";
    }
  }

  // command-line -m options override config file values
  if (not m_modulesOpt.value().empty()) {
    std::string modlist;
    const std::vector<std::string>& modules = m_modulesOpt.value();
    for(std::vector<std::string>::const_iterator it = modules.begin(); it != modules.end(); ++ it) {
      if (not modlist.empty()) modlist += ' ';
      modlist += *it;
    }
    MsgLogRoot(trace, "set module list to '" << modlist << "'");
    options["psana.modules"] = modlist;
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
      options["psana.instrument"] = instrName;
      options["psana.experiment"] = expName;
  }

  // set event numbers
  if (m_maxEventsOpt.value()) {
      options["psana.events"] = boost::lexical_cast<std::string>(m_maxEventsOpt.value());
  }
  if (m_skipEventsOpt.value()) {
      options["psana.skip-events"] = boost::lexical_cast<std::string>(m_skipEventsOpt.value());
  }

  // multi-processing
  if (m_parallelOpt.value()) {
      options["psana.parallel"] = boost::lexical_cast<std::string>(m_parallelOpt.value());
  }

  // set calib dir name if specified
  if (not m_calibDirOpt.value().empty()) {
    options["psana.calib-dir"] = m_calibDirOpt.value();
  }

  // now copy all -o options, they may override existing options
  typedef AppUtils::AppCmdOptList<std::string>::const_iterator OptIter;
  for (OptIter it = m_optionsOpt.begin(); it != m_optionsOpt.end(); ++ it) {
    std::string optname = *it;
    std::string optval;
    std::string::size_type p = optname.find('=');
    if (p != std::string::npos) {
      optval = std::string(optname, p+1);
      optname.erase(p);
    }
    options[optname] = optval;
  }

  // dump contents of config file if requested
  if (dumpConfigFileOptionSet(options)) {
    dumpConfigFile(cfgFile);
    removeDumpConfigFileOption(options);
  }
}

std::vector<std::string> 
PSAnaApp::inputDataSets() const 
{
  std::vector<std::string> input(m_datasets.begin(), m_datasets.end());
  return input;
}

} // namespace psana
