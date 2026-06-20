#include <TOOLKIT/MATH.H>
#include <math.h>

AABB AABBUnion(AABB x, AABB y)
{
   return (AABB) { .Min = Min3(x.Min, y.Min), .Max = Max3(x.Max, y.Max) };
}
AABB AABBIntersect(AABB x, AABB y)
{
   return (AABB) { .Min = Max3(x.Min, y.Min), .Max = Min3(x.Max, y.Max) };
}
BOOL AABBCollides(AABB x, AABB y)
{
   return (x.Min.X <= y.Max.X and x.Max.X >= y.Min.X) and (x.Min.Y <= y.Max.Y and x.Max.Y >= y.Min.Y) and (x.Min.Z <= y.Max.Z and x.Max.Z >= y.Min.Z);
}

FLOAT2 Lerp2(FLOAT2 a, FLOAT2 b, F32 t)
{
   return (FLOAT2) { lerp(a.X, b.X, t), lerp(a.Y, b.Y, t) };
}
FLOAT3 Lerp3(FLOAT3 a, FLOAT3 b, F32 t)
{
   return (FLOAT3) { lerp(a.X, b.X, t), lerp(a.Y, b.Y, t), lerp(a.Z, b.Z, t) };
}

FLOAT2 Clamp2(FLOAT2 v, FLOAT2 min, FLOAT2 max)
{
   return (FLOAT2) { clamp(v.X, min.X, max.X), clamp(v.Y, min.Y, max.Y) };
}
FLOAT3 Clamp3(FLOAT3 v, FLOAT3 min, FLOAT3 max)
{
   return (FLOAT3) { clamp(v.X, min.X, max.X), clamp(v.Y, min.Y, max.Y), clamp(v.Z, min.Z, max.Z) };
}

FLOAT2 Min2(FLOAT2 x, FLOAT2 y)
{
   return (FLOAT2) { Min(x.X, y.X), Min(x.Y, y.Y) };
}
FLOAT3 Min3(FLOAT3 x, FLOAT3 y)
{
   return (FLOAT3) { Min(x.X, y.X), Min(x.Y, y.Y), Min(x.Z, y.Z) };
}

FLOAT2 Max2(FLOAT2 x, FLOAT2 y)
{
   return (FLOAT2) { Max(x.X, y.X), Max(x.Y, y.Y) };
}
FLOAT3 Max3(FLOAT3 x, FLOAT3 y)
{
   return (FLOAT3) { Max(x.X, y.X), Max(x.Y, y.Y), Max(x.Z, y.Z) };
}

FLOAT2 Scalar2(F32 x) { return (FLOAT2) { x, x }; }
FLOAT3 Scalar3(F32 x) { return (FLOAT3) { x, x, x }; }
FLOAT4 Scalar4(F32 x) { return (FLOAT4) { x, x, x, x }; }

FLOAT2 Make2(F32 x, F32 y) { return (FLOAT2) { x, y }; }
FLOAT3 Make3(F32 x, F32 y, F32 z) { return (FLOAT3) { x, y, z }; }
FLOAT4 Make4(F32 x, F32 y, F32 z, F32 w) { return (FLOAT4) { x, y, z, w }; }

FLOAT3 Extend23(FLOAT2 v, F32 z) { return (FLOAT3) { v.X, v.Y, z }; }
FLOAT4 Extend24(FLOAT2 v, F32 z, F32 w) { return (FLOAT4) { v.X, v.Y, z, w }; }
FLOAT4 Extend34(FLOAT3 v, F32 w) { return (FLOAT4) { v.X, v.Y, v.Z, w }; }

F32 Dot2(FLOAT2 a, FLOAT2 b) { return a.X * b.X + a.Y * b.Y; }
F32 Dot3(FLOAT3 a, FLOAT3 b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z; }

F32 Length2(FLOAT2 v) { return sqrtf(Dot2(v, v)); }
F32 Length3(FLOAT3 v) { return sqrtf(Dot3(v, v)); }

