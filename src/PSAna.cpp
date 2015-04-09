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
#include <signal.h>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "ConfigSvc/ConfigSvc.h"
#include "ConfigSvc/ConfigSvcImplFile.h"
#include "IData/Dataset.h"
#include "MsgLogger/MsgLogger.h"
#include "psana/DynLoader.h"
#include "psana/Exceptions.h"
#include "psana/ExpNameFromConfig.h"
#include "psana/ExpNameFromDs.h"
#include "psana/MPWorkerId.h"
#include "PSEnv/Env.h"

//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

namespace fs = boost::filesystem ;

namespace {

  const char* logger = "PSAna";

  enum FileType { Unknown=-1, Mixed=0, XTC, HDF5, SHMEM, IDX, SMALLDATA };

  std::map<std::string, FileType> getDsetInputKeys() {
    std::map<std::string, FileType> dsetInputKeys;
    dsetInputKeys["xtc"]=XTC;
    dsetInputKeys["h5"]=HDF5;
    dsetInputKeys["shmem"]=SHMEM;
    dsetInputKeys["idx"]=IDX;
    dsetInputKeys["smd"]=SMALLDATA;
    return dsetInputKeys;
  }

  // Function which tries to guess input data type from file name extensions
  template <typename Iter>
  FileType guessType(Iter begin, Iter end) {

    FileType type = Unknown;

    std::map<std::string, FileType> dsetInputKeys = getDsetInputKeys();

    for ( ; begin != end; ++ begin) {

      IData::Dataset ds(*begin);

      FileType ftype = Unknown;
      int numSpecifiersInDataset = 0;
      
      for (std::map<std::string, FileType>::iterator dsKey = dsetInputKeys.begin();
           dsKey != dsetInputKeys.end(); ++dsKey) {
        const std::string inputKey = dsKey->first;
        FileType dsFtype = dsKey->second;
        if (ds.exists(inputKey)) {
          ftype = dsFtype;
          numSpecifiersInDataset += 1;
        } 
      }
      
      if (numSpecifiersInDataset > 1) {
        MsgLog(logger, fatal, "More than one input source specified in dataset");
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
PSAna::PSAna(const std::string& config, const std::map<std::string, std::string>& options)
  : m_context(Context::generate())
  , m_modules()
{
  Context::set(m_context);

  // initialize configuration service, this can only be done once
  boost::shared_ptr<ConfigSvc::ConfigSvcImplI> cfgImpl = boost::make_shared<ConfigSvc::ConfigSvcImplFile>(config);
  ConfigSvc::ConfigSvc::init(cfgImpl, m_context);

  // for backward compaibility also initialize config service in global context
  if (not ConfigSvc::ConfigSvc::initialized()) {
    ConfigSvc::ConfigSvc::init(boost::make_shared<ConfigSvc::ConfigSvcImplFile>());
  }

  ConfigSvc::ConfigSvc cfgsvc(m_context);
  ConfigSvc::ConfigSvc glbcfgsvc;

  // copy all options
  for (std::map<std::string, std::string>::const_iterator it = options.begin(); it != options.end(); ++ it) {
    std::string section;
    std::string option = it->first;
    std::string::size_type p = option.rfind('.');
    if (p == std::string::npos) {
      section = "psana";
    } else {
      section = std::string(option, 0, p);
      option.erase(0, p+1);
    }
    cfgsvc.put(section, option, it->second);
    // and update global config as well
    glbcfgsvc.put(section, option, it->second);
  }
}

//--------------
// Destructor --
//--------------
PSAna::~PSAna ()
{
}

/**
 *  @brief Get the list of modules.
 */
std::vector<std::string>
PSAna::modules()
{
  ConfigSvc::ConfigSvc cfgsvc(m_context);
  std::vector<std::string> moduleNames = cfgsvc.getList("psana", "modules", std::vector<std::string>());
  return moduleNames;
}


// Create data source instance for the set of input files/datasets.
DataSource
PSAna::dataSource(const std::vector<std::string>& input)
{
  Context::set(m_context);

  ConfigSvc::ConfigSvc cfgsvc(m_context);

  DataSource dataSrc;

  // if input is empty try to use input from config file
  std::vector<std::string> inputList(input);
  if (inputList.empty()) {
    inputList = cfgsvc.getList("psana", "input", std::vector<std::string>());
  }
  if (inputList.empty()) {
    inputList = cfgsvc.getList("psana", "files", std::vector<std::string>());
  }
  if (inputList.empty()) {
    MsgLog(logger, error, "no input data specified");
    return dataSrc;
  }

  // get calib directory name
  const char* datadir = getenv("SIT_PSDM_DATA");
  std::string calibDirRoot;
  if (datadir) {
    calibDirRoot = datadir;
  } else {
    calibDirRoot = "/reg/d/psdm";
  } 
  boost::format fmt("%1%/%2%");
  fmt % calibDirRoot % "{instr}/{exp}/calib";
  std::string calibDirDefault = fmt.str();
  std::string calibDir = cfgsvc.getStr("psana", "calib-dir", calibDirDefault);

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
  } else {
    expNameProvider = boost::make_shared<ExpNameFromDs>(inputList);
  }

  // make AliasMap instance
  boost::shared_ptr<PSEvt::AliasMap> amap = boost::make_shared<AliasMap>();

  // Guess input data type
  ::FileType ftype = ::guessType(inputList.begin(), inputList.end());
  MsgLog(logger, debug, "input data type: " << int(ftype));
  if (ftype == Mixed) {
    MsgLog(logger, error, "Mixed input file types");
    return dataSrc;
  }

  // check if requested multi-process mode and it's compatible with input data
  int nworkers = cfgsvc.get("psana", "parallel", 0);
  switch (ftype) {
  case IDX:
    if (nworkers > 0) {
      MsgLog(logger, warning, "Multi-process mode is not available for IDX data, switching to single-process");
      nworkers = 0;
    }
    break;
  case HDF5:
    if (nworkers > 0) {
      MsgLog(logger, warning, "Multi-process mode is not available for HDF5 data, switching to single-process");
      nworkers = 0;
    }
    break;
  case XTC:
  case SHMEM:
  case SMALLDATA:
    // OK
    break;
  case Unknown:
    ftype = XTC;
    break;
  case Mixed:
    // should not happen
    break;
  }
  if (nworkers > 255) {
    MsgLog(logger, warning, "Number of workers exceeds limit, reduced to 255");
    nworkers = 255;
  }

  // in parallel mode start spawning workers, workerId will be -1 in master
  // and non-negative number in workers
  int workerId = -1;
  int readyPipe = -1;   // fd for ready pipe
  int dPipe = -1;   // fd for data pipe
  boost::shared_ptr<std::vector<MPWorkerId> > workers;
  if (nworkers > 0) {

    workers = boost::make_shared<std::vector<MPWorkerId> >();

    // make a pipe for ready queue
    int rPipe[2];
    pipe(rPipe);
    readyPipe = rPipe[0]; // to be used by master

    for (int iworker = 0; iworker < nworkers; ++ iworker) {

      // make a pipe for communication with worker
      int dataPipe[2];
      pipe(dataPipe);

      pid_t pid = fork();
      if (pid == -1) {

        // error happened, this is fatal
        throw ExceptionErrno(ERR_LOC, "fork failed");

      } else if (pid == 0) {

        // we are in the child (worker) process

        // close pipe ends that we don't use
        close(dataPipe[1]);
        close(rPipe[0]);

        workerId = iworker;
        readyPipe = rPipe[1];
        dPipe = dataPipe[0];

        // can cleanup some space
        workers.reset();

        MsgLog(logger, trace, "Forked worker #" << iworker << " dataPipeFd: " << dataPipe[0] << " readyPipe: " << readyPipe);

        break;

      } else {

        // we are still in parent process

        // close pipe ends that we don't use
        close(dataPipe[0]);

        // save worker info
        workers->push_back(MPWorkerId(iworker, pid, dataPipe[1]));
        MsgLog(logger, trace, "Add worker #" << iworker << " pid " << pid << " dataPipeFd " << dataPipe[1]);

      }

    }

    if (workerId < 0) {
      // close unused end of ready pipe in master
      ::close(rPipe[1]);
    }
  }


  // Guess input module name
  std::string iname;
  switch (ftype) {
  case XTC:
    if (nworkers <= 0) {
      // single-process input for XTC
      iname = "PSXtcInput.XtcInputModule";
    } else if (workerId < 0) {
      // master process in multi-process mode
      iname = "PSXtcMPInput.XtcMPMasterInput";
    } else {
      // worker process in multi-process mode
      iname = "PSXtcMPInput.XtcMPWorkerInput";
    }
    break;
  case SMALLDATA:
    if (nworkers <= 0) {
      // single-process input for SMALLDATA
      iname = "PSXtcInput.XtcInputModule"; 
    } else if (workerId < 0) {
      // master process in multi-process mode
      MsgLog(logger, fatal, "smldata not supported with parallel");
    } else {
      // worker process in multi-process mode
      MsgLog(logger, fatal, "smldata not supported with parallel");
    }
    break;
  case SHMEM:
    if (nworkers <= 0) {
      // single-process input for shmem XTC
      iname = "PSShmemInput.ShmemInputModule";
    } else if (workerId < 0) {
      // master process in multi-process mode
      iname = "PSXtcMPInput.ShmemMPMasterInput";
    } else {
      // worker process in multi-process mode
      iname = "PSXtcMPInput.XtcMPWorkerInput";
    }
    break;
  case HDF5:
    iname = "PSHdf5Input.Hdf5InputModule";
    break;
  case IDX:
    iname = "PSXtcInput.XtcIndexInputModule";
    break;
  case Unknown:
  case Mixed:
    // should not happen
    break;
  }

  // pass datasets/file names to the configuration so that input module can find them
  std::string flist = boost::join(inputList, " ");
  cfgsvc.put(iname, "input", flist);
  cfgsvc.put(iname, "files", flist);
  if (readyPipe >= 0) {
    cfgsvc.put(iname, "fdReadyPipe", boost::lexical_cast<std::string>(readyPipe));
  }
  if (workerId >= 0) {
    cfgsvc.put(iname, "workerId", boost::lexical_cast<std::string>(workerId));
  }
  if (dPipe >= 0) {
    cfgsvc.put(iname, "fdDataPipe", boost::lexical_cast<std::string>(dPipe));
  }

  // Load input module
  DynLoader loader;
  boost::shared_ptr<psana::InputModule> inputModule(loader.loadInputModule(iname));
  MsgLog(logger, trace, "Loaded input module " << iname);

  // Setup environment
  boost::shared_ptr<PSEnv::Env> env = boost::make_shared<PSEnv::Env>(jobName, expNameProvider, calibDir, amap, workerId);
  MsgLogRoot(debug, "instrument = " << env->instrument() << " experiment = " << env->experiment());
  MsgLogRoot(debug, "calibDir = " << env->calibDir());

  // instantiate all user modules
  if (nworkers > 0 and workerId < 0) {

    // master process in multi-process mode does not need any user modules

    // put workers info into environment so that it can be seen by master module
    env->configStore().put(workers, Pds::Src());

    // install special signal handler so that dying children do not turn into zombies
    // and writing to a pipe directed to dead worker does not cause crash
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, NULL);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, NULL);

  } else {

    // single process mode or worker process in multi-process mode

    // get list of modules to load
    std::vector<std::string> moduleNames = cfgsvc.getList("psana", "modules", std::vector<std::string>());

    // instantiate all user modules
    for ( std::vector<std::string>::const_iterator it = moduleNames.begin(); it != moduleNames.end() ; ++ it ) {
      m_modules.push_back(loader.loadModule(*it));
      MsgLog(logger, trace, "From psana modules, loaded module " << m_modules.back()->name());
    }
    if (moduleNames.size()==0) {
      MsgLog(logger, trace, "psana modules parameter is empty.");
    }

  }

  // make new instance
  dataSrc = DataSource(inputModule, m_modules, env);

  return dataSrc;
}

} // namespace psana
