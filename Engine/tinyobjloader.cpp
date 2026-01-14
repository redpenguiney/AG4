// Lets the implementation of tinyobjloader be put here so that mesh.cpp doesn't have to compile it every time.
// (we don't use vcpkg for tinyobjloader because it made ASAN scream)
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"