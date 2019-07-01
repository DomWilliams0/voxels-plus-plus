import math
from enum import Enum, auto

import noise
import numpy as np


# from voxellib
class BlockType(Enum):
    AIR = 0
    GRASS = 1
    STONE = 2
    DARKSTONE = 3
    SNOW = 4
    SAND = 5
    MARKER = 6


def ranged_height(x, z, seed, scale, new_min=0.0, new_max=1.0):
    n = noise.snoise3(x / scale, seed, z / scale)
    nmin = -1
    nmax = +1

    scaled = ((n - nmin) * (new_max - new_min)) / (nmax - nmin) + new_min
    # assert(scaled >= new_min and scaled <= new_max)
    return scaled


# class BiomeType(Enum):
#     DESERT = auto()  # no trees and desert
#     GRASSLAND = auto()  # plains
#     FOREST_BOREAL = auto()  # pine trees, cold
#     FOREST_TEMPERATE = auto()  # normal forest
#     TUNDRA = auto()  # few trees and snowy
#     ICE = auto()  # mountain top


class Biome:
    min_height = None
    max_height = None
    scale = None

    @classmethod
    def height_at(cls, x, z, seed):
        return ranged_height(x, z, seed, cls.scale, cls.min_height, cls.max_height)

    @classmethod
    def type_at(cls, x, z, elevation):
        raise NotImplementedError()


class BiomeHilly(Biome):
    min_height = 0.2
    max_height = 0.5
    scale = 120.34

    @classmethod
    def type_at(cls, x, z, elevation):
        n = ranged_height(x, z, elevation, 100.1111)
        return BlockType.GRASS if n < 0.9 else BlockType.STONE


class BiomePlains(Biome):
    min_height = 0.1
    max_height = 0.3
    scale = 300.113

    @classmethod
    def type_at(cls, x, z, elevation):
        return BlockType.GRASS


class BiomeMountains(Biome):
    min_height = 0.5
    max_height = 0.9
    scale = 10.222

    @classmethod
    def type_at(cls, x, z, elevation):
        n = ranged_height(x, z, elevation - 3.99181, 10.776, 0.0, 1.0)
        if elevation > 0.8 and n < 0.6:
            return BlockType.SNOW
        elif n < 0.8:
            return BlockType.DARKSTONE
        else:
            return BlockType.STONE


BIOMES = [
    BiomeHilly, BiomePlains, BiomeMountains
]


def weighted_elevation(x, z, biome_noise, seed):
    """
    :param x:
    :param z:
    :param biome_noise:
    :param seed:
    :return: (biome class, elevation between 0 and 1)
    """
    n = len(BIOMES)
    out = 0
    max_biome = (-1, None)
    # TODO this weighting is terrible, all biomes have an equal chance everywhere. use humidity+latitude
    for i, biome in enumerate(BIOMES):
        if (i - 1) / n <= biome_noise or biome_noise <= (i + 1) / n:
            e = biome.height_at(x, z, seed)
            w = (-abs(n * biome_noise - 1) + 1)

            weighted = w * e
            out += weighted

            if weighted > max_biome[0]:
                max_biome = (weighted, biome)

    clamped = max(0, min(out, 1))
    return max_biome[1], clamped


# wraps every THIS * pi/2
LATITUDE_SCALE = 200


def latitude(x):
    lat = math.sin(x / LATITUDE_SCALE)

    # scale from -1->1 to 0-1
    return (lat + 1) / 2


def final_elevation(x, z, seed):
    """
    call me
    :param x:
    :param z:
    :return: (elevation 0 to 1, block type enum)
    """

    offset = 0.381
    xoff = x + offset
    zoff = z + offset
    biome_noise = ranged_height(xoff, zoff, seed, 600.555, 0.0, 1.0)

    # TODO use these too
    # humidity = noise_scaled(x, z, 4.88272, scale)
    # latitude = latitude(x)

    biome, elevation = weighted_elevation(xoff, zoff, biome_noise, seed)
    blocktype = biome.type_at(x, z, elevation).value

    return elevation, blocktype


#
# def lookup(elevation, humidity, latitude):
#     def getweights(lookup, val):
#         weights = {biome: 0 for biome, _ in lookup}
#         all_dists = 0
#         for biome, lim in lookup:
#             dist = abs(val - lim)
#             all_dists += dist
#             weights[biome] = dist
#
#         return {b.name: (1 - w) / all_dists for b, w in weights.items()}
#
#     ELEVATION = [
#         (Biome.FOREST_TEMPERATE, 0.3),
#         (Biome.FOREST_BOREAL, 0.6),
#         (Biome.TUNDRA, 0.8),
#         (Biome.ICE, 1),
#     ]
#
#     LATITUDE = [
#         (Biome.FOREST_TEMPERATE, 0.5),
#         (Biome.TUNDRA, 0.8),
#         (Biome.ICE, 1),
#     ]
#
#     # def elevation_lookup(val):
#     #     return dolookup(ELEVATION, val)
#
#     out = {biome.name: 0 for biome in Biome}
#     weights = getweights(ELEVATION, elevation)
#     for k, v in weights.items():
#         out[k] += v
#
#     # weights = getweights(LATITUDE, latitude)
#     # for k, v in weights.items():
#     #     out[k] += v * 0.3
#
#     sorted_weights = sorted(weights.items(), key=operator.itemgetter(1), reverse=True)
#     return sorted_weights

# COLOURS = {
#     BiomeType.ICE: (212, 215, 216),
#     BiomeType.TUNDRA: (193, 225, 229),
#     BiomeType.FOREST_BOREAL: (28, 100, 6),
#     BiomeType.FOREST_TEMPERATE: (83, 157, 61),
# }
# for k in list(COLOURS):
#     COLOURS[k.name] = COLOURS[k]
#     del COLOURS[k]

if __name__ == '__main__':
    from PIL import Image

    sz = 100
    arr = np.zeros((sz, sz, 3), dtype=np.uint8)

    for x in range(sz):
        for z in range(sz):
            real_elevation, blocktype = final_elevation(x, z, 100)
            p = real_elevation * 255
            p = (int(p),) * 3
            arr[x, z] = p

    im = Image.fromarray(arr, "RGB")
    im.show()