FLOAT2 Add2(FLOAT2 a, FLOAT2 b) { return (FLOAT2) { a.X + b.X, a.Y + b.Y }; }
FLOAT3 Add3(FLOAT3 a, FLOAT3 b) { return (FLOAT3) { a.X + b.X, a.Y + b.Y, a.Z + b.Z }; }

FLOAT2 Sub2(FLOAT2 a, FLOAT2 b) { return (FLOAT2) { a.X - b.X, a.Y - b.Y }; }
FLOAT3 Sub3(FLOAT3 a, FLOAT3 b) { return (FLOAT3) { a.X - b.X, a.Y - b.Y, a.Z - b.Z }; }

FLOAT2 Mul2(FLOAT2 v, F32 s) { return (FLOAT2) { v.X *s, v.Y *s }; }
FLOAT3 Mul3(FLOAT3 v, F32 s) { return (FLOAT3) { v.X *s, v.Y *s, v.Z *s }; }
FLOAT4X4 Mul4x4(FLOAT4X4 a, FLOAT4X4 b)
{
   FLOAT4X4 result;
   for (int i = 0; i < 4; ++i)
   {
      for (int j = 0; j < 4; ++j)
      {
         result.Cols[i].X += a.Cols[j].X * b.Cols[i].X;
         result.Cols[i].Y += a.Cols[j].Y * b.Cols[i].Y;
         result.Cols[i].Z += a.Cols[j].Z * b.Cols[i].Z;
         result.Cols[i].W += a.Cols[j].W * b.Cols[i].W;
      }
   }
   return result;
}
FLOAT4 Xform(FLOAT4X4 a, FLOAT4 b)
{
   FLOAT4 result;
   result.X = a.Cols[0].X * b.X + a.Cols[1].X * b.Y + a.Cols[2].X * b.Z + a.Cols[3].X * b.W;
   result.Y = a.Cols[0].Y * b.X + a.Cols[1].Y * b.Y + a.Cols[2].Y * b.Z + a.Cols[3].Y * b.W;
   result.Z = a.Cols[0].Z * b.X + a.Cols[1].Z * b.Y + a.Cols[2].Z * b.Z + a.Cols[3].Z * b.W;
   result.W = a.Cols[0].W * b.X + a.Cols[1].W * b.Y + a.Cols[2].W * b.Z + a.Cols[3].W * b.W;
   return result;
}
QUAT Mulq(QUAT a, QUAT b)
{
   return (QUAT) {
      a.W *b.X + a.X * b.W + a.Y * b.Z - a.Z * b.Y,
         a.W *b.Y - a.X * b.Z + a.Y * b.W + a.Z * b.X,
         a.W *b.Z + a.X * b.Y - a.Y * b.X + a.Z * b.W,
         a.W *b.W - a.X * b.X - a.Y * b.Y - a.Z * b.Z
   };
}

FLOAT2 Normalize2(FLOAT2 v)
{
   F32 len = Length2(v);
   return len == 0.f ? ZERO2 : Mul2(v, 1.0f / len);
}
FLOAT3 Normalize3(FLOAT3 v)
{
   F32 len = Length3(v);
   return len == 0.f ? ZERO3 : Mul3(v, 1.0f / len);
}
QUAT NormalizeQ(QUAT q)
{
   F32 len = sqrtf(q.X * q.X + q.Y * q.Y + q.Z * q.Z + q.W * q.W);
   if (len > 0.0f)
   {
      F32 len_inv = 1.0f / len;
      return (QUAT) { q.X *len_inv, q.Y *len_inv, q.Z *len_inv, q.W *len_inv };
   }
   return IDENTITYQ;
}

FLOAT3 Cross3(FLOAT3 a, FLOAT3 b)
{
   return (FLOAT3) { a.Y *b.Z - a.Z * b.Y, a.Z *b.X - a.X * b.Z, a.X *b.Y - a.Y * b.X };
}

