
"""
Note: expects to be called with 1 or 4 cores, though any number should work...
"""

import abc
import h5py
import psana
import numpy as np

from psana import smalldata as smalldata_mod


from mpi4py import MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()


class TestSmallData(object):

    def setup(self):

        self.filename = self.__class__.__name__ + '.h5'
        self.gather_interval = 5

        self.dsource = psana.MPIDataSource('exp=xpptut15:run=54:smd')
        self.smldata = self.dsource.small_data(self.filename, 
                                               gather_interval=self.gather_interval)

        self.gather_after = 3 # events (for each core)
        self.end_after    = 5 # events (for each core)

        assert self.gather_after > 2, 'gather after should be >= 3'
        assert self.gather_after < self.end_after

        return


    @abc.abstractmethod
    def dataset(self):
        return

    @abc.abstractmethod
    def missing(self):
        return

    def generate_h5(self): 

        for nevt,evt in enumerate(self.dsource.events()):

            # dataset a: always there
            self.smldata.event(a=self.dataset())


            # dataset b: missing data, but some data before gather
            if nevt + 1 > self.gather_after:
                self.smldata.event(b=self.dataset())


            # dataset c: missing data, not seen until after gather
            if nevt > self.gather_after:
                self.smldata.event(c=self.dataset())


            # dataset d: test all cores different
            if size > 3:
                if rank == 0:
                    self.smldata.event(d=self.dataset())
                elif rank == 1:
                    if nevt + 1 > self.gather_after:
                        self.smldata.event(d=self.dataset())
                elif rank == 2:
                    if nevt > self.gather_after:
                        self.smldata.event(d=self.dataset())
                else:
                    pass


            # gather after indicated events
            if nevt == self.gather_after:
                self.smldata._gather()


            # break after indicated events
            if nevt == self.end_after:
                break

        self.smldata.save()
        if self.smldata.master:
            self.smldata.file_handle.close()

        return

    def validate_h5(self):

        if rank == 0:

            expected_a = [self.dataset()] * (self.end_after + 1) * size
            expected_b = [self.missing()] * self.gather_after * size + \
                         [self.dataset()] * (self.end_after + 1 - self.gather_after) * size
            expected_c = [self.missing()] * (self.gather_after + 1) * size+ \
                         [self.dataset()] * (self.end_after - self.gather_after) * size
            expected_d = None # todo


            f = h5py.File(self.filename)
            np.testing.assert_allclose( np.array(f['a']),
                                        np.array(expected_a) )
            np.testing.assert_allclose( np.array(f['b']),
                                        np.array(expected_b) )
            np.testing.assert_allclose( np.array(f['c']), 
                                        np.array(expected_c) )
            f.close()

        return

    def test_h5gen(self):
        self.generate_h5()
        self.validate_h5()
        return


class TestInt(TestSmallData):
    def dataset(self):
        return 1
    def missing(self):
        return smalldata_mod.MISSING_INT

class TestFloat(TestSmallData):
    def dataset(self):
        return 1.0
    def missing(self):
        return smalldata_mod.MISSING_FLOAT

class TestIntArray(TestSmallData):
    def dataset(self):
        return np.ones((1,2,3))
    def missing(self):
        return smalldata_mod.MISSING_INT * np.ones(self.dataset().shape)

class TestFloatArray(TestSmallData):
    def dataset(self):
        return np.ones((1,2,3)).astype(np.float)
    def missing(self):
        return smalldata_mod.MISSING_FLOAT * np.zeros(self.dataset().shape)


if __name__ == '__main__':

    for Test in [TestInt, TestFloat]:# , TestIntArray, TestFloatArray]:
        t = Test()
        t.setup()
        print t.filename
        t.test_h5gen()


