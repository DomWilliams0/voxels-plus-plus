#ifndef VOXELS_CONFIG_H
#define VOXELS_CONFIG_H

#include <string>

class IGenerator;

// loaded once on startup and never updated
namespace config {

    // type of terrain generation
    // terrain.generator
    IGenerator *new_generator();

    // number of worker threads for terrain generation
    // terrain.threads
    // defaults to hardware limit if 0/not present
    extern int kTerrainThreadWorkers;


    // loads from config.json
    // to be called once only
    // throws on error
    void load();
}


#endif
