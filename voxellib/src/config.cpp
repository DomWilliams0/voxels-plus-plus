#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/thread/thread_pool.hpp>
#include "config.h"
#include "world/generation/generator.h"

namespace config {
    int kTerrainThreadWorkers;


    enum GeneratorType {
        kFlat,
        kPython,
        kNoise,
    };
    GeneratorType kGenType;

    IGenerator *new_generator() {
        switch (kGenType) {
            case kNoise:
                return new NativeGenerator;
            case kPython:
                return new PythonGenerator;
            case kFlat:
            default:
                return new DummyGenerator;
        }
    }

    static void resolve_path(std::string &out) {
        char *env = std::getenv("VOXELS_PATH");
        out.append(env ? env : ".");
        out.append("/config.xml");
    }

    static int thread_count(const boost::property_tree::ptree &tree, const char *key) {
        unsigned int count = tree.get<int>(key, 0);
        if (count <= 0)
            count = boost::thread::hardware_concurrency() * 2;

        if (count <= 0)
            count = 2;

        return count;
    }

    static GeneratorType generator(const boost::property_tree::ptree &tree, std::string &out) {
        std::string str = tree.get<std::string>("terrain.generator");
        GeneratorType type;

        if (str == "flat")
            type = GeneratorType::kFlat;
        else if (str == "noise")
            type = GeneratorType::kNoise;
        else if (str == "python")
            type = GeneratorType::kPython;
        else
            throw std::runtime_error("terrain.generator should be one of flat,noise,python");

        out = str;
        return type;
    }

    void load() {
        namespace pt = boost::property_tree;
        pt::ptree tree;

        std::string path;
        resolve_path(path);
        pt::read_xml(path, tree);

        // thread workers
        kTerrainThreadWorkers = thread_count(tree, "terrain.threads");
        log("config: terrain.threads == %d", kTerrainThreadWorkers);

        // generator
        std::string str;
        kGenType = generator(tree, str);
        log("config: terrain.generator == %s", str.c_str());
    }

}
