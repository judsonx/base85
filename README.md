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
  - Or via CTest: `cmake --build build --target test`

## CLI

The command line utilities `ascii85` and `z85` can be used to encode and
decode data from **stdin**/**stdout** or from files.

Read from **stdin** and write to **stdout**:
  - Encode: `ascii85 -e`
  - Decode: `ascii85 -d`

Read from a file and write to **stdout**:
  - Encode: `ascii85 -e source`
  - Decode: `ascii85 -d source`

Read from a file and write to another file:
  - Encode: `ascii85 -e source destination`
  - Decode: `ascii85 -d source destination`

The same arguments are supported by the `z85` command.

### License

MIT
