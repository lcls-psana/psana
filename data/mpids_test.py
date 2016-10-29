
"""
Note: expects to be called with 1 or 4 cores, though any number should work...
"""

import os
import abc
import h5py
import psana
import numpy as np
from math import ceil

from psana import smalldata as smalldata_mod


from mpi4py import MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()


class TestSmallData(object):

    def setup(self):

        self.filename = self.__class__.__name__ + '.h5'
        self.gather_interval = 2

        self.dsource = psana.MPIDataSource('exp=xpptut15:run=54:smd')
        self.smldata = self.dsource.small_data(self.filename, 
                                               gather_interval=5)

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


    def d_element(self, drank, nevt):
        """
        builder function for array d, which is a staggered example
        where each core behaves differently
        """

        if drank == 0:
            ret = self.dataset()

        elif drank == 1:
            if nevt + 1 > self.gather_after:
                ret = self.dataset()
            else:
                ret = None

        elif drank == 2:
            if nevt > self.gather_after:
                ret = self.dataset()
            else:
                ret = None

        else:
            ret = None

        return ret


    def generate_h5(self): 

        for nevt,evt in enumerate(self.dsource.events()):

            # dataset a: always there
            tmp = self.dataset()
            self.smldata.event(a=tmp)

            # dataset b: missing data, but some data before gather
            if nevt + 1 > self.gather_after:
                self.smldata.event(b=self.dataset())


            # dataset c: missing data, not seen until after gather
            if nevt > self.gather_after:
                self.smldata.event(c=self.dataset())


            # dataset d: test all cores different
            if size > 3:
                dret = self.d_element(rank, nevt)
                if dret is not None:
                    self.smldata.event(d=dret)


            # gather after indicated events
            if nevt == self.gather_after:
                self.smldata._gather()


            # break after indicated events
            if nevt == self.end_after:

                print rank, nevt, self.smldata._nevents
                break

        self.smldata.save()
        print 'psave', rank, nevt, self.smldata._nevents
        if self.smldata.master:
            self.smldata.file_handle.close()

        return


    def validate_h5(self):

        if rank == 0:


            expected_a = [self.dataset()] * (self.end_after + 1) * size
            expected_b = [self.missing()] * self.gather_after * size + \
                         [self.dataset()] * (self.end_after + 1 - self.gather_after) * size
            expected_c = [self.missing()] * (self.gather_after + 1) * size + \
                         [self.dataset()] * (self.end_after - self.gather_after) * size

            expected_d = []
            for nevt in range(self.end_after + 1):
                for drank in range(size):
                    de = self.d_element(drank, nevt)
                    if de is not None:
                        expected_d.append( self.d_element(drank, nevt) )
                    else:
                        expected_d.append( self.missing() )

            f = h5py.File(self.filename)
            np.testing.assert_allclose( np.array(f['a']),
                                        np.array(expected_a),
                                        err_msg='mismatch in a' )
            np.testing.assert_allclose( np.array(f['b']),
                                        np.array(expected_b),
                                        err_msg='mismatch in b' )
            np.testing.assert_allclose( np.array(f['c']), 
                                        np.array(expected_c),
                                        err_msg='mismatch in c' )

            if 'd' in f.keys():
                np.testing.assert_allclose( np.array(f['d']),
                                            np.array(expected_d),
                                            err_msg='mismatch in d' )

            f.close()

        return


    def teardown(self):
        if rank == 0:
            os.remove(self.filename)
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
        return np.ones((1,2,3)).astype(np.int)
    def missing(self):
        return smalldata_mod.MISSING_INT * np.ones(self.dataset().shape, dtype=np.int)

class TestFloatArray(TestSmallData):
    def dataset(self):
        return np.ones((1,2,3)).astype(np.float)
    def missing(self):
        return smalldata_mod.MISSING_FLOAT * np.zeros(self.dataset().shape)


if __name__ == '__main__':

    try:
        for Test in [TestInt, TestFloat, TestIntArray, TestFloatArray]:
            t = Test()
            t.setup()
            if rank == 0: print 'Testing: %s' % t.filename
            t.test_h5gen()
            t.teardown()
    except AssertionError as e:
        print e
        comm.Abort(1)
