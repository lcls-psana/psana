import _psana
import os

_cfgFile = None
_options = {}
_global_env = None

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
