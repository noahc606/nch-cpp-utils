#pragma once
#include <nch/math-utils/vec3.h>
namespace nch {
int64_t chunkedI64(int64_t worldPos);
int64_t chunkedF(float worldPos);
nch::Vec3i64 chunked3I64(nch::Vec3i64 worldPos);
nch::Vec3i64 chunked3F(nch::Vec3f worldPos);
int64_t subbedI64(int64_t worldPos);
float subbedF(float worldPos);
nch::Vec3i64 subbed3I64(nch::Vec3i64 worldPos);
nch::Vec3f subbed3F(nch::Vec3f worldPos);
};
