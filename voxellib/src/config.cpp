#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/thread/thread_pool.hpp>
#include <boost/lexical_cast.hpp>
#include "config.h"
#include "world/generation/generator.h"

namespace config {
    int kTerrainThreadWorkers, kInitialLoadedChunkRadius;


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

    template<typename T>
    static T get(const boost::property_tree::ptree &tree, const std::string &key) {
        // check env first
        const char *prefix = "VOX_";
        std::string env_key(prefix);
        env_key.append(key);

        for (auto &c : env_key)
            c = std::toupper(c == '.' ? '_' : c);

        char *env_value = std::getenv(env_key.c_str());
        if (env_value)
            return boost::lexical_cast<T>(env_value);

        // get from config
        return tree.get<T>(key);
    }

    static int thread_count(const boost::property_tree::ptree &tree, const char *key) {
        unsigned int count = get<int>(tree, key);
        if (count <= 0)
            count = boost::thread::hardware_concurrency() * 2; // TODO use threadpool concurrency

        if (count <= 0)
            count = 2;

        return count;
    }

    static GeneratorType generator(const boost::property_tree::ptree &tree, std::string &out) {
        std::string str = get<std::string>(tree, "terrain.generator");
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
        LOG_F(INFO, "config: terrain.threads == %d", kTerrainThreadWorkers);

        // generator
        std::string str;
        kGenType = generator(tree, str);
        LOG_F(INFO, "config: terrain.generator == %s", str.c_str());

        // chunk radius
        kInitialLoadedChunkRadius = get<int>(tree, "terrain.load_radius");
        if (kInitialLoadedChunkRadius < 1) kInitialLoadedChunkRadius = 1;
        LOG_F(INFO, "config: terrain.load_radius == %d", kInitialLoadedChunkRadius);
    }

}