FLOAT4X4 Perspective(F32 fov, F32 aspect, F32 near, F32 far)
{
   FLOAT4X4 result = ZERO4X4;
   F32 tanHalfFov = tanf(fov / 2.0f);
   result.Cols[0].X = 1.0f / (aspect * tanHalfFov);
   result.Cols[1].Y = 1.0f / tanHalfFov;
   result.Cols[2].Z = -(far + near) / (far - near);
   result.Cols[2].W = -1.0f;
   result.Cols[3].Z = -(2.0f * far * near) / (far - near);
   return result;
}
FLOAT4X4 Perspectiveinv(F32 fov, F32 aspect, F32 near, F32 far)
{
   FLOAT4X4 r = ZERO4X4;
   F32 tan_half_fov = tanf(fov / 2.0f);
   r.Cols[0].X = aspect * tan_half_fov;
   r.Cols[1].Y = tan_half_fov;
   r.Cols[2].W = -1.0f;
   r.Cols[3].Z = -(far - near) / (2.0f * far * near);
   r.Cols[3].W = (far + near) / (2.0f * far * near);
   return r;
}

FLOAT4X4 Orthographic(F32 left, F32 right, F32 bottom, F32 top, F32 near, F32 far)
{
   FLOAT4X4 r = ZERO4X4;
   r.Cols[0].X = 2.0f / (right - left);
   r.Cols[1].Y = 2.0f / (top - bottom);
   r.Cols[2].Z = -2.0f / (far - near);
   r.Cols[3].X = -(right + left) / (right - left);
   r.Cols[3].Y = -(top + bottom) / (top - bottom);
   r.Cols[3].Z = -(far + near) / (far - near);
   r.Cols[3].W = 1.0f;
   return r;
}
FLOAT4X4 OrthographicInv(F32 left, F32 right, F32 bottom, F32 top, F32 near, F32 far)
{
   FLOAT4X4 r = ZERO4X4;
   r.Cols[0].X = (right - left) / 2.0f;
   r.Cols[1].Y = (top - bottom) / 2.0f;
   r.Cols[2].Z = (far - near) / -2.0f;
   r.Cols[3].X = (right + left) / 2.0f;
   r.Cols[3].Y = (top + bottom) / 2.0f;
   r.Cols[3].Z = (far + near) / -2.0f;
   r.Cols[3].W = 1.0f;
   return r;
}

FLOAT4X4 Lookat(FLOAT3 eye, FLOAT3 center, FLOAT3 up)
{
   FLOAT3 f = Normalize3(Sub3(center, eye));
   FLOAT3 s = Normalize3(Cross3(f, up));
   FLOAT3 u = Cross3(s, f);
   FLOAT4X4 result = IDENTITY4X4;
   result.Cols[0].X = s.X;
   result.Cols[1].X = s.Y;
   result.Cols[2].X = s.Z;
   result.Cols[0].Y = u.X;
   result.Cols[1].Y = u.Y;
   result.Cols[2].Y = u.Z;
   result.Cols[0].Z = -f.X;
   result.Cols[1].Z = -f.Y;
   result.Cols[2].Z = -f.Z;
   result.Cols[3].X = -Dot3(s, eye);
   result.Cols[3].Y = -Dot3(u, eye);
   result.Cols[3].Z = Dot3(f, eye);
   return result;
}
FLOAT4X4 LookatInv(FLOAT3 eye, FLOAT3 center, FLOAT3 up)
{
   FLOAT3 f = Normalize3(Sub3(center, eye));
   FLOAT3 s = Normalize3(Cross3(f, up));
   FLOAT3 u = Cross3(s, f);
   FLOAT4X4 r = IDENTITY4X4;
   r.Cols[0].X = s.X;
   r.Cols[0].Y = s.Y;
   r.Cols[0].Z = s.Z;
   r.Cols[1].X = u.X;
   r.Cols[1].Y = u.Y;
   r.Cols[1].Z = u.Z;
   r.Cols[2].X = -f.X;
   r.Cols[2].Y = -f.Y;
   r.Cols[2].Z = -f.Z;
   r.Cols[3].X = eye.X;
   r.Cols[3].Y = eye.Y;
   r.Cols[3].Z = eye.Z;
   return r;
}

