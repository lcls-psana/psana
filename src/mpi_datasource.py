from __future__ import print_function
from __future__ import absolute_import

from .datasource import DataSource
from .det_interface import DetNames


class Step(object):
    """
    An object that represents a set of events within
    a run taken under identical conditions (also known
    as a `calib-cycle`.
    """
    def __init__(self, psana_step, ds_parent):
        """
        Create an MPIDataSource compatible Step object.

        Parameters
        ----------
        psana_step : psana.Step
            An instance of psana.Step

        ds_parent : psana.DataSoure
            The DataSource object that created the Step
        """
        self._psana_step = psana_step
        self._ds_parent = ds_parent
        return


    def events(self):
        """
        Returns a python generator of events.
        """
        return self._ds_parent._event_gen(self._psana_step)


    def env(self):
        return self._psana_step.env()


class MPIDataSource(object):
    """
    A wrapper for psana.Datasource that
    maintains the same interface but hides distribution of
    events to many MPI cores to simplify user analysis code.
    """

    def __init__(self, ds_string, **kwargs):
        """
        Create a wrapper for psana.Datasource that
        maintains the same interface but hides distribution of
        events to many MPI cores to simplify user analysis code.

        Parameters
        ----------
        ds_string : str
            A DataSource string, e.g. "exp=xpptut15:run=54:smd" that
            specifies the experiment and run to access.

        Example
        -------
        >>> ds = psana.MPIDataSource('exp=xpptut15:run=54:smd')
        >>> smldata = ds.small_data('my.h5')
        >>> cspad = psana.Detector('cspad')
        >>> for evt in ds.events():
        >>>     mu = np.mean( cspad.calib(evt) )
        >>>     smldata.append(cspad_mean=mu)

        See Also
        --------
        psana.DataSource
            The serial data access method this class is based on

        MPIDataSource.small_data
            Method to create a SmallData object that can aggregate
            data in a parallel fashion.
        """


        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        self.rank = comm.Get_rank()
        self.size = comm.Get_size()
        self.comm = comm

        if not ':smd' in ds_string:
            ds_string += ':smd'
        self.ds_string = ds_string

        if ':idx' in self.ds_string:
            self._ds_type = 'idx'
            raise RuntimeError('idx mode not supported')
        elif 'shmem' in self.ds_string:
            self._ds_type = 'shmem'
            raise NotImplementedError('shmem not supported')
        elif ':smd' in self.ds_string:
            self._ds_type = 'smd'
        else:
            raise RuntimeError('did not find smd mode on')
            #self._ds_type = 'std'

        self.global_gather_interval = None
        self.__cpp_ds = DataSource(ds_string, **kwargs)

        self._currevt     = None   # the current event
        self._break_after = 2**62  # max num events

        self.nevent = -1

        return


    def _currrun(self):
        """
        Get the current run number
        """
        if hasattr(self._currevt, 'run'):
            return self._currevt.run()
        else:
            return None


    @property
    def experiment(self):
        return self.env().experiment()


    def events(self):
        """
        Returns a python generator of events.
        """
        return self._event_gen(self.__cpp_ds)


    def _event_gen(self, psana_level):
        """
        psana_level is a DataSource, Run, Step object with a .events() method
        """

        # this code keeps track of the global (total) number of events
        # seen, and contains logic for when to call MPI gather

        self.nevent = -1
        while self.nevent < self._break_after-1:

            self.nevent += 1

            try:
                evt = next(psana_level.events())
            except StopIteration:
                return

            # logic for regular gathers
            if (self.global_gather_interval is not None) and \
               (self.nevent > 1)                              and \
               (self.nevent % self.global_gather_interval==0):
                self.sd.gather()

            if self.nevent % self.size == self.rank:
                self._currevt = evt
                yield evt

        if hasattr(self, 'sd'):
            self.sd.gather()

        return


    def event_number(self):
        return self.nevent


    def env(self):
        return self.__cpp_ds.env()


    def steps(self):
        for step in self.__cpp_ds.steps():
            yield Step(step, self)


    def runs(self):
        raise NotImplementedError()


    def break_after(self, n_events):
        """
        Limit the datasource to `n_events` (max global events).

        Unfortunately, you CANNOT break safely out of an event iteration
        loop when running in parallel. Sometimes, one core will break, but
        his buddies will keep chugging. Then they get stuck waiting for him
        to catch up, with no idea that he's given up!

        Instead, use this function to stop iteration safely.

        Parameters
        ----------
        n_events : int
            The GLOBAL number of events to include in the datasource
            (ie. break out of an event loop after this number of 
            events have been processed)
        """
        self._break_after = n_events
        return


    def detnames(self, which='detectors'):
        """
        List the detectors contained in this datasource.

        Parameters
        ----------
        which : str
            One of: "detectors", "epics", "all".

        Returns
        -------
        detnames : str
            A list of detector names and aliases
        """
        return DetNames(which, local_env=self.__cpp_ds.env())


    def small_data(self, filename=None, keys_to_save=[],
                   gather_interval=100, filters=None):
        """
        Returns an object that manages small per-event data as
        well as non-event data (e.g. a sum of an image over a run)

        Parameters
        ----------
        filename : string, optional
            A filename to use for saving the small data.

        keys_to_save : list of strings
            Optionally filter what data gets saved to disk by
            providing a list of key names here. Only event
            data matching those keys names will be saved.

        gather_interval: unsigned int, optional (default 100)
            If set to unsigned integer "N", gather results
            from all MPI cores every "N" events.  Events are
            counted separately on each core.  If not set,
            only gather results from all cores at end-run.

        filters: tables.Filters,, optional (default None)
            Filter properties for the smalldata h5 file. Mainly
            used to configure compression settings.
            See https://www.pytables.org/usersguide/libref/helper_classes.html#the-filters-class

        Example
        -------
        >>> ds = psana.MPIDataSource('exp=xpptut15:run=54:smd')
        >>> smldata = ds.small_data('my.h5')
        >>> cspad = psana.Detector('cspad')
        >>> for evt in ds.events():
        >>>     mu = np.mean( cspad.calib(evt) )
        >>>     smldata.append(cspad_mean=mu)
        """

        # defer the import because cctbx gets unhappy with
        # a floating-point-exception from the pytables import
        from .smalldata import SmallData

        # the SmallData and DataSource objects are coupled:
        # -- SmallData must know about the _currevt to "timestamp" its data
        # -- DataSource needs to call SmallData's gather method

        self.global_gather_interval = gather_interval*self.size
        self.sd = SmallData(self, filename=filename, 
                            keys_to_save=keys_to_save,
                            filters=filters)

        return self.sd


    @property
    def master(self):
        return (self.rank == 0)




if __name__ == '__main__':
    import psana
    ds = DataSource('exp=xpptut15:run=210')
    for ie, evt in enumerate(ds.events()):
        print(ie)
        print(evt.get(psana.EventId))
        if ie>5: break

    ds = DataSource('exp=xpptut15:run=210')
    for ix, step in enumerate(ds.steps()):
      for ie, evt in enumerate(step.events()):
        print(ix,ie)
        print(evt.get(psana.EventId))
        if ie>5: break
