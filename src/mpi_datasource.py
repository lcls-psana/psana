
from datasource import DataSource
from det_interface import DetNames


class Step(object):
    """
    An object that represents a set of events within
    a run taken under identical conditions (also known
    as a `calib-cycle`.
    """
    def __init__(self, psana_step, ds_parent):
        self._psana_step = psana_step
        self._ds_parent = ds_parent
        return

    def events(self):
        """
        Returns a python generator of events.

        Parameters
        ----------
        None
        """
        return self._ds_parent._event_gen(self._psana_step)

    # TODO ? add env method


class MPIDataSource(object):

    """
    A wrapper for psana.Datasource that
    maintains the same interface but hides distribution of
    events to many MPI cores to simplify user analysis code.
    """

    def __init__(self, ds_string, **kwargs):

        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        self.rank = comm.Get_rank()
        self.size = comm.Get_size()

        self.ds_string = ds_string
        self.__cpp_ds = DataSource(ds_string, **kwargs)

        if ':idx' in self.ds_string:
            self._ds_type = 'idx'
            raise RuntimeError('idx mode not supported')
        elif ':smd' in self.ds_string:
            self._ds_type = 'smd'
        elif 'shmem' in self.ds_string:
            self._ds_type = 'shmem'
            raise NotImplementedError('shmem not supported')
        else:
            self._ds_type = 'std'

        self._currevt = None # the current event

        return


    def events(self):
        """
        Returns a python generator of events.

        Parameters
        ----------
        None
        """
        return self._event_gen(self.__cpp_ds)


    def _event_gen(self, psana_level):
        """
        psana_level is a DataSource, Run, Step object with a .events() method
        """

        if self._ds_type in ['std', 'shmem']:
            for evt in psana_level.events():
                self._currevt = evt
                yield evt

        elif self._ds_type == 'smd':

            # this code keeps track of the global (total) number of events
            # seen, and contains logic for when to call MPI gather

            # the try/except loop enforces the fact that we wish to gather
            # after all events (in eg Step/Run) have been processed -- we watch
            # for the StopIteration exception indicating the final event, then
            # catch it, gather, then re-raise it

            try:

                nevent = -1
                while True:

                    nevent += 1

                    evt = psana_level.events().next()

                    if (self.global_gather_interval is not None) and \
                       (nevent > 1)                       and \
                       (nevent % self.global_gather_interval==0):
                        self.sd._gather()

                    # logic for regular gathers
                    if nevent % self.size == self.rank:
                        self._currevt = evt
                        yield evt


            # logic for final gather (after all events seen)
            except StopIteration as e:
                print '&&& final gather',self.rank
                self.sd._gather()
                raise StopIteration(e)

        else:
            raise RuntimeError('%s not valid ds_type' % ds_type)


    def env(self):
        return self.__cpp_ds.env()


    def steps(self):
        for step in self.__cpp_ds.steps():
            yield Step(step, self)


    def runs(self):
        raise NotImplementedError()


    def detnames(self, which='detectors'):
        # this could prob be better
        return DetNames(which, local_env=self.__cpp_ds.env())


    def small_data(self, filename=None, 
                   save_on_gather=False, gather_interval=100):
        """
        Returns an object that manages small per-event data as
        well as non-event data (e.g. a sum of an image over a run)

        Parameters
        ----------
        filename : string, optional
            A filename to use for saving the small data

        save_on_gather: bool, optional (default False)
            If true, save data to HDF5 file everytime
            results are gathered from all MPI cores

        gather_interval: unsigned int, optional (default 100)
            If set to unsigned integer "N", gather results
            from all MPI cores every "N" events.  Events are
            counted separately on each core.  If not set,
            only gather results from all cores at end-run.
        """

        # defer the import because cctbx gets unhappy with
        # a floating-point-exception from the pytables import
        from smalldata import SmallData

        # the SmallData and DataSource objects are coupled:
        # -- SmallData must know about the _currevt to "timestamp" its data
        # -- DataSource needs to call SmallData's _gather method

        self.global_gather_interval = gather_interval*self.size
        self.sd = SmallData(self, filename=filename, 
                            save_on_gather=save_on_gather)

        return self.sd


    @property
    def master(self):
        return (self.rank == 0)




if __name__ == '__main__':
    import psana
    ds = DataSource('exp=xpptut15:run=210')
    for ie, evt in enumerate(ds.events()):
        print ie
        print evt.get(psana.EventId)
        if ie>5: break

    ds = DataSource('exp=xpptut15:run=210')
    for ix, step in enumerate(ds.steps()):
      for ie, evt in enumerate(step.events()):
        print ix,ie
        print evt.get(psana.EventId)
        if ie>5: break