VOID Frustum(const FLOAT4X4 *proj_inv, const FLOAT4X4 *view_inv, FLOAT4 corners[8])
{
   FLOAT4X4 ndc_to_world = Mul4x4(*view_inv, *proj_inv);
   FLOAT4 ndc_corners[8] = {
      // Near plane 
      { -1.0f, -1.0f, 0.0f, 1.0f }, // Top-Left Near
      {  1.0f, -1.0f, 0.0f, 1.0f }, // Top-Right Near
      { -1.0f,  1.0f, 0.0f, 1.0f }, // Bottom-Left Near
      {  1.0f,  1.0f, 0.0f, 1.0f }, // Bottom-Right Near
      // Far plane
      { -1.0f, -1.0f, 1.0f, 1.0f }, // Top-Left Far
      {  1.0f, -1.0f, 1.0f, 1.0f }, // Top-Right Far
      { -1.0f,  1.0f, 1.0f, 1.0f }, // Bottom-Left Far
      {  1.0f,  1.0f, 1.0f, 1.0f }  // Bottom-Right Far
   };

   for (I32 i = 0; i < 8; ++i)
   {
      FLOAT4 world_space_corner = Xform(ndc_to_world, ndc_corners[i]);
      // Perspective divide 
      corners[i].X = world_space_corner.X / world_space_corner.W;
      corners[i].Y = world_space_corner.Y / world_space_corner.W;
      corners[i].Z = world_space_corner.Z / world_space_corner.W;
      corners[i].W = 1.0f;
   }
}

QUAT FromAxisAngle(FLOAT3 axis, F32 angle)
{
   F32 half_angle = angle * 0.5f;
   F32 s = sinf(half_angle);
   return (QUAT) { axis.X *s, axis.Y *s, axis.Z *s, cosf(half_angle) };
}
QUAT FromEuler(F32 pitch, F32 yaw, F32 roll)
{
   F32 half_pitch = pitch * 0.5f;
   F32 half_yaw = yaw * 0.5f;
   F32 half_roll = roll * 0.5f;
   F32 sin_pitch = sinf(half_pitch);
   F32 cos_pitch = cosf(half_pitch);
   F32 sin_yaw = sinf(half_yaw);
   F32 cos_yaw = cosf(half_yaw);
   F32 sin_roll = sinf(half_roll);
   F32 cos_roll = cosf(half_roll);
   QUAT q;
   q.X = sin_roll * cos_pitch * cos_yaw - cos_roll * sin_pitch * sin_yaw;
   q.Y = cos_roll * sin_pitch * cos_yaw + sin_roll * cos_pitch * sin_yaw;
   q.Z = cos_roll * cos_pitch * sin_yaw - sin_roll * sin_pitch * cos_yaw;
   q.W = cos_roll * cos_pitch * cos_yaw + sin_roll * sin_pitch * sin_yaw;
   return q;
}

QUAT Slerp(QUAT a, QUAT b, F32 t)
{
   F32 dot = a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;
   if (dot < 0.0f)
   {
      b.X = -b.X;
      b.Y = -b.Y;
      b.Z = -b.Z;
      b.W = -b.W;
      dot = -dot;
   }
   if (dot > 0.9995f)
   {
      QUAT r;
      r.X = a.X + t * (b.X - a.X);
      r.Y = a.Y + t * (b.Y - a.Y);
      r.Z = a.Z + t * (b.Z - a.Z);
      r.W = a.W + t * (b.W - a.W);
      return NormalizeQ(r);
   }
   F32 theta_0 = acosf(dot);
   F32 theta = theta_0 * t;
   F32 sin_theta = sinf(theta);
   F32 sin_theta_0 = sinf(theta_0);
   F32 s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
   F32 s1 = sin_theta / sin_theta_0;
   QUAT r;
   r.X = s0 * a.X + s1 * b.X;
   r.Y = s0 * a.Y + s1 * b.Y;
   r.Z = s0 * a.Z + s1 * b.Z;
   r.W = s0 * a.W + s1 * b.W;
   return r;
}
QUAT Nlerp(QUAT a, QUAT b, F32 t)
{
   QUAT r;
   r.X = a.X + t * (b.X - a.X);
   r.Y = a.Y + t * (b.Y - a.Y);
   r.Z = a.Z + t * (b.Z - a.Z);
   r.W = a.W + t * (b.W - a.W);
   return NormalizeQ(r);
}

