
"""
This code is a 'replacement' for psana.Datasource that
maintains the same interface but enables under-the-hood MPI
"""


from datasource import DataSource
from det_interface import DetNames

from mpi4py import MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()


class Step(object):

    def __init__(self, psana_step, ds_parent):
        self._psana_step = psana_step
        self._ds_parent = ds_parent
        return

    def events(self):
        return self._ds_parent._event_gen(self._psana_step)

    # TODO ? add env method


class MPIDataSource(object):

    def __init__(self, ds_string):

        self.ds_string = ds_string
        self.__cpp_ds = DataSource(ds_string)

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

                nevent = 0
                while True:

                    evt = psana_level.events().next()

                    # logic for regular gathers
                    if (self.gather_interval is not None) and \
                       (nevent > 0)                       and \
                       (nevent % self.gather_interval==0):
                        self.sd._gather()

                    if nevent % size == rank:
                        self._currevt = evt
                        yield evt

                    nevent += 1

            # logic for final gather (after all events seen)
            except StopIteration as e:
                self.sd._gather()
                raise StopIteration(e)

        else:
            raise RuntimeError('%s not valid ds_type' % ds_type)


    def env(self):
        return self.__cpp_ds.env()

    def steps(self):
        for step in self.__cpp_ds.steps():
            yield Step(step, self)

    def runs(self): pass

    def detnames(self, which='detectors'):
        # this could prob be better
        DetNames(which, local_env=self.__cpp_ds.env())


    def small_data(self, filename=None, 
                   save_on_gather=False, gather_interval=None):

        # defer the import because cctbx gets unhappy with
        # a floating-point-exception from the pytables import
        from smalldata import SmallData

        # the SmallData and DataSource objects are coupled:
        # -- SmallData must know about the _currevt to "timestamp" its data
        # -- DataSource needs to call SmallData's _gather method

        self.gather_interval = gather_interval
        self.sd = SmallData(self, filename=filename, 
                            save_on_gather=save_on_gather)

        return self.sd


    @property
    def master(self):
        return (rank == 0)




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
