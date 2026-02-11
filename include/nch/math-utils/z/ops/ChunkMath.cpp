#include "ChunkMath.h"
using namespace nch;

int64_t nch::chunkedI64(int64_t worldPos) {
    return std::floor(worldPos/32.0);
}
int64_t nch::chunkedF(float worldPos) {
    return std::floor(worldPos/32.0);
}
Vec3i64 nch::chunked3I64(Vec3i64 worldPos) {
    return Vec3i64(chunkedI64(worldPos.x), chunkedI64(worldPos.y), chunkedI64(worldPos.z));
}
Vec3i64 nch::chunked3F(Vec3f worldPos) {
    return Vec3i64(chunkedF(worldPos.x), chunkedF(worldPos.y), chunkedF(worldPos.z));
}
int64_t nch::subbedI64(int64_t worldPos) {
    worldPos %= 32;
    if(worldPos<0) worldPos+=32;
    return worldPos; 
}
float nch::subbedF(float worldPos) {
    float dec = worldPos-std::floor(worldPos); int64_t i64Pos = worldPos-dec;
    i64Pos %= 32;
    if(i64Pos<0) i64Pos+=32;

    float ret = (float)i64Pos+dec;
    return ret;
}
Vec3i64 nch::subbed3I64(Vec3i64 worldPos) {
    return Vec3i64(subbedI64(worldPos.x), subbedI64(worldPos.y), subbedI64(worldPos.z));   
}
Vec3f nch::subbed3F(Vec3f worldPos) {
    return Vec3f(subbedF(worldPos.x), subbedF(worldPos.y), subbedF(worldPos.z));   
}