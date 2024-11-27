
"""
Note: expects to be called with 1 or 4 cores, though any number should work...
"""
from __future__ import print_function

import os
import abc
import h5py
import psana
import numpy as np
from uuid import uuid4
import sys

from psana import smalldata as smalldata_mod


from mpi4py import MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()


class TestSmallData(object):

    def setup(self):

        self.filename = '/tmp/' + str(uuid4()) + '.h5'
        self.gather_interval = 2

        dstr = 'exp=sxrk4816:run=66:smd:dir=/reg/g/psdm/data_test/multifile/test_028_sxrk4816'
        self.dsource = psana.MPIDataSource(dstr)
        self.smldata = self.dsource.small_data(self.filename,
                                               gather_interval=5)

        self.gather_after = 3 # events (for each core)
        self.end_after    = 5 # events (for each core)
        #self.dsource.break_after(self.end_after * size)

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
                self.smldata.gather()


            # break after indicated events
            if nevt == self.end_after:
                break

        # we're breaking early, so we're not giving the final gather a chance to run,
        # so gather by hand.
        self.smldata.gather() # test empty gather
        self.smldata.save()
        self.smldata.close()

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

            f = h5py.File(self.filename, mode="r")
            np.testing.assert_allclose( np.array(f['a']),
                                        np.array(expected_a),
                                        err_msg='mismatch in a' )
            np.testing.assert_allclose( np.array(f['b']),
                                        np.array(expected_b),
                                        err_msg='mismatch in b' )
            np.testing.assert_allclose( np.array(f['c']), 
                                        np.array(expected_c),
                                        err_msg='mismatch in c' )

            if 'd' in list(f.keys()):
                np.testing.assert_allclose( np.array(f['d']),
                                            np.array(expected_d),
                                            err_msg='mismatch in d' )

            fid_arr = np.array(f['fiducials'])
            assert np.all((fid_arr[1:]-fid_arr[:-1])==3), \
                'fiducials not in order'

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


    def test_monitor(self):

        tst = []
        def monitor_test(dlist):
            tst.extend(list(dlist['dset']))

        self.smldata.add_monitor_function(monitor_test)

        for nevt,evt in enumerate(self.dsource.events()):
            x = rank + size * nevt # count up, each global event
            self.smldata.event(dset=x)

        np.testing.assert_array_equal( np.arange(len(tst)), np.array(tst) )

        return


    def test_vlen(self):

        for nevt,evt in enumerate(self.dsource.events()):
            self.smldata.event(a=1)   # makes sure every event gets data
            x = rank + size * nevt    # count up, each global event
            if x % 3 != 0:            # adds missing data...
                self.smldata.event(ragged_dset=np.arange(x)) 
                self.smldata.event(var_dset=np.arange(x)) 
        self.smldata.save()

        if rank == 0:
            f = h5py.File(self.filename, mode="r")
            vlen = f['var_dset_len']
            vd   = f['var_dset']
            j = 0
            for l,row in enumerate(f['ragged_dset']):
                if l % 3 != 0:
                    np.testing.assert_array_equal(np.arange(l), row)
                    np.testing.assert_array_equal(np.arange(l), vd[j:j+vlen[l]])
                else:
                    np.testing.assert_array_equal(np.empty(0), row)
                    np.testing.assert_equal(vlen[l], 0)
                j += vlen[l]
            f.close()

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
        return np.ones((1,2,3)).astype(np.int32)
    def missing(self):
        return smalldata_mod.MISSING_INT * np.ones(self.dataset().shape, dtype=np.int32)

class TestFloatArray(TestSmallData):
    def dataset(self):
        return np.ones((1,2,3)).astype(np.float32)
    def missing(self):
        return smalldata_mod.MISSING_FLOAT * np.zeros(self.dataset().shape)


if __name__ == '__main__':

    try:

        print('Testing: monitor')
        t = TestSmallData()
        t.setup()
        t.test_monitor()
        t.teardown()

        print('Testing: vlen')
        t = TestSmallData()
        t.setup()
        t.test_vlen()
        t.teardown()

        for Test in [TestInt, TestFloat, TestIntArray, TestFloatArray]:
            t = Test()
            t.setup()
            if rank == 0: print('Testing: %s' % t.filename)
            t.test_h5gen()
            t.teardown()

    except Exception as e:
        print(e)
        comm.Abort(1)
