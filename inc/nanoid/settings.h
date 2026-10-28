#pragma once

#define NANOID_VERSION_MAJOR 3
#define NANOID_VERSION_MINOR 0
#define NANOID_VERSION_PATCH 1

// Namespace of nanoid's API.
//
// If you change it, the library and all code using it must be built with the same value.
// With CMake, set the NANOID_NAMESPACE cache variable, which passes it on to everything that links nanoid::nanoid.
#ifndef NANOID_NAMESPACE
    #define NANOID_NAMESPACE nanoid
#endif
