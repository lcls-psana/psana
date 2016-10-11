

"""

-- Note 9/29 

* we believe we can handle cases 1-3 below
* currently there is still an issue of a "ragged" exit
  (e.g. for 2 cores, with gather interval 1 and break after 1 global evt)
  > we believe the way to fix this is to implement a datasource.break() method
  > not idea
* still need to implement fix for case 4


missing data ideas:

- do an allgather of the keys/types/shapes into "send_list"

- client-side issues:
- (case 1) when we append data to _dlist we will backfill numbers before adding the new data, roughly done.
- (case 2) when we gather we check for keys that we didn't see at all OR keys that are "ragged" and backfill.
  did some stuff here, but needs work.
- (case 3) no events at all on a client
- need to construct the missing data for each of the above cases

- master-side issues:
- (case 4) master also needs to backfill on disk and memory

---------------------------------------------------------------------------------------------

> user forgets to call save, they get a small but empty HDF5
-- atexit
-- use a destructor: __del__ (prone to circular ref issues)
-- use "with" statement + __exit__ (may affect interface)

> nightly test

> Saving variable length event data:
-- Automatic detection of vlen at sh5.save()
-- Explicit declare of vlen at sh5.flush()
-- We will try out best to autodetect vlen, and raise if we
   get surprised

> metadata
-- xarray compatability?

> always write certain detectors?

> chunking for performance?

>>> from Silke
- put datasets in user-definable groups [on the list]
- storing extra "attributes" or datasets with stuff like ROI (like summary/config field)
- think about cube problem
- summary data (e.g. cube) and event data could be in different files.  maybe we provide the option
  for multiple smallh5 files [done]
- user-controlled mode for event distribution (e.g. based on delay-time)
- detectors come and go from run to run and not crash
- always write detectors like phasecav
- put in Nan's for missing data (requires everything has to be a float)

>>> from Jason
- coordinates/attributes for everything, using h5netcdf
- for analyzing the small-data: xarray makes it easy to select/filter,
  and will keep track of coordinates for you
- can use pandas for plotting and packages like seaborn (stat plotting)
  with these coordinate arrays
- the xarray coordinates will provide the time sorting
- wildcard to merge multiple files
- didn't support variable length data? (maybe can do this)
- treats NaNs correctly
- merge fast and slow detectors (eventcode 40 and 42)
- handles dropped data
- not clear that we can write pieces while taking data?  look at it.
- probably supports hierarchy of hdf5 groups in h5netcdf, but might
  make it more difficult to cut/merge/sort in xarray
"""

import numpy as np
import tables
import collections

from _psana import EventId

from mpi4py import MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

MISSING_INT = -99999
MISSING_FLOAT = np.nan


def _flatten_dictionary(d, parent_key='', sep='/'):
    """
    http://stackoverflow.com/questions/6027558/flatten-nested-python-dictionaries-compressing-keys
    """
    items = []
    for k, v in d.items():
        new_key = parent_key + sep + k if parent_key else k
        if isinstance(v, collections.MutableMapping):
            items.extend(_flatten_dictionary(v, new_key, sep=sep).items())
        else:
            items.append((new_key, v))
    return dict(items)


def remove_values(the_list, val):
   return [value for value in the_list if value != val]


