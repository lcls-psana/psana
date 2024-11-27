
"""
Code for parallel processing of event loop data.

-- Chris O'Grady and TJ Lane
Jan 2017

-------------------------------------------------------------

> metadata
-- docstrings for default values
-- user interface for adding attributes/info/units (e.g. smld.attribute(a='a is great'))
-- xarray compatability?

> chunking for performance?

>>> from Silke
- think about cube problem
- user-controlled mode for event distribution (e.g. based on delay-time)

>>> xarray thoughts from Jason:
- coordinates/attributes for everything, using h5netcdf
- for analyzing the small-data: xarray makes it easy to select/filter,
  and will keep track of coordinates for you
- can use pandas for plotting and packages like seaborn (stat plotting)
  with these coordinate arrays
- the xarray coordinates will provide the time sorting
- wildcard to merge multiple files
- didn't support variable length data? (maybe can do this)
- treats NaNs correctly
- would merge fast and slow detectors (e.g. eventcode 40 and 42)
- handles dropped data
- not clear that xarray can write pieces while taking data?
- probably supports hierarchy of hdf5 groups in h5netcdf, but might
  make it more difficult to cut/merge/sort in xarray
"""


import numpy as np
import tables
import collections
import warnings

from _psana import EventId, Source, Bld
from psana import Detector, DetNames

try:
    from pscache import publisher
    _PSCACHE_AVAILABLE = True
except ImportError as e:
    _PSCACHE_AVAILABLE = False

from mpi4py import MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

MISSING_INT = -99999
MISSING_FLOAT = np.nan

INT_TYPES   = [int, np.int8, np.int16, np.int32, np.int64,
               np.uint8, np.uint16, np.uint32, np.uint64]
FLOAT_TYPES = [float, np.float16, np.float32, np.float64, np.float128]

RAGGED_PREFIX = 'ragged_'
VAR_PREFIX = 'var_'
LEN_SUFFIX = '_len'

# Let's just keep track of variable, length, and ragged keys.
#
# Now, in the New Regime, "var_" can appear at any level in the key.
# We assume that the lengths of all the variables that branch from a
# var will be the same and will share a "_len" array.
# We add len_map to map from key to the "_len" name, and len_evt
# which will map from "_len" name to a fiducial to size mapping.
# (If there is no mapping, we'll know that this is the first var array
# for that _len array.)
var_dict = {}
len_dict = {}
len_map = {}     # var name to len name
len_evt = {}     # len name to {fid: size}
ragged_dict = {}

def set_keytypes(k):
    parts = k.split('/')
    ragged_dict[k] = parts[-1].startswith(RAGGED_PREFIX)
    # New regime: any part can have "var_"!
    for (i,p) in enumerate(parts):
        if p.startswith(VAR_PREFIX):
            var_dict[k] = True
            if p.endswith(LEN_SUFFIX) and i == len(parts) - 1:
                len_dict[k] = True
            else:
                m = '/'.join(parts[:i+1]) + LEN_SUFFIX
                len_map[k] = m
                len_dict[k] = False
                len_dict[m] = True
                if m not in len_evt.keys():
                    len_evt[m] = {}
            return
    var_dict[k] = False
    len_dict[k] = False

def get_len_map(k):
    try:
        return len_map[k]
    except:
        set_keytypes(k)
        return len_map[k]

def is_len_key(k):
    try:
        return len_dict[k]
    except:
        set_keytypes(k)
        return len_dict[k]

def is_var_key(k):
    try:
        return var_dict[k]
    except:
        set_keytypes(k)
        return var_dict[k]

def is_ragged_key(k):
    try:
        return ragged_dict[k]
    except:
        set_keytypes(k)
        return ragged_dict[k]


def _flatten_dictionary(d, parent_key='', sep='/'):
    """
    http://stackoverflow.com/questions/6027558/flatten-nested-python-dictionaries-compressing-keys
    """
    items = []
    for k, v in d.items():
        new_key = parent_key + sep + k if parent_key else k
        if isinstance(v, collections.MutableMapping):
            items.extend(list(_flatten_dictionary(v, new_key, sep=sep).items()))
        else:
            items.append((new_key, v))
    return dict(items)


def remove_zero_len(the_list):
    """
    Remove all items with value `val` from `the_list`
    """
    return [value for value in the_list if len(value) != 0]


def num_or_array(obj):
    """
    Returns string 'num' or 'array' if object is a number
    (int, float) or array.
    """

    data_type = type(obj)
    if ((data_type in [int, float]) or
         np.issubdtype(data_type, np.integer) or
         np.issubdtype(data_type, np.floating)):
        s = 'num'

    elif data_type is np.ndarray:
        s = 'array'

    else:
        raise TypeError('object is not number or array')

    return s


