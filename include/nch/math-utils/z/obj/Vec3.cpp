#include "nch/math-utils/vec3.h"
using namespace nch;

size_t Vec3i64Hash::operator()(const Vec3i64& v) const {
    //splitmix64 finalizer per lane, hash_combine-style folding into one 64-bit hash.
    auto mix = [](uint64_t x) {
        x += 0x9E3779B97F4A7C15ULL;
        x = (x^(x>>30))*0xBF58476D1CE4E5B9ULL;
        x = (x^(x>>27))*0x94D049BB133111EBULL;
        return x^(x>>31);
    };
    uint64_t h = mix((uint64_t)v.x);
    h ^= mix((uint64_t)v.y) + 0x9E3779B97F4A7C15ULL + (h<<6) + (h>>2);
    h ^= mix((uint64_t)v.z) + 0x9E3779B97F4A7C15ULL + (h<<6) + (h>>2);
    return (size_t)h;
}