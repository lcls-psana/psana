//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class PSAna...
//
// Author List:
//      Andy Salnikov
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "psana/PSAna.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "ConfigSvc/ConfigSvc.h"
#include "ConfigSvc/ConfigSvcImplFile.h"
#include "MsgLogger/MsgLogger.h"
#include "psana/DynLoader.h"
#include "psana/ExpNameFromConfig.h"
#include "psana/ExpNameFromXtc.h"
#include "PSEnv/Env.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace fs = boost::filesystem ;

namespace {

  const char* logger = "PSAna";

  enum FileType { Unknown=-1, Mixed=0, XTC, HDF5 };

  // Function which tries to guess input data type from file name extensions
  template <typename Iter>
  FileType guessType(Iter begin, Iter end) {

    FileType type = Unknown;

    for ( ; begin != end; ++ begin) {

      std::string ext = fs::path(*begin).extension().string();
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

//----------------
// Constructors --
//----------------
PSAna::PSAna(const std::string& config, std::map<std::string, std::string>& options)
  : m_modules()
{

  // initialize configuration service, this can only be done once
  std::auto_ptr<ConfigSvc::ConfigSvcImplI> cfgImpl;
  if (config.empty()) {
    cfgImpl.reset( new ConfigSvc::ConfigSvcImplFile() );
  } else {
    cfgImpl.reset( new ConfigSvc::ConfigSvcImplFile(config) );
  }
  ConfigSvc::ConfigSvc::init(cfgImpl);
  ConfigSvc::ConfigSvc cfgsvc;

  // copy all options
  for (std::map<std::string, std::string>::const_iterator it = options.begin(); it != options.end(); ++ it) {
    std::string section;
    std::string option = it->first;
    std::string::size_type p = option.find('.');
    if (p == std::string::npos) {
      section = "psana";
    } else {
      section = std::string(option, 0, p);
      option.erase(0, p+1);
    }
    cfgsvc.put(section, option, it->second);
  }

  // get list of modules to load
  std::list<std::string> moduleNames = cfgsvc.getList("psana", "modules");

  // instantiate all user modules
  DynLoader loader;
  for ( std::list<std::string>::const_iterator it = moduleNames.begin(); it != moduleNames.end() ; ++ it ) {
    m_modules.push_back(loader.loadModule(*it));
    MsgLog(logger, trace, "Loaded module " << m_modules.back()->name());
  }


}

//--------------
// Destructor --
//--------------
PSAna::~PSAna ()
{
}

// Create data source instance for the set of input files/datasets.
DataSource
PSAna::dataSource(const std::vector<std::string>& input)
{
  ConfigSvc::ConfigSvc cfgsvc;

  DataSource dataSrc;

  // if input is empty try to use input from config file
  std::vector<std::string> inputList(input);
  if (inputList.empty()) {
    inputList = cfgsvc.getList("psana", "files", std::vector<std::string>());
  }
  if (inputList.empty()) {
    MsgLog(logger, error, "no input data specified");
    return dataSrc;
  }

  // Guess input data type, by default use XTC input even if cannot correctly
  // guess types of the input files
  std::string iname = "PSXtcInput.XtcInputModule";
  ::FileType ftype = ::guessType(inputList.begin(), inputList.end());
  if (ftype == Mixed) {
    MsgLog(logger, error, "Mixed input file types");
    return dataSrc;
  } else if (ftype == HDF5) {
    iname = "PSHdf5Input.Hdf5InputModule";
  }

  // pass file names to the configuration so that input module can find them
  std::string flist = boost::algorithm::join(inputList, " ");
  cfgsvc.put(iname, "files", flist);

  // Load input module
  DynLoader loader;
  boost::shared_ptr<psana::InputModule> inputModule(loader.loadInputModule(iname));
  MsgLog(logger, trace, "Loaded input module " << iname);

  // get calib directory name
  std::string calibDir = cfgsvc.getStr("psana", "calib-dir", "/reg/d/psdm/{instr}/{exp}/calib");

  // get/build job name
  std::string jobName = cfgsvc.getStr("psana", "job-name", "");
  if (jobName.empty() and not inputList.empty()) {
    boost::filesystem::path path = inputList.front();
    jobName = path.stem().string();
  }
  MsgLog(logger, debug, "job name = " << jobName);

  // instantiate experiment name provider
  boost::shared_ptr<PSEnv::IExpNameProvider> expNameProvider;
  if(not cfgsvc.getStr("psana", "experiment", "").empty()) {
    const std::string& instr = cfgsvc.getStr("psana", "instrument", "");
    const std::string& exp = cfgsvc.getStr("psana", "experiment", "");
    expNameProvider = boost::make_shared<ExpNameFromConfig>(instr, exp);
  } else if (ftype == XTC) {
    expNameProvider = boost::make_shared<ExpNameFromXtc>(inputList);
  } else {
    expNameProvider = boost::make_shared<ExpNameFromConfig>("", "");
  }

  // Setup environment
  boost::shared_ptr<PSEnv::Env> env = boost::make_shared<PSEnv::Env>(jobName, expNameProvider, calibDir);
  MsgLogRoot(debug, "instrument = " << env->instrument() << " experiment = " << env->experiment());
  MsgLogRoot(debug, "calibDir = " << env->calibDir());

  // make new instance
  dataSrc = DataSource(inputModule, m_modules, env);

  return dataSrc;
}

} // namespace psana