class SynchDict(dict):
    """
    Class used to keep track of all arrays that need to be gathered
    (a.k.a. "send_list").  This dictionary is synched before every call
    to gather.
    """

    def synchronize(self):
        """
        Synchronize all keys in the dictionary (across MPI ranks)
        """
        tot_send_list = comm.allgather(self)
        for node_send_list in tot_send_list:
            for k in node_send_list:
                if k not in list(self.keys()):
                    self[k] = node_send_list[k]
        # now that all ranks have everybody's keys, update all ranks
        # with the "correct" dtypes from rank0 (other ranks may have
        # different dtypes) this ensures that all ranks use the same
        # dtype for Gatherv.
        master_send_list = comm.bcast(self)
        for k in self.keys():
            if self[k][0] != master_send_list[k][0]:
                warnings.warn('changing data type of key %s '
                              'from %s to %s on rank %d' %
                              (k,self[k][0],master_send_list[k][0],rank))
                self[k][0] = master_send_list[k][0]
        return


    def keys(self):
        """
        Return the dictionary keys
        """
        # this helps ensure that we call "gather" in the right order
        # on all cores
        return sorted(self)


class SmallData(object):
    """
    On master,
      * numbers (int/float) are lists of arrays, representing workers (items
        in the list) and events (items in the array)
      * arrays are lists of lists (!) of arrays, representing workers (items
        in the outter list) and events (items in the inner list) and values
        of the original array (arrays)

    """

    def __init__(self, datasource_parent, filename=None, keys_to_save=[], filters=None):
        """
        Parameters
        ----------
        datasource_parent : psana.DataSource
            The data source from which to generate small data

        filename : str
            The full path filename at which to save (in HDF5 format)

        keys_to_save : list of strings
            A list of event data keys to save to disk. For example, if you
            add only ['a'] to this list, but call SmallData.event(a=x, b=y),
            only the values for 'a' will be saved. Hierarchical data structures
            using dictionaries can be referenced using '/' for each level, e.g.
            SmallData.event({'c' : {'d' : z}}) --> keys_to_save=['c/d'].i

        filters: tables.Filters
            Filter properties for the smalldata h5 file. Mainly
            used to configure compression settings.
        """

        self.rank = rank
        self._datasource_parent = datasource_parent
        self._num_send_list     = SynchDict()
        self._arr_send_list     = SynchDict()
        self._monitors          = [] # aka "callbacks"

        self._initialize_default_detectors()

        # storage space for event data
        self._dlist = {}
        if self.master:
            self._dlist_master = {}
            self._newkeys = []

        if filename and self.master:
            self._small_file = SmallFile(filename, keys_to_save, filters=filters)
            self.add_monitor_function(self._small_file.save_event_data)

        return



    @property
    def master(self):
        return (rank == 0)


    @property
    def currevt(self):
        return self._datasource_parent._currevt


    @property
    def currrun(self):
        return self._datasource_parent._currrun


    def add_monitor_function(self, fxn):
        """
        Add a monitor function that will intermittently operate on the
        event data collected so far.

        After every gather, the SmallData object calls all "monitor" functions
        and passes them a dictionary of the event data. E.g. if you are adding
        data with 

        >>> smalldata.event(a=x, b=y)

        then on gather, SmallData will automatically call

        >>> fxn({a : x, b : y})

        for all monitors added.

        Parameters
        ----------
        fxn : function
            The callback function
        """
        self._monitors.append(fxn)
        return


    def save(self, *args, **kwargs):
        """
        Save summary data to an HDF5 file (e.g. at the end of an event loop).

            1. Add data using key-value pairs (similar to SmallData.event())
            2. Add data organized in hierarchy using nested dictionaries (similar to 
               SmallData.event())

        These data are then saved to the file specifed in the SmallData
        constructor.

        Parameters
        ----------
        *args : dictionaries
            In direct analogy to the SmallData.event call, you can also pass
            HDF5 group heirarchies using nested dictionaries. Each level
            of the dictionary is a level in the HDF5 group heirarchy.

        **kwargs : datasetname, dataset
            Similar to SmallData.event, it is possible to save arbitrary
            singleton data (e.g. at the end of a run, to save an average over
            some quanitity).

        Examples
        --------
        >>> # save "average_over_run"
        >>> smldata.save(average_over_run=x)
        
        >>> # save "/base/next_group/data"
        >>> smldata.save({'base': {'next_group' : data}})
        """

        self.gather()
        if self.master:
            if hasattr(self, '_small_file'):
                self._small_file.save(*args, **kwargs)
            else:
                raise IOError('No filename to save to provided')

        return


    def close(self):
        """
        Close the HDF5 file used for writing.
        """
        # avoids pytables warning
        if self.master:
            if hasattr(self, '_small_file'):
                self._small_file.close()
        return


    def connect_redis(self, keys=None):
        """
        Connect the smalldata object to LCLS's redis database, where
        all event data will be sent.
        
        Every gather, all event data obtained will be added to a
        database on psdb3, where it is accessible through a pscache
        client.

        NOTE: Using this feature may affect your performance. There
        are two things to worry about:

            1. If you send big data over the network, it may take a
               while... you will notice your MPIDataSource iteration
               rate slowing down.

           2. The redis database has limited memory. You can interact
              with the redis db through pscache to optimize e.g. how
              long it keeps things in memory to squeeze the most out
              of this hard limit.

        Parameters
        ----------
        keys : list
            A list of the subset of keys to send to redis. Useful
            to filter out big data that may cause performance issues
            either by taking up a lot of memory in the redis DB or
            by taking a long time to send over the wire. `None`
            (default) means send all.
        """

        if not _PSCACHE_AVAILABLE:
            raise ImportError('package `pscache` needs to be installed'
                              ' to use the redis feature!')

        # TODO / Note
        # Right now we set the experiment id, but then we don't even
        # really use it. Later we will need to use it to send to a 
        # unique redis DB for each expt.

        # ALSO: it may be better to have the MPIDataSource provide
        # an interface telling us what experiment it is using...

        expt = self._datasource_parent.experiment
        run = self._datasource_parent._currrun

        self._redispub = publisher.ExptPublisher(expt, host='psdb3')
        self._redismon = self._redispub.smalldata_monitor(run,
                                                          keys=keys)

        self.add_monitor_function(self._redismon)

        return


    def _missing(self, key):
        """
        Return an example (single instance) of the missing data value
        for `key`.

        Parameters
        ----------
        key : str
            The event data key
        """

        if key in self._num_send_list.keys():
            dtype = self._num_send_list[key][0]

            if dtype in INT_TYPES:
                if is_len_key(key):
                    missing_value = 0
                else:
                    missing_value = MISSING_INT
            elif dtype in FLOAT_TYPES:
                missing_value = MISSING_FLOAT
            else:
                raise ValueError('%s :: Invalid num type for missing data' % str(dtype))


        elif key in list(self._arr_send_list.keys()):

            dtype = self._arr_send_list[key][0]
            shape = self._arr_send_list[key][1]

            # for vlen case, fill missing values with len 0 array
            if is_ragged_key(key):
                missing_value = np.empty(0, dtype=dtype)

            # otherwise, use a fixed sized array to maintain square shape
            else:
                missing_value = np.empty(shape, dtype=dtype)

                if dtype in INT_TYPES:
                    missing_value.fill(MISSING_INT)
                elif dtype in FLOAT_TYPES:
                    missing_value.fill(MISSING_FLOAT)
                else:
                    raise ValueError('%s :: Invalid array type for missing data' % str(dtype))

        else:
            raise KeyError('key %s not found in array or number send_list' % key)

        return missing_value


    def gather(self):
        """
        Gather arrays and numbers from all MPI ranks.
        """

        # "send lists" hold a catalogue of the data keys to expect
        # aggregated across all ranks
        self._arr_send_list.synchronize()
        self._num_send_list.synchronize()

        # for all data in our aggregated catalogue, gather
        for k in self._arr_send_list.keys():
            if k not in list(self._dlist.keys()): self._dlist[k] = []
            self._backfill_client(self._nevents, self._dlist[k], k)
            self._gather_arrays(self._dlist[k], k)

        for k in self._num_send_list.keys():
            if k not in list(self._dlist.keys()): self._dlist[k] = []
            self._backfill_client(self._nevents, self._dlist[k], k)
            self._gather_numbers(self._dlist[k], k)

        self._dlist = {}  # forget all the data we just sent

        # if we are the master:
        #   (1) sort data by time
        #   (2) backfill missing data [to beginning of time, memory + disk]
        #       this must be after the sort, because there are no timestamps
        #       to sort backfilled data
        #   (3) re-shape dlist master data into single np array (per key)
        #   (4) callback monitor functions (includes save, if requested)
        #   (5) clear the dlist_master

        # note that _dlist_master is different than the client _dlist's.  it is a list of lists,
        # one list for each gather that has happened since the last save.  the style of
        # the contents of _dlist_master is also changed by the "sort" call: individual numbers
        # (not arrays) in a gather are originally arrays, but the sort converts them into lists
        # of numbers.

        if self.master:
            # handle the case of zero events
            if len(list(self._dlist_master.keys())) > 0:

                # (1) sort data by time

                # get the order to sort in from the event times
                sort_map = np.argsort(self._dlist_master['event_time'][-1])

                # Backfill the variable arrays with empty arrays.
                for k in self._dlist_master.keys():
                    if is_var_key(k) and not is_len_key(k):
                        l = self._dlist_master[k][-1]
                        for i,v in enumerate(self._dlist_master[get_len_map(k)][-1]):
                            if v == 0:
                                l.insert(i, [])
                        self._dlist_master[k][-1] = l

                for k in self._dlist_master.keys():

                    # "-1" here says we are only sorting the result from the most recent gather
                    self._dlist_master[k][-1] = [self._dlist_master[k][-1][i] for i in sort_map]
            

                # (2) backfill missing data
                for k in self._newkeys:
                
                    if is_var_key(k) and not is_len_key(k):
                        continue  # No need to backfill variable arrays!

                    events_in_mem = sum([len(x) for x in self._dlist_master['fiducials']])
                    target_events = events_in_mem
                    if hasattr(self, '_small_file'):
                        target_events += self._small_file.nevents_on_disk

                    self._dlist_master[k] = self._backfill_master(target_events, 
                                                                  self._dlist_master[k], 
                                                                  self._missing(k))
                self._newkeys = []

                # (3) re-shape dlist master data into single np array (per key)
                for k in self._dlist_master.keys():
                    if is_var_key(k) and not is_len_key(k):
                        # var data needs to be concatenated right now!
                        self._dlist_master[k][-1] = remove_zero_len(self._dlist_master[k][-1])
                        if len(self._dlist_master[k][-1]) > 0:
                            self._dlist_master[k][-1] = np.concatenate(self._dlist_master[k][-1])
                        else:
                            self._dlist_master[k][-1] = np.array([])
                    self._dlist_master[k] = remove_zero_len(self._dlist_master[k])
                    if len(self._dlist_master[k]) > 0: # dont crash np.concatenate
                        self._dlist_master[k] = np.concatenate(self._dlist_master[k])
                    else:
                        self._dlist_master[k] = np.array([])

                # (4) callback monitor functions (includes save, if requested)
                for f in self._monitors:
                    f(self._dlist_master)

                # (5) clear the dlist_master
                for k in self._dlist_master.keys():
                    self._dlist_master[k] = []

        return


    def _gather_numbers(self, num_list, key):
        """
        Gather numbers (int/float) from all workers and update master's dlist
        """

        lengths = np.array(comm.gather(len(num_list))) # get list of lengths
        mysend = np.array(num_list,dtype=self._num_send_list[key][0])
        myrecv = None

        if self.master:
            myrecv = np.empty((sum(lengths)),mysend.dtype) # allocate receive buffer

        try:
            comm.Gatherv(sendbuf=mysend, recvbuf=[myrecv, lengths])
        except Exception:
            raise Exception('smalldata.py:_gather_numbers Gatherv exception: rank %d with key %s.' % (rank,key))

        if self.master:
            self._dlist_append(self._dlist_master, key, myrecv)

        return


    def _gather_arrays(self, array_list, key):
        """
        Gather arrays from all workers and update the master's dlist
        """

        # send to master the shape of each array to expect
        # on rank 0 (master), `worker_shps` is a list of list of tuples:
        # first list:  one item for each rank
        # second list: one item for each array
        # tuple:       the shape of the array
        worker_shps = comm.gather([ a.shape for a in array_list ])

        expected_type = self._arr_send_list[key][0]
        # workers flatten arrays and send those to master
        if len(array_list) > 0:
            mysend = np.concatenate([ x.reshape(-1) for x in array_list ])
            if mysend.dtype is not expected_type:
                mysend = mysend.astype(expected_type)
            mysend = np.ascontiguousarray(mysend)
        else:
            mysend = np.array([], dtype=expected_type)

        # master computes how many array elements to expect, 
        # recvs in linear array

        if self.master:
            # flattened size of each array
            worker_lens = [[np.product(shp) for shp in worker] for worker in worker_shps]

            # size of msg from each rank
            worker_msg_sizes = [np.sum(lens,dtype=int) for lens in worker_lens]
            recv_buff_size = np.sum(worker_msg_sizes,dtype=int)

            # allocate receive buffer
            myrecv = np.empty(recv_buff_size, mysend.dtype)
            recvbuf = [myrecv, worker_msg_sizes]
        else:
            recvbuf = None

        try:
            comm.Gatherv(sendbuf=mysend, recvbuf=recvbuf)
        except Exception:
            raise Exception('smalldata.py:_gather_arrays Gatherv exception: rank %d with key %s.' % (rank,key))
        
        # master re-shapes linear received message into a list of well-shaped arrays
        # the array shapes were communicated previously
        if self.master:
            start = 0
            reshaped_recv = []
            for worker in worker_shps:
                for shp in worker:
                    l = np.product(shp)
                    reshaped_recv.append( myrecv[start:start+l].reshape(*shp) )
                    start += l

            # finally, insert the new arrays into master's dlist
            self._dlist_append(self._dlist_master, key, reshaped_recv)

        return


    @property
    def _nevents(self):
        """
        Number of events in memory
        """
        if 'fiducials' in self._dlist:
            return len(self._dlist['fiducials'])
        else:
            return 0


    # missing data ideas:
    # 
    # - do an allgather of the keys/types/shapes into "send_list"
    # 
    # * client-side issues:
    # - (case 1) when we append data to _dlist we will backfill numbers before adding the new data, roughly done.
    # - (case 2) when we gather we check for keys that we didn't see at all OR keys that are "ragged" and backfill.
    # - (case 3) no events at all on a client
    # - need to construct the missing data for each of the above cases
    # 
    # * master-side issues:
    # - (case 4) master also needs to backfill on disk and memory

    def _backfill_client(self, target_events, dlist_element, key):
        if is_var_key(key) and not is_len_key(key):
            return # No need to backfill var arrays!
        numfill = target_events - len(dlist_element)
        if numfill > 0:
            fill_value = self._missing(key)
            dlist_element.extend([fill_value]*numfill)
        return


    @staticmethod
    def _backfill_master(target_events, dlist_element, fill_value):
        numfill = target_events - sum([len(x) for x in dlist_element])
        if numfill > 0:
            dlist_element = [[fill_value]*numfill] + dlist_element
        return dlist_element


    def _dlist_append_client(self, key, value, fid):

        data_type = type(value)
        lenkey = None
        is_var = False

        if data_type is np.ndarray:
            value = np.atleast_1d(value)
            if is_ragged_key(key):
                if len(value.shape)>1:
                    raise ValueError('Currently only support 1D ragged arrays'
                                     'for HDF5 dataset name "'+key+'"')
            if is_var_key(key):
                if is_len_key(key):
                    raise ValueError('Variable arrays cannot end with "'+_LEN_PREFIX+'" for dataset name "'+key+'"')
                if len(value) == 0: # Just skip 0 length arrays!!
                    return
                is_var = True
                lenkey = get_len_map(key)
                if key not in self._arr_send_list:
                    # this may get over-ruled by the SynchDict.  But we assume "var" keys are variable in the first
                    # dimension.
                    self._arr_send_list[key] = [value.dtype, value.shape[1:]]
                if lenkey not in self._num_send_list:
                    self._num_send_list[lenkey] = [int]
            else:
                if key not in self._arr_send_list:
                    # this may get over-ruled by the SynchDict
                    self._arr_send_list[key] = [value.dtype, value.shape]
        else:
            if key not in self._num_send_list:
                self._num_send_list[key] = [data_type]

        if key not in list(self._dlist.keys()):
            self._dlist[key] = []
        if is_var and lenkey not in list(self._dlist.keys()):
            self._dlist[lenkey] = []

        # patch up _dlist with missing data before we add new values
        if is_var:
            # Backfill the lengths with 0.  We don't need to backfill the 
            # data, since the lengths we just filled are zero!  Moreover,
            # we only need to backfill if we *haven't* backfilled previously!
            if fid not in len_evt[lenkey].keys():
                self._backfill_client(self._nevents - 1, self._dlist[lenkey], lenkey)
        else:
            self._backfill_client(self._nevents - 1, self._dlist[key], key)

        if data_type is np.ndarray:
            # save a copy of the array (not a reference) in case the
            # user reuses the same array memory for the next event
            self._dlist[key].append(np.copy(value))
            if is_var:
                # Now, if this was a var length array, we might have to
                # save/check the length!
                try:
                    s = len_evt[lenkey][fid]
                except:
                    s = -1
                if s >= 0:
                    if s != len(value):
                        raise Exception("Key %s does not have expected size %d (actually %d)"
                                        % (key, s, len(value)))
                else:
                    len_evt[lenkey] = {fid: len(value)}
                    self._dlist[lenkey].append(len(value))
        else:
            self._dlist[key].append(value)

        return


    def _dlist_append(self, dlist, key, value):

        if key not in list(dlist.keys()):
            dlist[key] = [value]
            self._newkeys.append(key)
        else:
            dlist[key].append(value)

        return


    def event(self, *args, **kwargs):
        """
        Save some data from this event for later use.

        Parameters
        ----------
        *args : dictionaries
            Save HDF5 group heirarchies using nested dictionaries. Each level
            of the dictionary is a level in the HDF5 group heirarchy.

        **kwargs : datasetname, dataset
            Save an arbitrary piece of data from this run. The kwarg will
            give it an (HDF5 dataset) name that appears in the resulting 
            HDF5 file.

        Examples
        --------
        >>> # save the status of a laser
        >>> smldata.event(laser_on=laser_on)
        
        >>> # save "data' at a special location "/base/next_group/data"
        >>> smldata.event({'base': {'next_group' : data}})
        """

        # get timestamp data for most recently yielded evt
        evt_id = self.currevt.get(EventId)
        if evt_id is None: return  # can't do anything without a timestamp

        if ('event_time' in list(kwargs.keys())) or ('fiducials' in list(kwargs.keys())):
            raise KeyError('`event_time` and `fiducials` are special names'
                           ' reserved for timestamping -- choose a different '
                           'name')

        # *args can be used to pass hdf5 hierarchies (groups/names) in a dict
        # flatten these and create a single dictionary of (name : value) pairs
        # when groups will be created in the hdf5, we will use a "/"

        event_data_dict = {}
        event_data_dict.update(kwargs)
        for d in args:
            event_data_dict.update( _flatten_dictionary(d) )


        time = evt_id.time()[0] << 32 | evt_id.time()[1] # chris' craziness
        fid  = evt_id.fiducials()

        # --> check to see if we already save the time for this event
        if ('fiducials' in list(self._dlist.keys())) and (self._dlist['fiducials'][-1] == fid):

            # check to see if we already added this field this event
            events_seen = len(self._dlist['fiducials'])
            for k in event_data_dict.keys():

                # if the list is as long as the # evts seen, 
                # user has tried to add key twice
                if len( self._dlist.get(k, []) ) == events_seen:        # False if key not in dlist
                    warnings.warn('Found duplicate key %s for event with time/fiducials %d/%d. Could be caused by DAQ split event.' % (k,time,fid))

        # --> otherwise this is a new event
        else:
            self._dlist_append_client('event_time', time, fid)
            self._dlist_append_client('fiducials', fid, fid)
            self._event_default()

        for k in event_data_dict.keys():
            self._dlist_append_client(k, event_data_dict[k], fid)

        return


    def _initialize_default_detectors(self):

        self._default_detectors = []

        for detector_name in ['EBeam', 'PhaseCavity', 'FEEGasDetEnergy']:
            try:
                det = Detector(detector_name)
                self._default_detectors.append(det)
            except KeyError as e:
                pass

        # add all Evrs (there may be more than one)
        self._evr_cfgcodes = [] 
        for n in DetNames():
            if ':Evr.' in n[0]:
                det = Detector(n[0])
                self._default_detectors.append(det)

                cfg = det._fetch_configs()
                assert len(cfg) == 1
                self._evr_cfgcodes += [e.code() for e in cfg[0].eventcodes()]

        self._evr_cfgcodes = set(self._evr_cfgcodes)
        self._evr_cfgcodes = list(self._evr_cfgcodes)
        self._evr_cfgcodes.sort()

        return


    def _event_default(self):
        """
        Cherry-picked machine parameters we think will be useful
        for basically every experiment
        """

        default = {}

        for det in self._default_detectors:

            if det.name.dev == 'EBeam':
                ebeam_ddl = det.get(self.currevt)
                if ebeam_ddl is not None:
                    default['ebeam/charge']        = ebeam_ddl.ebeamCharge()
                    if hasattr(ebeam_ddl,'ebeamDumpCharge'):
                        default['ebeam/dump_charge']   = ebeam_ddl.ebeamDumpCharge()
                    default['ebeam/L3_energy']     = ebeam_ddl.ebeamL3Energy()
                    # check existence of this for old data
                    if hasattr(ebeam_ddl,'ebeamPhotonEnergy'):
                        default['ebeam/photon_energy'] = ebeam_ddl.ebeamPhotonEnergy()
                    default['ebeam/pk_curr_bc2']  = ebeam_ddl.ebeamPkCurrBC2()
                    default['ebeam/LTU_pos_x'] = ebeam_ddl.ebeamLTUPosX()
                    default['ebeam/LTU_pos_y'] = ebeam_ddl.ebeamLTUPosY()
                    default['ebeam/LTU_ang_x'] = ebeam_ddl.ebeamLTUAngX()
                    default['ebeam/LTU_ang_y'] = ebeam_ddl.ebeamLTUAngY()
                    if hasattr(ebeam_ddl,'ebeamUndPosX'):
                        default['ebeam/und_pos_x'] = ebeam_ddl.ebeamUndPosX()
                    if hasattr(ebeam_ddl,'ebeamUndPosY'):
                        default['ebeam/und_pos_y'] = ebeam_ddl.ebeamUndPosY()
                    if hasattr(ebeam_ddl,'ebeamUndAngX'):
                        default['ebeam/und_ang_x'] = ebeam_ddl.ebeamUndAngX()
                    if hasattr(ebeam_ddl,'ebeamUndAngY'):
                        default['ebeam/und_ang_y'] = ebeam_ddl.ebeamUndAngY()
                    if hasattr(ebeam_ddl,'ebeamXTCAVAmpl'):
                        default['ebeam/XTCAV_ampl'] = ebeam_ddl.ebeamXTCAVAmpl()
                    if hasattr(ebeam_ddl,'ebeamXTCAVPhase'):
                        default['ebeam/XTCAV_phase'] = ebeam_ddl.ebeamXTCAVPhase()
                    if hasattr(ebeam_ddl,'ebeamLTU250'):
                        default['ebeam/LTU_250'] = ebeam_ddl.ebeamLTU250()
                    if hasattr(ebeam_ddl,'ebeamLTU450'):
                        default['ebeam/LTU_450'] = ebeam_ddl.ebeamLTU450()

            if det.name.dev == 'PhaseCavity':
                pc_ddl = det.get(self.currevt)
                if pc_ddl is not None:
                    default['phase_cav/charge1']    = pc_ddl.charge1()
                    default['phase_cav/charge2']    = pc_ddl.charge2()
                    default['phase_cav/fit_time_1'] = pc_ddl.fitTime1()
                    default['phase_cav/fit_time_2'] = pc_ddl.fitTime2()

            if det.name.dev == 'FEEGasDetEnergy':
                gdet_ddl = det.get(self.currevt)
                if gdet_ddl is not None:
                    default['gas_detector/f_11_ENRC'] = gdet_ddl.f_11_ENRC()
                    default['gas_detector/f_12_ENRC'] = gdet_ddl.f_12_ENRC()
                    default['gas_detector/f_21_ENRC'] = gdet_ddl.f_21_ENRC()
                    default['gas_detector/f_22_ENRC'] = gdet_ddl.f_22_ENRC()
                    # check existence of this for old data
                    if hasattr(gdet_ddl,'f_63_ENRC'):
                        default['gas_detector/f_63_ENRC'] = gdet_ddl.f_63_ENRC()
                        default['gas_detector/f_64_ENRC'] = gdet_ddl.f_64_ENRC()

            if det.name.dev == 'Evr':
                evr_codes = det.eventCodes(self.currevt, this_fiducial_only=True)

                if evr_codes is not None:
                    for c in self._evr_cfgcodes:
                        if c in evr_codes:
                            default['evr/code_%d' % c] = 1
                        else:
                            default['evr/code_%d' % c] = 0
                
        self.event(default)

        return


    def sum(self, value):
        """
        Sum `value` across ranks
        """
        return self._mpi_reduce(value, MPI.SUM)


    def max(self, value):
        """
        Find the maximum of `value` across ranks
        """
        return self._mpi_reduce(value, MPI.MAX)


    def min(self, value):
        """
        Find the minimum of `value` across ranks
        """
        return self._mpi_reduce(value, MPI.MIN)


    def _mpi_reduce(self, value, function):

        if value is None:
            arrInfo= None
        else:
            t = num_or_array(value)
            # in principle could avoid amin/amax calls here since only
            # really needed for min/max reductions
            if t == 'num':
                arrInfo = [0,type(value),value,value]
            else:
                arrInfo = [value.shape,value.dtype,np.amin(value),np.amax(value)]
        arrInfoAll=comm.allgather(arrInfo)
        summaryArrInfo = None
        for arrInfo in arrInfoAll:
            if arrInfo is None: continue
            if summaryArrInfo == None: summaryArrInfo = arrInfo
            if arrInfo[:2] != summaryArrInfo[:2]: # check shape/dtype compatible
                raise Exception('Unable to reduce incompatible shapes/types:',arrInfo,summaryArrInfo)
            # find global min/max (only necessary for min/max reduction)
            if arrInfo[2]<summaryArrInfo[2]: summaryArrInfo[2]=arrInfo[2]
            if arrInfo[3]>summaryArrInfo[3]: summaryArrInfo[3]=arrInfo[3]
        if summaryArrInfo is None:
            raise Exception('No data found for MPI reduce')
        (shape,dtype,amin,amax) = summaryArrInfo

        # if our rank has no data manufacture some that won't affect the result
        if value is None: # we have no data on our rank
            if shape==0: # indicates a number
                if function is MPI.SUM: value = 0
                elif function is MPI.MIN: value = amax
                elif function is MPI.MAX: value = amin
            else: # an array
                if function is MPI.SUM:
                    value = np.zeros(shape,dtype=dtype)
                elif function is MPI.MIN:
                    value = np.empty(shape,dtype=dtype)
                    value.fill(amax) # use global max so we don't affect min calculation
                elif function is MPI.MAX:
                    value = np.empty(shape,dtype=dtype)
                    value.fill(amin) # use global min so we don't affect max calcul

        if shape==0: # non-array type (float/int)
            s = comm.reduce(value, function)
        else:
            s = np.empty_like(value) # recvbuf
            comm.Reduce(value, s, function)
        return s


