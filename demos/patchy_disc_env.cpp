/*
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef ISOTROPIC
#error patchy_disc.cpp cannot be linked to isotropic VMMC library!
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "Demo.h"
#include "VMMC.h"

#ifndef M_PI
    #define M_PI 3.1415926535897932384626433832795
#endif

class PatchyDiscEnv {
private:
    // Simulation parameters.
    unsigned int dimension = 2;                     // dimension of simulation box
    unsigned int nParticles = 1000;                 // number of particles
    double interactionEnergy = 8.0;                 // pair interaction energy scale (in units of kBT)
    double interactionRange = 0.1;                  // diameter of patch (in units of particle diameter)
    double density = 0.2;                           // particle density
    double baseLength;                              // base length of simulation box
    unsigned int maxInteractions = 3;               // maximum number of interactions per particle (number of patches)

    std::vector<Particle> particles = std::vector<Particle>(nParticles);    // particle container

    CellList *cells;                                 // cell list

    Box *box;
    PatchyDisc *patchyDisc;
    // Initialise input/output class,
    InputOutput io;

    vmmc::VMMC *vmmc;
public:
    PatchyDiscEnv(){
        std::cout << "Constructor entered.\n";
        // Data structures.
        
        
        bool isIsotropic[nParticles];

        // Resize particle container.
        particles.resize(nParticles);

         // Work out base length of simulation box (particle diameter is one).
        if (dimension == 2) baseLength = std::pow((nParticles*M_PI)/(4.0*density), 1.0/2.0);
        else baseLength = std::pow((nParticles*M_PI)/(6.0*density), 1.0/3.0);

        std::vector<double> boxSize;
        for (unsigned int i=0;i<dimension;i++)
            boxSize.push_back(baseLength);

        // Initialise simulation box object.
        box = new Box(boxSize);

        // Create VMD script.
        io.vmdScript(boxSize);

        // Initialise cell list.
        cells = new CellList;
        cells->setDimension(dimension);
        cells->initialise(box->boxSize, 1 + 0.5*interactionRange);

        // Initialise the patchy disc model.
        patchyDisc = new PatchyDisc(*box, particles, *cells,
            maxInteractions, interactionEnergy, interactionRange);

        // Initialise random number generator.
        MersenneTwister rng;

        // Initialise particle initialisation object.
        Initialise initialise;

        // Generate a random particle configuration.
        initialise.random(particles, *cells, *box, rng, false);

        // Initialise data structures needed by the VMMC class.
        double coordinates[dimension*nParticles];
        double orientations[dimension*nParticles];

        // Copy particle coordinates and orientations into C-style arrays.
        for (unsigned int i=0;i<nParticles;i++)
        {
            for (unsigned int j=0;j<dimension;j++)
            {
                coordinates[dimension*i + j] = particles[i].position[j];
                orientations[dimension*i + j] = particles[i].orientation[j];
            }

            // Set all particles as anisotropic.
            isIsotropic[i] = false;
        }

        // Initialise the VMMC callback functions.
        using namespace std::placeholders;
        vmmc::CallbackFunctions callbacks;
    #ifndef ISOTROPIC
        callbacks.energyCallback =
            std::bind(&PatchyDisc::computeEnergy, patchyDisc, _1, _2, _3);
        callbacks.pairEnergyCallback =
            std::bind(&PatchyDisc::computePairEnergy, patchyDisc, _1, _2, _3, _4, _5, _6);
        callbacks.interactionsCallback =
            std::bind(&PatchyDisc::computeInteractions, patchyDisc, _1, _2, _3, _4);
        callbacks.postMoveCallback =
            std::bind(&PatchyDisc::applyPostMoveUpdates, patchyDisc, _1, _2, _3);
    #else
        callbacks.energyCallback =
            std::bind(&PatchyDisc::computeEnergy, patchyDisc, _1, _2);
        callbacks.pairEnergyCallback =
            std::bind(&PatchyDisc::computePairEnergy, patchyDisc, _1, _2, _3, _4);
        callbacks.interactionsCallback =
            std::bind(&PatchyDisc::computeInteractions, patchyDisc, _1, _2, _3);
        callbacks.postMoveCallback =
            std::bind(&PatchyDisc::applyPostMoveUpdates, patchyDisc, _1, _2);
    #endif

    // Initialise VMMC object.
    vmmc = new vmmc::VMMC(nParticles, dimension, coordinates, orientations,
        0.15, 0.2, 0.5, 0.5, maxInteractions, &boxSize[0], isIsotropic, false, callbacks);
    std::cout << "Initializer finished.\n";

    }

    ~PatchyDiscEnv(){
        std::cout << "Destructors.\n";
        
        delete box;
        delete cells;
        delete patchyDisc;
        std::cout << " PatchyDisk destroyed.\n";
        delete vmmc;
        std::cout << "VMMC destroyed.\n";
    }
    void execute(){
        std::cout << "Execute entered.\n";
        for (unsigned int i=0;i<1000;i++)
        {
            // Increment simulation by 1000 Monte Carlo Sweeps.
            std::cout << "  Particles...\n";
            vmmc += 10*nParticles;
 
            std::cout << "  Trajectory\n";

            // Append particle coordinates to an xyz trajectory.
            if (i == 0) {
                io.appendXyzTrajectory(dimension, particles, true);
                std::cout << "  Append True\n";
            }
            else {
                std::cout << "  Append False\n";
                io.appendXyzTrajectory(dimension, particles, false);
            }
            std::cout << "Report\n";
            // Report.
            printf("sweeps = %9.4e, energy = %5.4f\n", ((double) (i+1)*1000), patchyDisc->getEnergy());
            std::cout << "End report\n";
        }

        std::cout << "\nComplete!\n";
    }
};


int main(int argc, char** argv)
{
    PatchyDiscEnv env = PatchyDiscEnv();
    std::cout << "Beginning execution\n";
    env.execute();
    std::cout << "Ending execution\n";

    // We're done!
    return (EXIT_SUCCESS);
}
