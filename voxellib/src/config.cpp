#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/lexical_cast.hpp>
#include "config.h"
#include "threadpool.h"
#include "world/generation/generator.h"
#include "loguru/loguru.hpp"

namespace config {
    unsigned int kTerrainThreadWorkers, kInitialLoadedChunkRadius;


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
        auto val = get<float>(tree, key);
        int count;

        // use max concurrency
        if (val <= 0)
            count = ThreadPool::hardware_concurrency();
        else {
            if (ceilf(val) == floorf(val))
                count = static_cast<int>(val); // integer
            else {
                val = fminf(val, 1.f);
                count = static_cast<int>(floorf(val * ThreadPool::hardware_concurrency()));
            }
        }

        // default
        if (count <= 0)
            count = 2;

        return count;
    }

    static GeneratorType generator(const boost::property_tree::ptree &tree, std::string &out) {
        auto str = get<std::string>(tree, "terrain.generator");
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
        kInitialLoadedChunkRadius = get<unsigned int>(tree, "terrain.load_radius");
        if (kInitialLoadedChunkRadius < 1) kInitialLoadedChunkRadius = 1;
        LOG_F(INFO, "config: terrain.load_radius == %d", kInitialLoadedChunkRadius);
    }

}