class SynchDict(dict):
    def synchronize(self):
        tot_send_list = comm.allgather(self)
        for node_send_list in tot_send_list:
            for k in node_send_list:
                if k not in self.keys():
                    self[k] = node_send_list[k]

    def keys(self):
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

    def __init__(self, datasource_parent, filename=None, save_on_gather=False):
        """

        Parameters
        ----------
        gather_interval : int
            The number of events a single process must see before the results
            get gathered for processing. If `None`, then only gather results
            when `save` is called.

        save_on_gather : bool
            Whether to save the results to disk periodically (after the
            results get gathered)
        """

        self._datasource_parent = datasource_parent
        self.save_on_gather     = save_on_gather
        self._num_send_list     = SynchDict()
        self._arr_send_list     = SynchDict()

        self._dlist = {}
        if self.master:
            self._dlist_master = {}
            self._newkeys = []

        if filename and self.master:
            self.file_handle = tables.File(filename, 'w')

        return



    @property
    def master(self):
        return (rank == 0)


    @staticmethod
    def _num_or_array(obj):

        data_type = type(obj)
        if ((data_type in [int, float]) or
             np.issubdtype(data_type, np.integer) or
             np.issubdtype(data_type, np.float)):
            s = 'num'

        elif data_type is np.ndarray:
            s = 'array'

        else:
            raise TypeError('object is not number or array')

        return s


    def _sort(self, obj, sort_order):
        """
        sort `obj`, an array or list, by `sort_order` in dim -1,
        IN PLACE
        """
        
        assert type(sort_order) == np.ndarray, 'sort_order must be array'
        t = self._num_or_array(obj[0])

        if t is 'num':
            res = obj[sort_order]
        elif t is 'array':
            res = [x for (y,x) in sorted( zip(sort_order, obj) ) ]

        return res


    def missing(self, key):



        if key in self._num_send_list.keys():
            t = self._num_send_list[key]

            if t in [int, np.int8, np.int16, np.int32, np.int64, np.int]:
                missing_value = MISSING_INT
            elif t in [float, np.float16, np.float32, np.float64, np.float128, np.float]:
                missing_value = MISSING_FLOAT
            else:
                raise ValueError('%s :: Invalid num type for missing data' % str(t))


        elif key in self._arr_send_list.keys():

            t     = self._arr_send_list[key][0]
            shape = self._arr_send_list[key][1]
            dtype = self._arr_send_list[key][2]

            missing_value = np.empty(shape, dtype=t)

            if dtype in [int, np.int8, np.int16, np.int32, np.int64, np.int]:
                missing_value.fill(MISSING_INT)
            elif dtype in [float, np.float16, np.float32, np.float64, np.float128, np.float]:
                missing_value.fill(MISSING_FLOAT)
            else:
                raise ValueError('%s :: Invalid array type for missing data' % str(dtype))

        else:
            raise KeyError('key %s not found in array or number send_list' % key)

        return missing_value


    def _gather(self):

        # "send lists" hold a catalogue of the data keys to expect
        # aggregated across all ranks
        self._arr_send_list.synchronize()
        self._num_send_list.synchronize()

        # for all data in our aggregated catalogue, gather
        for k in self._arr_send_list.keys():
            if k not in self._dlist.keys(): self._dlist[k] = []
            self._backfill_client(self._nevents, self._dlist[k], self.missing(k))
            self._gather_arrays(self._dlist[k], k)

        for k in self._num_send_list.keys():
            if k not in self._dlist.keys(): self._dlist[k] = []
            self._backfill_client(self._nevents, self._dlist[k], self.missing(k))
            self._gather_numbers(self._dlist[k], k)

        self._dlist = {}  # forget all the data we just sent

        # if we are the master:
        #   (1) sort data by time
        #   (2) backfill missing data [to beginning of time, memory + disk]
        #       this must be after the sort, because there are no timestamps
        #       to sort backfilled data
        #   (3) save if requested

        # note that _dlist_master is different than the client _dlist's.  it is a list of lists,
        # one list for each gather that has happened since the last save.  the style of
        # the contents of _dlist_master is also changed by the "sort" call: individual numbers
        # (not arrays) in a gather are originally arrays, but the sort converts them into lists
        # of numbers.

        if self.master:

            # (1) sort data by time
            for k in self._dlist_master.keys():

                if k is 'event_time':
                    continue
                    
                # "-1" here says we are only sorting the result from the most recent gather
                self._dlist_master[k][-1] = [x for (y,x) in 
                                             sorted( zip(self._dlist_master['event_time'][-1], 
                                                         self._dlist_master[k][-1]) ) ]
            
            self._dlist_master['event_time'][-1] = sorted(self._dlist_master['event_time'][-1])

            # (2) backfill missing data
            for k in self._newkeys:
                
                events_in_mem = sum([len(x) for x in self._dlist_master['fiducials']])
                target_events = self._nevents_on_disk + events_in_mem
                self._dlist_master[k] = self._backfill_master(target_events, 
                                                              self._dlist_master[k], 
                                                              self.missing(k))
            self._newkeys=[]

            # (3) save if requested
            if self.save_on_gather:
                self._save() # save all event data on gather

        return


    def _gather_numbers(self, num_list, key):

        lengths = np.array(comm.gather(len(num_list))) # get list of lengths
        mysend = np.array(num_list,dtype=self._num_send_list[key])
        myrecv = None

        if self.master:
            myrecv = np.empty((sum(lengths)),mysend.dtype) # allocate receive buffer

        comm.Gatherv(sendbuf=mysend, recvbuf=[myrecv, lengths])

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

        # workers flatten arrays and send those to master
        if len(array_list) > 0:
            mysend = np.concatenate([ x.reshape(-1) for x in array_list ])
            mysend = np.ascontiguousarray(mysend)
        else:
            mysend = np.array([])

        # master computes how many array elements to expect, 
        # recvs in linear array

        if self.master:
            worker_lens = [[np.product(shp) for shp in worker] for worker in worker_shps]  # flattened size of each array
            worker_msg_sizes = [np.sum(lens) for lens in worker_lens]                      # size of msg from each rank
            recv_buff_size = np.sum(worker_msg_sizes)
            myrecv = np.empty(recv_buff_size, mysend.dtype)                                # allocate receive buffer
            recvbuf = [myrecv, worker_msg_sizes]
        else:
            recvbuf = None

        comm.Gatherv(sendbuf=mysend, recvbuf=recvbuf)
        
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
        if 'fiducials' in self._dlist:
            return len(self._dlist['fiducials'])
        else:
            return 0


    @property
    def _nevents_on_disk(self):
        try:
            self.file_handle.get_node('/', 'fiducials')
            return len(self.file_handle.root.fiducials)
        except tables.NoSuchNodeError:
            return 0


    @staticmethod
    def _backfill_client(target_events, dlist_element, fill_value):
        numfill = target_events - len(dlist_element)
        if numfill > 0:
            dlist_element.extend([fill_value]*numfill)
        return


    @staticmethod
    def _backfill_master(target_events, dlist_element, fill_value):
        numfill = target_events - sum([len(x) for x in dlist_element])
        if numfill > 0:
            dlist_element = [[fill_value]*numfill] + dlist_element
        return dlist_element


    def _dlist_append_client(self, key, value):

        data_type = type(value)

        if data_type is np.ndarray:
            value = np.atleast_1d(value)
            if key not in self._arr_send_list:
                self._arr_send_list[key] = (data_type, value.shape, value.dtype)

        else:
            if key not in self._num_send_list:
                self._num_send_list[key] = data_type

        if key not in self._dlist.keys():
            self._dlist[key] = []

        # patch up _dlist with missing data before we add new values
        self._backfill_client(self._nevents - 1,
                              self._dlist[key],
                              self.missing(key))

        self._dlist[key].append(value)

        return


    def _dlist_append(self, dlist, key, value):

        if key not in dlist.keys():
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

        if ('event_time' in kwargs.keys()) or ('fiducials' in kwargs.keys()):
            raise KeyError('`event_time` and `fiducials` are special names'
                           ' reserved for timestamping -- choose a different '
                           'name')

        # *args can be used to pass hdf5 heirarchies (groups/names) in a dict
        # flatten these and create a single dictionary of (name : value) pairs
        # when groups will be created in the hdf5, we will use a "/"

        event_data_dict = {}
        event_data_dict.update(kwargs)
        for d in args:
            event_data_dict.update( _flatten_dictionary(d) )


        # get timestamp data for most recently yielded evt
        evt_id = self._datasource_parent._currevt.get(EventId)

        time = evt_id.time()[0] << 32 | evt_id.time()[1] # chris' craziness
        fid  = evt_id.fiducials()

        # --> check to see if we already save the time for this event
        if ('fiducials' in self._dlist.keys()) and (self._dlist['fiducials'][-1] == fid):

            # check to see if we already added this field this event
            events_seen = len(self._dlist['fiducials'])
            for k in event_data_dict.keys():

                # if the list is as long as the # evts seen, 
                # user has tried to add key twice
                if len( self._dlist.get(k, []) ) == events_seen:        # False if key not in dlist
                    raise RuntimeError("Event data already added for "
                                       "key '%s' this event!" % k)

        # --> otherwise this is a new event
        else:
            self._dlist_append_client('event_time', time)
            self._dlist_append_client('fiducials', fid)

        for k in event_data_dict.keys():
            self._dlist_append_client(k, event_data_dict[k])

        return


    def sum(self, value):
        return self._mpi_reduce(value, MPI.SUM)


    def max(self, value):
        return self._mpi_reduce(value, MPI.MAX)


    def min(self, value):
        return self._mpi_reduce(value, MPI.MIN)


    def _mpi_reduce(self, value, function):
        t = self._num_or_array(value)
        if t is 'num':
            s = comm.reduce(value, function)
        elif t is 'array':
            s = np.empty_like(value) # recvbuf
            comm.Reduce(value, s, function)
        return s


    def _get_node(self, k):
        """
        Retrieve or create (if necessary) the pytables node
        for a specific key.
        """

        try:
            #print 'trying to get node: %s' % k
            node = self.file_handle.get_node('/'+k)

        except tables.NoSuchNodeError as e: # --> create node
            ex = self._dlist_master[k][0]
            if self._num_or_array(ex) == 'array':
                a = tables.Atom.from_dtype(ex.dtype)
                shp = tuple([0] + list(ex.shape))
            elif self._num_or_array(ex) == 'num':
                a = tables.Atom.from_dtype(np.array(ex).dtype)
                shp = (0,)

            path, _, name = k.rpartition('/')
            node = self.file_handle.create_earray(where='/'+path, name=name,
                                                  shape=shp, atom=a,
                                                  createparents=True)

        return node


    def save(self, *args, **kwargs):
        """
        Save registered data to an HDF5 file.

        There are 3 behaviors of the arguments to this function:

            1. Decide what 'event data' (declared by SmallData.event())
               should be saved
            2. Add summary (ie. any non-event) data using key-value
               pairs (similar to SmallData.event())
            3. Add summary (ie. any non-event) data organized in a
               heirarchy using nested dictionaries (als similar to 
               SmallData.event())

        These data are then saved to the file specifed in the SmallData
        constructor.

        Parameters
        ----------
        *args : strings
            A list of the event data names to save, if you want to save a
            subset. Otherwise save all data to disk. For example, imagine
            you had called SmallData.event(a=..., b=...). Then smldata.save('b')
            would not save data labelled 'a'. smldata.save() would save both
            'a' and 'b'.

        *args : dictionaries
            In direct analogy to the SmallData.event call, you can also pass
            HDF5 group heirarchies using nested dictionaries. Each level
            of the dictionary is a level in the HDF5 group heirarchy.

        **kwargs : datasetname, dataset
            Similar to SmallData.event, it is possible to save arbitrary
            singleton data (e.g. at the end of a run, to save an average over
            some quanitity).

        event_data : bool
            Special kwarg. If `False`, do not save the event data, just
            the run summary data. If `True` (default), do.

        Examples
        --------
        >>> # save all event data
        >>> smldata.save()

        >>> # save all event data AND "array_containing_sum"
        >>> smldata.save(cspad_sum=array_containing_sum)
        
        >>> # save 'b' event data, and no other
        >>> smldata.save('b')

        >>> # save all event data AND "/base/next_group/data"
        >>> smldata.save({'base': {'next_group' : data}})
        """

        self._gather()

        if self.master:
            self._save(*args, **kwargs)

        return


    def _save(self, *args, **kwargs):

        evt_variables_to_save = [s for s in args if type(s)==str]
        dictionaries_to_save  = [d for d in args if type(d)==dict]
        

        dict_to_save = {}
        dict_to_save.update(kwargs)
        for d in dictionaries_to_save:
            dict_to_save.update(_flatten_dictionary(d))


        if self.file_handle is None:
            # we could accept a 'filename' argument here in the save method
            raise IOError('no filename specified in SmallData constructor')

        # if 'event_data' key is set, save event data (default True)
        if kwargs.pop('event_data', True):
          
            # if the user has specified which keys to save, just
            # save those; else, save all event data
            if len(evt_variables_to_save) > 0:
                keys_to_save = ['event_time', 'fiducials']
                for k in evt_variables_to_save:
                    if k in self._dlist_master.keys():
                        keys_to_save.append(k)
                    else:
                        print('Warning: event data key %s has no '
                              'associated event data and will not '
                              'be saved' % k)
            else:
                keys_to_save = self._dlist_master.keys()

            # for each item to save, write to disk
            for k in keys_to_save:
                if len(self._dlist_master[k]) > 0:

                    # make a list of lists of arrays a single np array
                    #     note "[]" due to gathers with no data (for any rank)
                    self._dlist_master[k] = remove_values(self._dlist_master[k], [])
                    self._dlist_master[k] = np.concatenate(self._dlist_master[k])

                    node = self._get_node(k)
                    node.append( self._dlist_master[k] )
                    self._dlist_master[k] = []
                else:
                    #print 'Warning: no data to save for key %s' % k
                    pass
        

        # save "accumulated" data (e.g. averages)
        for k,v in dict_to_save.items():

            if type(v) is not np.ndarray:
                v = np.array([v])

            path, _, name = k.rpartition('/')
            node = self.file_handle.create_carray(where='/'+path, name=name,
                                                  obj=v,
                                                  createparents=True)

        return
