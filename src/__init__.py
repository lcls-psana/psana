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
from __future__ import absolute_import

# We believe there is some non-ideal dependency behavior in this package.
# the psana __init__.py depends on psana_python by importing _psana.
# However _psana wraps the C++ code in the psana package, which
# is circular.  Perhaps ideally the C++ code would be moved to a
# different package, but this would break C++ users who would have
# to change their include statements.  We certainly want to keep
# __init__.py in psana so "import psana" works.
# - TJ Lane and Christopher O'Grady

import sys

# we believe this must be the FIRST import of _psana, so it's important
# that other software that imports _psana (like DataSource) happen
# after this.  one symptom we see if the order is wrong is that some
# of the boost converters stop working, but only on RHEL5.

if sys.platform.startswith('linux'):
    # on Linux with g++ one needs RTLD_GLOBAL for dlopen
    # which Python does not set by default
    import ctypes
    flags = sys.getdlopenflags()
    sys.setdlopenflags( flags | ctypes.RTLD_GLOBAL )    
    import _psana
    sys.setdlopenflags( flags )    
    del flags
    del ctypes
else:
    import _psana
del sys

from .datasource import DataSource, setOption, setOptions, setConfigFile
from .mpi_datasource import MPIDataSource
from .det_interface import Detector, DetNames

#
# import everything from _psana
#
from _psana import *

#
# Build the psana namespace from external packages
#
from XtcInput.PyLiveAvail import LiveAvail as LiveAvail
