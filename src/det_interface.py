from Detector.PyDetector import detector_factory as _detector_factory
import datasource

def _getEnv(local_env=None):
    if local_env is None:
        if datasource._global_env is None:
            raise RuntimeError('Detector object cannot be created before an instance'
                               ' of psana.DataSource exists')
        return datasource._global_env
    else:
        return local_env

def Detector(name, local_env=None):
    env = _getEnv(local_env)
    return _detector_factory(name, env)

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
