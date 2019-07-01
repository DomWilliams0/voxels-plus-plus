#!/usr/bin/env python3

import collections
import struct

import numpy

import biome

VERSION = 1

Req = collections.namedtuple("Req", "version cw ch cd x z seed")


# returns list of blocktypes of len chunk wxhxd
def gen(req):
    terrain = numpy.zeros((req.cw, req.ch, req.cd))

    # test pillar
    # for a in range(5):
    # terrain[a][a][a] = 1

    for x in range(req.cw):
        for z in range(req.cd):
            nx = (req.x * req.cw) + x
            nz = (req.z * req.cd) + z
            elevation, block = biome.final_elevation(nx, nz, req.seed)
            top = int(max(1, elevation * (req.ch - 1)))
            for y in range(top, 0, -1):
                terrain[z][y][x] = block

    return terrain


def write_img(req, terrain):
    def non_air(req, terrain, x, z):
        for y in range(req.ch - 1, 0, -1):
            b = terrain[x, y, z]
            if b != 0:
                return float(y) / req.ch
        return 0

    from PIL import Image
    import colorsys
    im = Image.new("RGB", (req.cw, req.cd))

    for x in range(req.cw):
        for z in range(req.cd):
            c = non_air(req, terrain, x, z)

            hue = 0.5
            sat = 0 if c == 0 else 0.5
            lig = c
            col = colorsys.hls_to_rgb(hue, sat, lig)
            im.putpixel((x, z), tuple(map(lambda x: int(x * 255), col)))
    im.save(f"test-{req.x}-{req.z}.bmp")


def go_server(gen_func):
    import socketserver

    class Handler(socketserver.BaseRequestHandler):
        def handle(self):
            while True:
                blob = self.request.recv(4096)
                if not blob:
                    return

                req = Req(*struct.unpack("7i", blob))
                print(f"{self.client_address[1]}: {req}")
                assert (req.version == VERSION)

                resp = gen_func(req)

                # TODO return code first
                fmt = f"{req.cw * req.ch * req.cd}i"
                blob = struct.pack(fmt, *list(map(int, resp.flat)))
                self.request.sendall(blob)

    class Server(socketserver.ForkingMixIn, socketserver.TCPServer):
        allow_reuse_address = True

    with Server(("localhost", 17771), Handler) as server:
        print("listening")
        server.serve_forever()


def mkreq(x, z, seed):
    return Req(VERSION,
               16, 16, 16,
               x, z, seed)


def go_test(gen_func):
    def recv(i):
        for x in range(i):
            yield mkreq(x, 1, 10)

    for req in recv(4):
        resp = gen_func(req)
        write_img(req, resp)


def main():
    import sys
    if len(sys.argv) > 1:
        go_test(gen)
    else:
        go_server(gen)


if __name__ == '__main__':
    main()