#--------------------------------------------------------------------------
# File and Version Information:
#  $Id$
#
# Description:
#  Module psana
#
#------------------------------------------------------------------------

"""High-level wrapper module for psana.

Typical usage::

    from psana import *
    
    setConfigFile('config-file.cfg')   # optional, default is to use psana.cfg if present
    setOption("psana.modules", "EventKeys")   # can set/change options from script

    # define "data source" specifying experiment and run number
    datasrc = DataSource("exp=cxi63112:run=111")

    # specify source device for data   
    cam_src = Source("CxiDg4.0:Tm6740.0")

    # loop over all events in a data source
    for evt in datasrc.events():
        frame = evt.get(Camera.FrameV1, cam_src)
        image = frame.data16()

This software was developed for the LCLS project.  If you use all or 
part of it, please give an appropriate acknowledgment.

@version $Id$

@author Andy Salnikov
"""

import sys
import os
import logging

if sys.platform.startswith('linux'):
    # on Linux with g++ one needs RTLD_GLOBAL for dlopen
    # which Python does not set by default
    import DLFCN
    flags = sys.getdlopenflags()
    sys.setdlopenflags( flags | DLFCN.RTLD_GLOBAL )    
    import _psana
    sys.setdlopenflags( flags )    
    del flags
    del DLFCN
else:
    import _psana
del sys

#
# import everything from _psana
#
from _psana import *

# this is not used here directly but still imported for documentation
from _psana import _DataSource


#
# Build the psana namespace from external packages
#
from Detector.PyDetector import detector_factory as _detector_factory

from XtcInput.PyLiveAvail import LiveAvail as LiveAvail

#from psparallel.mpi_datasource import MPIDataSource

#----------------------------------
# Local non-exported definitions --
#----------------------------------

_cfgFile = None
_options = {}
_global_env = None


def _getEnv(local_env=None):
    if local_env is None:
        if _global_env is None:
            raise RuntimeError('Detector object cannot be created before an instance'
                               ' of psana.DataSource exists')
        return _global_env
    else:
        return local_env

def Detector(name, local_env=None):
    env = _getEnv(local_env)
    return _detector_factory(name, env)

def setConfigFile(name):
    """
    Set the name of the psana configuration file, default configuration
    file name is psana.cfg. If you want to avoid reading any configuration
    file then set it to empty string. 
    
    Configuration file name set with setConfigFile() is used in a next call to DataSource().
    """
    global _cfgFile
    _cfgFile = name

def setOption(name, value):
    """
    Set the psana configuration option, this will override any configuration 
    option in a file. Option name has format "section.option" where "section" 
    is the name of the section in configuration file (such as "psana" or 
    "psana_examples.DumpPrinceton"). Value can be any string, possibly empty,
    non-string values will be converted to strings using str() call.

    Configuration options set with setOption() are used in a next call to DataSource().
    """
    global _options
    _options[name] = str(value)

def setOptions(mapping):
    """
    Set the psana configuration option, this will override any configuration 
    option in a file. Argument to this calls is a mapping (dictionary) whose
    keys are option names. Option name has format "section.option" where "section" 
    is the name of the section in configuration file (such as "psana" or 
    "psana_examples.DumpPrinceton"). Value can be any string, possibly empty,
    non-string values will be converted to strings using str() call.

    Configuration options set with setOptions() are used in a next call to DataSource().
    """
    global _options
    for key, val in mapping.items():
        _options[key] = str(val)

def _epicsNames(local_env=None):
    env = _getEnv(local_env)
    epics = env.epicsStore()
    # leave a placeholder for "user alias" from the calibdir
    return [(pv, epics.alias(pv),'') for pv in epics.pvNames()]

def _detNames(local_env=None):
    env = _getEnv(local_env)
    amap = env.aliasMap()
    namelist = []
    # leave a placeholder for "user alias" from the calibdir
    for s in amap.srcs():
        if str(s).find('EpicsArch')!=-1: continue
        namelist.append((str(s).split('(')[-1].split(')')[0],amap.alias(s),''))
    cstore = env.configStore()
    for k in cstore.keys():
        if 'ControlData.Config' in str(k.type()):
            namelist.append(('ControlData','',''))
            break
    return namelist

def DetNames(nametype='detectors',local_env=None):
    """
    Return tuples of detector names.  Nametype should be one of
    'detectors','epics','all'.  'detectors' returns the names of all standard
    detectors included in the data.  'epics' returns the names of all epics
    variables (epics variables are typically updated at 1Hz with information
    about slow quantities like temperatures, voltages, motor positions).
    'all' returns both of the above.

    Each detector can have 3 names: a "full name", a simpler "daq alias", or a
    "user alias".  The last two are optional.

    This routine takes an optional "DataSource.Env" argument.  If not provided
    the Env from the most recently created DataSource will be used.  It typically
    only needs to be specified when using multiple DataSource instances simultaneously.
    """
    if nametype=='detectors':
        return _detNames(local_env)
    elif nametype=='epics':
        return _epicsNames(local_env)
    elif nametype=='all':
        return _detNames(local_env)+_epicsNames(local_env)
    else:
        raise ValueError('Name type must be one of "detectors, epics, all"')

def DataSource(*args,**kwargs):
    """
    Makes an instance of the data source object (:py:class:`psana._DataSource`).
    Arguments can be either a single list of strings or any number of strings,
    each string represents either an input file name or event collection.
    """
    global _options, _cfgFile, _global_env
    # make instance of the framework
    cfgFile = _cfgFile
    if cfgFile is None:
        if os.path.exists("psana.cfg"): 
            cfgFile = "psana.cfg"
        else:
            cfgFile = ""
    fwk = _psana.PSAna(cfgFile, _options)


    # Create the PSANA datasource object
    ds = fwk.dataSource(*args)


    # Check if any keyword arguments given    
    # module keyword -- add module or list of modules
    if 'module' in kwargs:    
        # Add modules
        try :        
            for module in kwargs['module'] :
                ds.__add_module(module)
        except TypeError:
            # Incase a single module is added
            ds.__add_module(kwargs['module'])

    # inject the environment into the global namespace --TJL
    _global_env = ds.env()    
        
    # --> return the datasource object
    return ds