QUAT Conjugate(QUAT q)
{
   return (QUAT) { -q.X, -q.Y, -q.Z, q.W };
}
QUAT FromTo(FLOAT3 from, FLOAT3 to)
{
   FLOAT3 f = Normalize3(from);
   FLOAT3 t = Normalize3(to);
   FLOAT3 axis = Cross3(f, t);
   F32 dot = Dot3(f, t);
   if (dot < -0.999999f)
   {
      axis = Cross3((FLOAT3) { 1.0f, 0.0f, 0.0f }, f);
      if (Length3(axis) < 0.000001f)
         axis = Cross3((FLOAT3) { 0.0f, 1.0f, 0.0f }, f);

      axis = Normalize3(axis);
      return FromAxisAngle(axis, 3.14159265358979323846f);
   }
   F32 s = sqrtf((1.0f + dot) * 2.0f);
   F32 invs = 1.0f / s;
   return (QUAT) { axis.X *invs, axis.Y *invs, axis.Z *invs, s * 0.5f };
}
QUAT RotationInbetween(FLOAT3 start, FLOAT3 dest)
{
   FLOAT3 v0 = Normalize3(start);
   FLOAT3 v1 = Normalize3(dest);
   F32 d = Dot3(v0, v1);
   if (d >= 1.0f)
      return IDENTITYQ;

   if (d < (1e-6f - 1.0f))
   {
      FLOAT3 axis = Cross3((FLOAT3) { 1.0f, 0.0f, 0.0f }, v0);
      if (Length3(axis) < 1e-6f)
         axis = Cross3((FLOAT3) { 0.0f, 1.0f, 0.0f }, v0);

      axis = Normalize3(axis);
      return FromAxisAngle(axis, 3.14159265358979323846f);
   }
   F32 s = sqrtf((1 + d) * 2);
   F32 invs = 1 / s;
   FLOAT3 c = Cross3(v0, v1);
   return (QUAT) { c.X *invs, c.Y *invs, c.Z *invs, s * 0.5f };
}

FLOAT4X4 Cast4x4(QUAT q)
{
   FLOAT4X4 m = IDENTITY4X4;
   F32 x2 = q.X + q.X;  F32 y2 = q.Y + q.Y;  F32 z2 = q.Z + q.Z;
   F32 xx = q.X * x2;   F32 xy = q.X * y2;   F32 xz = q.X * z2;
   F32 yy = q.Y * y2;   F32 yz = q.Y * z2;   F32 zz = q.Z * z2;
   F32 wx = q.W * x2;   F32 wy = q.W * y2;   F32 wz = q.W * z2;
   // Column 0
   m.Cols[0].X = 1.0f - (yy + zz);
   m.Cols[0].Y = xy + wz;
   m.Cols[0].Z = xz - wy;
   m.Cols[0].W = 0.0f;
   // Column 1
   m.Cols[1].X = xy - wz;
   m.Cols[1].Y = 1.0f - (xx + zz);
   m.Cols[1].Z = yz + wx;
   m.Cols[1].W = 0.0f;
   // Column 2
   m.Cols[2].X = xz + wy;
   m.Cols[2].Y = yz - wx;
   m.Cols[2].Z = 1.0f - (xx + yy);
   m.Cols[2].W = 0.0f;
   // Column 3
   m.Cols[3].X = 0.0f;
   m.Cols[3].Y = 0.0f;
   m.Cols[3].Z = 0.0f;
   m.Cols[3].W = 1.0f;
   return m;
}