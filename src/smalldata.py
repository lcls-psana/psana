
"""
> Idea of missing data
  (skip this for alpha v)
-- NaN for missing ints?

> Support users who save e.g. one dset at 60 Hz, one at 120 Hz (keep things aligned)
  (skip this for alpha v)

Saving variable length event data:
  (skip this for alpha v)
-- Automatic detection of vlen at sh5.save()
-- Explicit declare of vlen at sh5.flush()
-- We will try out best to autodetect vlen, and raise if we
   get surprised

> chunking for performance?
   (skip this for alpha v)


>>> before alpha release <<<

> clean up and merge w/DataSource ***
> keep data on master after save()?

"""

import numpy as np
import tables

from _psana import EventId

from mpi4py import MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()


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

        self._dlist = {}
        if self.master:
            self._dlist_master = {}

        if filename and self.master:
            self.file_handle = tables.File(filename, 'w')

        return


    def __del__(self):
        print 'del method'
        if hasattr(self, file_handle):
            self.file_handle.close()
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


    def _gather(self):

        for k in self._dlist.keys():
            t = self._num_or_array(self._dlist[k][0])
            if t is 'num':
                self._gather_numbers(self._dlist[k], k)
            elif t is 'array':
                self._gather_arrays(self._dlist[k], k)

        self._dlist = {}  # forget all the data we just sent

        # if we are the master, sort
        if self.master:
            for k in self._dlist_master.keys():

                if k is 'event_time':
                    continue
                    
                self._dlist_master[k][-1] = [x for (y,x) in 
                                             sorted( zip(self._dlist_master['event_time'][-1], 
                                                         self._dlist_master[k][-1]) ) ]

            self._dlist_master['event_time'][-1] = sorted(self._dlist_master['event_time'][-1])

            if self.save_on_gather:
                self._save() # save all event data on gather

        return


    def _gather_numbers(self, num_list, key):

        lengths = np.array(comm.gather(len(num_list))) # get list of lengths
        mysend = np.array(num_list)
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
        mysend = np.concatenate([ x.reshape(-1) for x in array_list ])
        mysend = np.ascontiguousarray(mysend)

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


    @staticmethod
    def _dlist_append(dlist, key, value):

        if key not in dlist.keys():
            dlist[key] = [value]
        else:
            dlist[key].append(value)

        return


    def event(self, **kwargs):

        if ('event_time' in kwargs.keys()) or ('fiducials' in kwargs.keys()):
            raise KeyError('`event_time` and `fiducials` are special names'
                           ' reserved for timestamping -- choose a different '
                           'name')


        # get timestamp data for most recently yielded evt
        evt_id = self._datasource_parent._currevt.get(EventId)

        time = evt_id.time()[0] << 32 | evt_id.time()[1] # chris' craziness
        fid  = evt_id.fiducials()

        # --> check to see if we already save the time for this event
        if ('fiducials' in self._dlist.keys()) and (self._dlist['fiducials'][-1] == fid):

            # check to see if we already added this field this event
            events_seen = len(self._dlist['fiducials'])
            for k in kwargs.keys():

                # if the list is as long as the # evts seen, 
                # user has tried to add key twice
                if len( self._dlist.get(k, []) ) == events_seen:        # False if key not in dlist
                    raise RuntimeError("Event data already added for "
                                       "key '%s' this event!" % k)

        # --> otherwise this is a new event
        else:
            self._dlist_append(self._dlist, 'event_time', time)
            self._dlist_append(self._dlist, 'fiducials', fid)

        for k in kwargs.keys():
            self._dlist_append(self._dlist, k, kwargs[k])

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

        if k in [x.name for x in self.file_handle.iter_nodes('/')]:
            node = self.file_handle.get_node('/%s' % k)

        else:
            ex = self._dlist_master[k][0]
            if self._num_or_array(ex) == 'array':
                a = tables.Atom.from_dtype(ex.dtype)
                shp = tuple([0] + list(ex.shape))
            elif self._num_or_array(ex) == 'num':
                a = tables.Atom.from_dtype(np.array(ex).dtype)
                shp = (0,)

            node = self.file_handle.create_earray(where='/', name=k,
                                                  shape=shp, atom=a)

        return node


    def save(self, *evt_variables_to_save, **kwargs):

        self._gather()

        if self.master:
            self._save(*evt_variables_to_save, **kwargs)

        return


    def _save(self, *evt_variables_to_save, **kwargs):

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
                    self._dlist_master[k] = np.concatenate(self._dlist_master[k])
                    node = self._get_node(k)
                    node.append( self._dlist_master[k] )
                    self._dlist_master[k] = []
                else:
                    #print 'Warning: no data to save for key %s' % k
                    pass
        

        # save "accumulated" data (e.g. averages)
        for k,v in kwargs.items():
            v = np.array(v)
            a = tables.Atom.from_dtype(v.dtype)
            node = self.file_handle.create_carray(where='/', name=k,
                                                  shape=v.shape, atom=a)

        return



