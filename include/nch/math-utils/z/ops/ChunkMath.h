#pragma once
#include <nch/math-utils/vec3.h>
namespace nch {


int64_t chunkedI64(int64_t worldPos);
Vec3i64 chunked3I64(Vec3i64 worldPos);
int64_t subbedI64(int64_t worldPos);
Vec3i64 subbed3I64(Vec3i64 worldPos);

int64_t chunkedF(float worldPos);
Vec3i64 chunked3F(Vec3f worldPos);
float subbedF(float worldPos);
Vec3f subbed3F(Vec3f worldPos);

int64_t chunkedD(double worldPos);
Vec3i64 chunked3D(Vec3d worldPos);
double subbedD(double worldPos);
Vec3d subbed3D(Vec3d worldPos);

int64_t chunked512_I64(int64_t worldPos);
Vec3i64 chunked512_3I64(Vec3i64 worldPos);
int64_t subbed512_I64(int64_t worldPos);
Vec3i64 subbed512_3I64(Vec3i64 worldPos);


};