class SmallFile(object):
    """
    An interface to an HDF5 file for saving data using SmallData and MPIDataSource.
    """

    def __init__(self, filename, keys_to_save=[], filters=None):
        """
        Parameters
        ----------
        filename : str
            The full path filename to save to.

        keys_to_save : list of strings
            A list of event data keys to save to disk. For example, if you
            add only ['a'] to this list, but call SmallData.event(a=x, b=y),
            only the values for 'a' will be saved. Hierarchical data structures
            using dictionaries can be referenced using '/' for each level, e.g.
            event({'c' : {'d' : z}}) --> keys_to_save=['c/d'].

        filters: tables.Filters
            Filter properties for the smalldata h5 file. Mainly
            used to configure compression settings.
        """
        self.file_handle = tables.File(filename, 'w', filters=filters)
        self.keys_to_save = keys_to_save
        return


    def _get_node(self, k, dlist_master):
        """
        Retrieve or create (if necessary) the pytables node
        for a specific key.
        """

        try:
            node = self.file_handle.get_node('/'+k)

        except tables.NoSuchNodeError as e: # --> create node

            ex = dlist_master[k][0]

            if num_or_array(ex) == 'array':
                a = tables.Atom.from_dtype(ex.dtype)
                shp = tuple([0] + list(ex.shape))
            elif num_or_array(ex) == 'num':
                a = tables.Atom.from_dtype(np.array(ex).dtype)
                shp = (0,)

            path, _, name = k.rpartition('/')

            if is_ragged_key(name):
                node = self.file_handle.create_vlarray(where='/'+path, name=name,
                                                       atom=a,
                                                       createparents=True)
            else:
                node = self.file_handle.create_earray(where='/'+path, name=name,
                                                      shape=shp, atom=a,
                                                      createparents=True)

        return node


    @property
    def nevents_on_disk(self):
        try:
            self.file_handle.get_node('/', 'fiducials')
            return len(self.file_handle.root.fiducials)
        except tables.NoSuchNodeError:
            return 0


    def save(self, *args, **kwargs):
        """
        Save summary data to an HDF5 file (e.g. at the end of an event loop).

            1. Add data using key-value pairs (similar to SmallData.event())
            2. Add data organized in hierarchy using nested dictionaries (similar to 
               SmallData.event())

        These data are then saved to the file specifed in the SmallData
        constructor.

        Parameters
        ----------
        *args : dictionaries
            In direct analogy to the SmallData.event call, you can also pass
            HDF5 group hierarchies using nested dictionaries. Each level
            of the dictionary is a level in the HDF5 group heirarchy.

        **kwargs : datasetname, dataset
            Similar to SmallData.event, it is possible to save arbitrary
            singleton data (e.g. at the end of a run, to save an average over
            some quanitity).

        Examples
        --------
        >>> # save "average_over_run"
        >>> smldata.save(average_over_run=x)
        
        >>> # save "/base/next_group/data"
        >>> smldata.save({'base': {'next_group' : data}})
        """

        to_save = {}
        to_save.update(kwargs) # deals with save(cspad_sum=array_containing_sum)

        # deals with save({'base': {'next_group' : data}}) case
        dictionaries_to_save  = [d for d in args if type(d)==dict]
        for d in dictionaries_to_save:
            to_save.update(_flatten_dictionary(d))


        # save "accumulated" data (e.g. averages)
        for k,v in to_save.items():

            if type(v) is not np.ndarray:
                v = np.array([v])

            path, _, name = k.rpartition('/')

            node = self.file_handle.create_carray(where='/'+path, name=name,
                                                  obj=v,
                                                  createparents=True)

        return



    def save_event_data(self, dlist_master):
        """
        Save (append) all event data in memory to disk.

        NOTE: This function gets called automatically on every gather
              when using a SmallData object.

        Parameters
        ----------
        dlist_master : dict
            The SmallData object's dlist_master, which is a dictionary
            where the keys are the event data keys, the values arrays
            of the event data.
        """

        if self.file_handle is None:
            # we could accept a 'filename' argument here in the save method
            raise IOError('no filename specified in SmallData constructor')

        # if the user has specified which keys to save, just
        # save those; else, save all event data
        if len(self.keys_to_save) > 0:
            keys_to_save = ['event_time', 'fiducials']
            for k in self.keys_to_save:
                if k in list(dlist_master.keys()):
                    keys_to_save.append(k)
                else:
                    warnings.warn('event data key %s has no '
                                  'associated event data and will not '
                                  'be saved' % k)
        else:
            keys_to_save = list(dlist_master.keys())

        # for each item to save, write to disk
        for k in keys_to_save:
            if len(dlist_master[k]) > 0:
                node = self._get_node(k, dlist_master)
                if type(node) == tables.vlarray.VLArray:
                    for row in dlist_master[k]:
                        node.append(row)
                else:
                    if not (is_var_key(k) and not is_len_key(k)):
                        if not all(arr.shape==node.shape[1:] for arr in dlist_master[k]):
                            raise ValueError('Found ragged array named "%s". ' 
                                             'Prepend HDF5 dataset name with '
                                             '"ragged_" to avoid this error.' % k)
                    node.append( dlist_master[k] )
            else:
                pass

        return


    def close(self):
        """
        Close the HDF5 file used for writing.
        """
        self.file_handle.close()
        return

