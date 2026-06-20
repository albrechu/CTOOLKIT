#pragma once

#include <TOOLKIT/TYPES.H>

#ifdef __cplusplus
extern "c" {
#endif

typedef struct UBYTE4   { U8 X, Y, Z, W;   } UBYTE4;
typedef struct USHORT2  { U16 X, Y;        } USHORT2;
typedef struct USHORT4  { U16 X, Y, Z, W;  } USHORT4;
typedef struct INT2     { I32 X, Y;        } INT2;
typedef struct INT3     { I32 X, Y, Z;     } INT3;
typedef struct UINT2    { U32 X, Y;        } UINT2;
typedef struct UINT3    { U32 X, Y, Z;     } UINT3;
typedef struct FLOAT2   { F32 X, Y;        } FLOAT2;
typedef struct FLOAT3   { F32 X, Y, Z;     } FLOAT3;
typedef struct FLOAT4   { F32 X, Y, Z, W;  } FLOAT4;
typedef struct FLOAT4X4 { FLOAT4 Cols[4];  } FLOAT4X4;
typedef struct QUAT     { F32 X, Y, Z, W;  } QUAT;
typedef struct AABB     { FLOAT3 Min, Max; } AABB;

const static FLOAT2   ZERO2       = { 0.0f, 0.0f };
const static FLOAT3   ZERO3       = { 0.0f, 0.0f, 0.0f };
const static FLOAT4   ZERO4       = { 0.0f, 0.0f, 0.0f, 0.0f };
const static FLOAT4X4 IDENTITY4X4 = { { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } } };
const static QUAT     IDENTITYQ   = { 0.0f, 0.0f, 0.0f, 1.0f };
const static FLOAT3   UP3         = { 0.0f, 1.0f, 0.0f };
const static FLOAT3   RIGHT3      = { 1.0f, 0.0f, 0.0f };
const static FLOAT3   FORWARD3    = { 0.0f, 0.0f, -1.0f };
const static FLOAT3   BACK3       = { 0.0f, 0.0f, 1.0f };
const static FLOAT3   LEFT3       = { -1.0f, 0.0f, 0.0f };
const static FLOAT3   DOWN3       = { 0.0f, -1.0f, 0.0f };

#define ZERO4X4 IDENTITY4X4
#define ZEROQ   IDENTITYQ

#define radians(degrees)   ((degrees) * (3.14159265358979323846f / 180.0f))
#define degrees(radians)   ((radians) * (180.0f / 3.14159265358979323846f))
#define clamp(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define lerp(a, b, t)      ((1.0f - (t)) * (a) + (t) * (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))

AABB AABBUnion(AABB x, AABB y);
AABB AABBIntersect(AABB x, AABB y);
BOOL AABBCollides(AABB x, AABB y);

FLOAT2 Lerp2(FLOAT2 a, FLOAT2 b, F32 t);
FLOAT3 Lerp3(FLOAT3 a, FLOAT3 b, F32 t);

FLOAT2 Clamp2(FLOAT2 v, FLOAT2 min, FLOAT2 max);
FLOAT3 Clamp3(FLOAT3 v, FLOAT3 min, FLOAT3 max);

FLOAT2 Min2(FLOAT2 x, FLOAT2 y);
FLOAT3 Min3(FLOAT3 x, FLOAT3 y);

FLOAT2 Max2(FLOAT2 x, FLOAT2 y);
FLOAT3 Max3(FLOAT3 x, FLOAT3 y);

FLOAT2 Scalar2(F32 x);
FLOAT3 Scalar3(F32 x);
FLOAT4 Scalar4(F32 x);

FLOAT2 Make2(F32 x, F32 y);
FLOAT3 Make3(F32 x, F32 y, F32 z);
FLOAT4 Make4(F32 x, F32 y, F32 z, F32 w);

FLOAT3 Extend23(FLOAT2 v, F32 z);
FLOAT4 Extend24(FLOAT2 v, F32 z, F32 w);
FLOAT4 Extend34(FLOAT3 v, F32 w);

F32 Dot2(FLOAT2 a, FLOAT2 b);
F32 Dot3(FLOAT3 a, FLOAT3 b);

F32 Length2(FLOAT2 v);
F32 Length3(FLOAT3 v);

FLOAT2 Add2(FLOAT2 a, FLOAT2 b);
FLOAT3 Add3(FLOAT3 a, FLOAT3 b);

FLOAT2 Sub2(FLOAT2 a, FLOAT2 b);
FLOAT3 Sub3(FLOAT3 a, FLOAT3 b);

FLOAT2   Mul2(FLOAT2 v, F32 s);
FLOAT3   Mul3(FLOAT3 v, F32 s);
FLOAT4X4 Mul4x4(FLOAT4X4 a, FLOAT4X4 b);
FLOAT4   Xform(FLOAT4X4 a, FLOAT4 b);
QUAT     Mulq(QUAT a, QUAT b);

FLOAT2 Normalize2(FLOAT2 v);
FLOAT3 Normalize3(FLOAT3 v);
QUAT   NormalizeQ(QUAT q);

FLOAT3 Cross3(FLOAT3 a, FLOAT3 b);

FLOAT4X4 Perspective(F32 fov, F32 aspect, F32 near, F32 far);
FLOAT4X4 Perspectiveinv(F32 fov, F32 aspect, F32 near, F32 far);

FLOAT4X4 Orthographic(F32 left, F32 right, F32 bottom, F32 top, F32 near, F32 far);
FLOAT4X4 OrthographicInv(F32 left, F32 right, F32 bottom, F32 top, F32 near, F32 far);

FLOAT4X4 Lookat(FLOAT3 eye, FLOAT3 center, FLOAT3 up);
FLOAT4X4 LookatInv(FLOAT3 eye, FLOAT3 center, FLOAT3 up);

VOID Frustum(const FLOAT4X4 *proj_inv, const FLOAT4X4 *view_inv, FLOAT4 corners[8]);

QUAT FromAxisAngle(FLOAT3 axis, F32 angle);
QUAT FromEuler(F32 pitch, F32 yaw, F32 roll);

QUAT Slerp(QUAT a, QUAT b, F32 t);
QUAT Nlerp(QUAT a, QUAT b, F32 t);

QUAT Conjugate(QUAT q);

QUAT FromTo(FLOAT3 from, FLOAT3 to);

QUAT RotationInbetween(FLOAT3 start, FLOAT3 dest);

FLOAT4X4 Cast4x4(QUAT q);

#ifdef __cplusplus
}
#endif
