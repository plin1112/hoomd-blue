from hoomd import *
from hoomd import deprecated
from hoomd import hpmc

import unittest

import math

# this script needs to be run on two ranks

# initialize with one rank per partitions
context.initialize()

class muvt_updater_test(unittest.TestCase):
    def setUp(self):
        self.system = deprecated.init.create_random(N=1000,phi_p=0.001,min_dist=4.0,seed=12345)

    def tearDown(self):
        del self.muvt
        del self.mc
        del self.system
        context.initialize()

    def test_spheres(self):
        self.mc = hpmc.integrate.sphere(seed=123)
        self.mc.set_params(d=0.1)

        self.mc.shape_param.set('A', diameter=1.0)

        self.muvt=hpmc.update.muvt(mc=self.mc,seed=456,transfer_types=['A'])
        self.muvt.set_fugacity('A', 100)

        run(100)

    def test_convex_polyhedron(self):
        self.mc = hpmc.integrate.convex_polyhedron(seed=10,max_verts=8);
        self.mc.shape_param.set("A", vertices=[(-2,-1,-1),
                                               (-2,1,-1),
                                               (-2,-1,1),
                                               (-2,1,1),
                                               (2,-1,-1),
                                               (2,1,-1),
                                               (2,-1,1),
                                               (2,1,1)]);

        self.muvt=hpmc.update.muvt(mc=self.mc,seed=456,transfer_types=['A'])
        self.muvt.set_fugacity('A', 100)

        run(100)

    def test_sphere_union(self):
        self.mc = hpmc.integrate.sphere_union(seed=10);
        self.mc.shape_param.set("A", diameters=[1.0, 1.0], centers=[(-0.25, 0, 0), (0.25, 0, 0)]);

        self.muvt=hpmc.update.muvt(mc=self.mc,seed=456,transfer_types=['A'])
        self.muvt.set_fugacity('A', 100)

        run(100)

    def test_polyhedron(self):
        self.mc = hpmc.integrate.polyhedron(seed=10);
        self.mc.shape_param.set('A', vertices=[(-0.5, -0.5, -0.5), (-0.5, -0.5, 0.5), (-0.5, 0.5, -0.5), (-0.5, 0.5, 0.5), \
                                (0.5, -0.5, -0.5), (0.5, -0.5, 0.5), (0.5, 0.5, -0.5), (0.5, 0.5, 0.5)],\
                                faces = [(7, 3, 1, 5), (7, 5, 4, 6), (7, 6, 2, 3), (3, 2, 0, 1), (0, 2, 6, 4), (1, 0, 4, 5)]);

        self.muvt=hpmc.update.muvt(mc=self.mc,seed=456,transfer_types=['A'])
        self.muvt.set_fugacity('A', 100)

        run(100)

    def test_faceted_sphere(self):
        self.mc = hpmc.integrate.faceted_sphere(seed=10);
        self.mc.shape_param.set("A", normals=[(-1,0,0),
                                              (1,0,0),
                                              (0,1,0,),
                                              (0,-1,0),
                                              (0,0,1),
                                              (0,0,-1)],
                                    offsets=[-1]*6,
                                    vertices=[(-1,-1,-1),(-1,-1,1),(-1,1,-1),(-1,1,1),(1,-1,-1),(1,-1,1),(1,1,-1),(1,1,1)],
                                    diameter=2,
                                    origin=(0,0,0));

        self.muvt=hpmc.update.muvt(mc=self.mc,seed=456,transfer_types=['A'])
        self.muvt.set_fugacity('A', 100)

        run(100)

    def test_spheropolyhedron(self):
        self.mc = hpmc.integrate.convex_spheropolyhedron(seed=10);
        self.mc.shape_param.set("A", vertices=[(-2,-1,-1),
                                               (-2,1,-1),
                                               (-2,-1,1),
                                               (-2,1,1),
                                               (2,-1,-1),
                                               (2,1,-1),
                                               (2,-1,1),
                                               (2,1,1)]);

        self.muvt=hpmc.update.muvt(mc=self.mc,seed=456,transfer_types=['A'])
        self.muvt.set_fugacity('A', 100)

        run(100)

    def test_ellipsoid(self):
        self.mc = hpmc.integrate.ellipsoid(seed=10);
        self.mc.shape_param.set('A', a=0.5, b=0.25, c=0.125);

        self.muvt=hpmc.update.muvt(mc=self.mc,seed=456,transfer_types=['A'])
        self.muvt.set_fugacity('A', 100)

        run(100)

if __name__ == '__main__':
    unittest.main(argv = ['test.py', '-v'])
