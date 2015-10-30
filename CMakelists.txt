cmake_minimum_required (VERSION 3.3.0)
project (BASE85)

add_library (_ascii85 STATIC src/base85.c)

add_executable (ascii85 src/main.c)
target_link_libraries (ascii85 LINK_PUBLIC _ascii85)

add_executable (ascii85_test src/test.c)
target_link_libraries (ascii85_test LINK_PUBLIC _ascii85)

add_library (_z85 STATIC src/base85.c)
target_compile_definitions (_z85 PUBLIC -DB85_ZEROMQ)

add_executable (z85 src/main.c)
target_link_libraries (z85 LINK_PUBLIC _z85)

enable_testing ()
add_test (NAME test COMMAND ascii85_test)
