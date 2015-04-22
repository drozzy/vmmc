# LibVMMC

Copyright &copy; 2015 Lester Hedges.

Released under the [GPL](http://www.gnu.org/copyleft/gpl.html).

## About
A simple C++ library to implement the "virtual-move" Monte Carlo (VMMC)
algorithm of [Steve Whitelam](http://nanotheory.lbl.gov/people/SteveWhitelam.html)
and [Phill Geissler](http://www.cchem.berkeley.edu/plggrp/index.html), see:

* Avoiding unphysical kinetic traps in Monte Carlo simulations of strongly
attractive particles, S. Whitelam and P.L. Geissler,
[Journal of Chemical Physics, 127, 154101 (2007)](http://dx.doi.org/10.1063/1.2790421)

* Approximating the dynamical evolution of systems of strongly interacting
overdamped particles, S. Whitelam,
[Molecular Simulation, 37 (7) (2011)](http://dx.doi.org/10.1080/08927022.2011.565758).
(Preprint version available [here](http://arxiv.org/abs/1009.2008).)

Our primary goal is to make VMMC accessible to a wider audience, for whom the
time required to code the algorithm poses a significant barrier to using the
method. This allows the user to focus on model development.

## Installation
A `Makefile` is included for building and installing LibVMMC.

To compile LibVMMC, then install the library, documentation, and demos:

```bash
$ make build
$ make install
```

By default, the library installs to `/usr/local`. Therefore, you may need admin
priveleges for the final `make install` step above. An alternative is to change
the install location:

```bash
$ PREFIX=MY_INSTALL_DIR make install
```

Further details on using the Makefile can be found by running make without
a target, i.e.

```bash
$ make
```

## Compiling and linking
To use LibVMMC with a C/C++ code first include the LibVMMC header file somewhere
in the code.

```bash
//example.cpp
#include <vmmc/VMMC.h>
```

Then to compile, we can use something like the following:

```bash
$ g++ -std=c++11 example.cpp -lvmmc
```

This assumes that we have used the default install location `/usr/local`. If
we specify an install location, we would use a command more like the following:

```bash
$ g++ -std=c++11 example.cpp -I/my/path/include -L/my/path/lib -lvmmc
```

Note that the `-std=c++11` compiler flag is needed for `std::function`.

## Dependencies
LibVMMC uses the [Mersenne Twister](http://en.wikipedia.org/wiki/Mersenne_Twister)
psuedorandom number generator. A C++11 implementation using `std::random` is
included as a bundled header file, `MersenneTwister.h`.

## Callback functions
LibVMMC works via four user-defined callback functions that abstract model
specific details, such as the pair potential. We make use of C++11's
`std::function` to provide a general-purpose function wrapper, i.e.
the callbacks can be free functions, member functions, etc. These callbacks
allow LibVMMC to be blind to the implementation of the model, as well as
the model to be blind to the details of the VMMC algorithm. The generic
nature of the function wrapper provides great flexibility to the user, freeing
them from a specific design choice for the model in hand. It is possible to
glue together components written in different ways, or to use the callbacks
themselves as C/C++ wrappers to external libraries.

The callbacks have the following prototypes:

### Particle energy
Calculate the total pair interaction energy felt by a particle.
```cpp
typedef std::function<double (unsigned int index, double position[],
    double orientation[])> VMMC_energyCallback;
```
`index` = The particle index.

`position` = An x, y, z (or x, y in 2D) coordinate vector for the particle.

`orientation` = The particle's orientation unit vector.

This callback function is currently somewhat redundant since it is possible to
achieve the same outcome by combining the `pairEnergyCallback` and
`interactionsCallback` functions described below. Ultimately, the callback
will be able to account for non-pairwise terms in the potential, such as
an external field.

### Pair energy
Calculate the pair interaction between two particles.
```cpp
typedef std::function<double (unsigned int index1, double position1[],
    double orientation1[], unsigned int index2, double position2[],
    double orientation2[])> VMMC_pairEnergyCallback;
```
`index1` = The index of the first particle.

`position1` = The coordinate vector of the first particle.

`orientation1` = The orientation unit vector of the first particle.

`index2` = The index of the second particle.

`position2` = The coordinate vector of the second particle.

`orientation2` = The orientation unit vector of the second particle.

### Interactions
Determine the interactions for a given particle.
```cpp
typedef std::function<unsigned int (unsigned int index, double position[],
    double orientation[], unsigned int interactions[])> VMMC_interactionsCallback;
```
`index` = The index of the  particle.

`position` = The coordinate vector of the particle.

`orientation` = The orientation unit vector of the particle.

`interactions` = An array to store the indices of the interactions.

### Post-move
Apply any post-move updates, e.g. update cell lists, or neighbour lists.
```cpp
typedef std::function<void (unsigned int index, double position[],
    double orientation[])> VMMC_postMoveCallback;
```
`index` = The index of the  particle.

`position` = The coordinate vector of the particle following the move.

`orientation` = The orientation unit vector of the particle following the move.

## The VMMC object
To use LibVMMC you will want to create an instance of the VMMC object. This has the following
constructor:
```cpp
VMMC(unsigned int nParticles, unsigned int dimension, double coordinates[],
    double orientations[], double maxTrialTranslation, double maxTrialRotation,
    double probTranslate, double referenceRadius, unsigned int maxInteractions,
    double boxSize[], bool isRepulsive, energyCallback computeEnergy,
    pairEnergyCallback computePairEnergy, interactionsCallback computeInteractions,
    postMoveCallback, applyPostMoveUpdates);
```
`nParticles` = The number of particles in the simulation box.

`dimension` = The dimension of the simulation box (either 2 or 3).

`coordinates` = An array containing coordinates for all of the particles in the
system, i.e. `x1, y1, z1, x2, y2, z2, ... , xN, yN, zN.`
Coordinates should run from 0 to the box size in each dimension.

`orientations` = An array containing orientations (unit vectors) for all of the
particles in the system, i.e. `nx1, ny1, nz1, nx2, ny2, nz2, ... , nxN, nyN, nzN.`
In the case of particles interacting via an isotropic potential, the particle
orientations are simply dummy unit vectors that provide a means for LibVMMC
to execute rotational moves, i.e. the orientation has no effect on the potential.
This allows the use of a single set of callback functions for models with both
isotropic and anisotropic potentials.

`maxTrialTranslation` = The maximum trial translation, in units of the particle
diameter (or typical particle size).

`maxTrialTranslation` = The maximum trial rotation in radians.

`probTranslate` = The probability of attempting a translation move (relative to rotations).

`referenceRadius` = A reference radius for computing the approximate hydrodynamic
damping factor, e.g. the radius of a typical particle in the system.

`maxInteractions` = The maximum number of pair interactions that an individual
particle can make. This will be used to resize LibVMMC's internal data
structures and the user should assert that this limit isn't exceed in the
`interactionsCallback` function. The number can be chosen from the symmetry
of the system, e.g. if particles can only make a certain number of patchy
interactions, or by estimating the average number of neighbours within the
interaction volume around a particle.

`boxSize` = The base length of the simulation box in each dimension.

`isRepulsive` = Whether the potential has finite energy repulsions.

`computeEnergy` = The callback function to calculate the total pair interaction
for a particle.

`computePairEnergy` = The callback function to calculate the pair interaction
between two particles.

`computeInteractions` = The callback function to determine the neighbours with
which a particle interacts.

`applyPostMoveUpdates` = The callback function to perform any required updates
following the move. Here you should copy the updated particle position and
orientation back into your own data structures and implement any additional
updates, e.g. cell lists.

## Executing a virtual-move
Once an instance of the VMMC object is created, e.g.
```cpp
VMMC(...) vmmc;
```
then a single trial move can be executed as follows:
```cpp
vmmc.step();
```
To perform 1000 trial moves:
```cpp
vmmc.step(1000);
```
The same can be achieved by using the overloaded `++` and `+=` operators,
i.e. `vmmc++` for a single step, and `vmmc += 1000` for 1000 steps.

## Demos
The following example codes showing how to interface with LibVMMC are included
in the `demos` directory.

* `square_wellium.cpp`: A simulation of a square-well fluid in two- or three-dimensions.
* `lennard_jonesium.cpp`: A simulation of a Lennard-Jones fluid in two- or three-dimensions.

Both demo codes output a trajectory file, `trajectory.xyz`, and a TcL script,
`vmd.tcl` that can be used to set camera and particle attributes and to draw the
periodic simulation box when visualising the trajectory with
[VMD](http://www.ks.uiuc.edu/Research/vmd/). To generate and view a trajectory,
run, e.g.

```bash
$ ./demos/square_wellium
$ vmd trajectory.xyz -e vmd.tcl
```

The demo code also illustrates how to implement efficient, dynamically
updated cell lists. See `demos/src/CellList.h` and `demos/src/CellList.cpp`
for implementation details.

## Limitations
* For spherical particles bearing isotropic interactions, e.g. the square-well
fluid, single particle rotations will always be accepted. While not a problem
from a thermodynamic perspective, this may cause issues if the user wishes to
enforce a strict Stokes scaling of translational and rotational diffusion.
* The recursive manner in which the trial cluster is built can lead to a stack
overflow if the cluster contains many particles. Typically, thousands, or tens
of thousands of particles should be perfectly manageable. The typical memory
footprint for a simulation of 1000 particles is around 2.5MB for hard particles.
This is roughly doubled if the potential has finite energy repulsions.

## Efficiency
In aid of generality there are several sources of redundancy that impact the
efficiency of the VMMC implementation. As written, LibVMMC performs around 3-4
times worse than a fully optimised VMMC code for square-well fluids. A few
efficiency considerations are listed below in case the user wishes to modify
the VMMC source code in order to improve performance.

* When calculating a list of neighbours with which a given particle interacts
it's likely that you'll need to calculate the pair interaction energy. For
certain models it may be more efficient to return a list of pair energies
along with the interactions, rather than having to recalculate them.
* For models with an isotropic interaction of fixed energy scale the pair
energy is simply a constant. As such, the pair energy calculation is entirely
redundant, i.e. knowing that two particles interact is enough to know the
pair energy.
* If using cell lists, the typical size of a trial displacement will be small
enough such that a particle stays within the same neighbourhood of cells
following the trial move. As such, there is often no need to update cell lists
until confirming that the post-move configuration is valid, e.g. no overlaps.
At present the same `postMoveCallback` function is called twice: once in order
to apply the move; again if the move is subsequently rejected. This means that
the cell lists will be updated twice if a move is rejected.

## Tips
* LibVMMC currently assumes that the simulation box is periodic in all dimensions.
To impose non-periodic boundaries simply check whether the move leads to a particle
being displaced by more than half the box width along the restricted dimension and
return an appropriately large energy so that the move will be rejected.
* It is not a requirement that all particles in the simulation box be of the same
type. Make use of the particle indices that are passed to callback functions in
order to distinguish different species.
* The use of `std::function` allows the user to wrap arbitary functions as callbacks
(rather than only using free functions, as with C-style function pointers). See
[here](http://en.cppreference.com/w/cpp/utility/functional/function) for details
on how to bind member functions, or function objects.

## TODO
A test suite is forthcoming. This will allow comparison between VMMC and single
particle dynamics for model systems at well-known state points, e.g. equilibrium
energy and pair distribution functions.
