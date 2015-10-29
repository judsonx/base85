# Base85

Ascii85 encoder/decoder written in C. Also supports encoding/decoding with the
ZeroMQ (Z85) alphabet.

## Building

From the root of the project folder:

  - Create a build folder: `mkdir build && cd build`
  - Generate build files: `cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..`
  - Return to the project folder: `cd ..`
  - Build project: `cmake --build build`

The binaries will be generated in the `build` folder.

## Tests

Currently, tests only exist for the 'ascii85' encoder/decoder.

  - Run the tests (from the project folder): `build/ascii85_test`

## CLI

The command line applications, `ascii85`, and `z85` read from `stdin` and write
to `stdout`. Run `ascii85 -e` to encode data from `stdin`, and run `ascii85 -d`
to decode data from `stdin` (same holds for `z85`).

### License

MIT
