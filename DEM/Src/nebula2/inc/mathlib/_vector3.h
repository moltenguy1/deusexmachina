#ifndef _VECTOR3_H
#define _VECTOR3_H
//------------------------------------------------------------------------------
/**
    @class _vector3
    @ingroup NebulaMathDataTypes

    Generic vector3 class. Uses 16 Byte of mem instead of 12 (!)

    (C) 2002 RadonLabs GmbH
*/
#include "mathlib/nmath.h"
#include <float.h>

class _vector3
{
public:

	static const _vector3 Zero;
	static const _vector3 Up;

	union
	{
		struct { float x, y, z; };
		float v[3];
	};

	_vector3(): x(0.f), y(0.f), z(0.f) {}
	_vector3(const float _x, const float _y, const float _z): x(_x), y(_y), z(_z) {}
	_vector3(const _vector3& vec): x(vec.x), y(vec.y), z(vec.z) {}
	_vector3(const float* vec): x(vec[0]), y(vec[1]), z(vec[2]) {}

	static float	Distance(const _vector3& v0, const _vector3& v1);
	static float	Distance2D(const _vector3& v0, const _vector3& v1) { return n_sqrt(SqDistance2D(v0, v1)); }
	static float	SqDistance2D(const _vector3& v0, const _vector3& v1);
	static float	angle(const _vector3& v0, const _vector3& v1);

	void			set(const float _x, const float _y, const float _z) { x = _x; y = _y; z = _z; }
	void			set(const _vector3& vec) { x = vec.x; y = vec.y; z = vec.z; }
	void			set(const float* vec) { x = vec[0]; y = vec[1]; z = vec[2]; }
	float			len() const { return n_sqrt(x * x + y * y + z * z); }
	float			lensquared() const { return x * x + y * y + z * z; }
	void			norm();

	bool			isequal(const _vector3& v, float tol) const { return n_fabs(v.x - x) <= tol && n_fabs(v.y - y) <= tol && n_fabs(v.z - z) <= tol; }
	int				compare(const _vector3& v, float tol) const;
	void			rotate(const _vector3& axis, float angle);
	void			lerp(const _vector3& v0, float lerpVal);
	void			lerp(const _vector3& v0, const _vector3& v1, float lerpVal);
	_vector3		findortho() const;
	void			saturate() { x = n_saturate(x); y = n_saturate(y); z = n_saturate(z); }
	float			dot(const _vector3& v0) const { return x * v0.x + y * v0.y + z * v0.z; }

	void operator +=(const _vector3& v0) { x += v0.x; y += v0.y; z += v0.z; }
	void operator -=(const _vector3& v0) { x -= v0.x; y -= v0.y; z -= v0.z; }
	void operator *=(float s) { x *= s; y *= s; z *= s; }
	void operator /=(float s) { s = 1.f / s; x *= s; y *= s; z *= s; }
	bool operator >(const _vector3& rhs) { return x > rhs.x || y > rhs.y || z > rhs.z; }
	bool operator <(const _vector3& rhs) { return x < rhs.x || y < rhs.y || z < rhs.z; }
	bool operator ==(const _vector3& rhs) { return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator !=(const _vector3& rhs) { return x != rhs.x || y != rhs.y || z != rhs.z; }
};

static inline _vector3 operator -(const _vector3& v)
{
	return _vector3(-v.x, -v.y, -v.z);
}
//---------------------------------------------------------------------

static inline _vector3 operator +(const _vector3& v0, const _vector3& v1)
{
	return _vector3(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z);
}
//---------------------------------------------------------------------

static inline _vector3 operator -(const _vector3& v0, const _vector3& v1)
{
	return _vector3(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z);
}
//---------------------------------------------------------------------

static inline _vector3 operator *(const _vector3& v0, const float s)
{
	return _vector3(v0.x * s, v0.y * s, v0.z * s);
}
//---------------------------------------------------------------------

static inline _vector3 operator /(const _vector3& v0, float s)
{
	s = 1.0f / s;
	return _vector3(v0.x * s, v0.y * s, v0.z * s);
}
//---------------------------------------------------------------------

// Dot product.
static inline float operator %(const _vector3& v0, const _vector3& v1)
{
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}
//---------------------------------------------------------------------

// Cross product.
static inline _vector3 operator *(const _vector3& v0, const _vector3& v1)
{
	return _vector3(v0.y * v1.z - v0.z * v1.y,
					v0.z * v1.x - v0.x * v1.z,
					v0.x * v1.y - v0.y * v1.x);
}
//---------------------------------------------------------------------

inline void _vector3::norm()
{
	float l = len();
	if (l > TINY)
	{
		l = 1.f / l;
		x *= l;
		y *= l;
		z *= l;
	}
}
//---------------------------------------------------------------------

inline int _vector3::compare(const _vector3& v, float tol) const
{
	if (n_fabs(v.x - x) > tol) return (v.x > x) ? +1 : -1;
	else if (n_fabs(v.y - y) > tol) return (v.y > y) ? +1 : -1;
	else if (n_fabs(v.z - z) > tol) return (v.z > z) ? +1 : -1;
	else return 0;
}
//---------------------------------------------------------------------

inline void _vector3::rotate(const _vector3& axis, float angle)
{
	// rotates this one around given vector. We do
	// rotation with matrices, but these aren't defined yet!
	float rotM[9];
	float sa, ca;

	sa = (float) sin(angle);
	ca = (float) cos(angle);

	// build a rotation matrix
	rotM[0] = ca + (1 - ca) * axis.x * axis.x;
	rotM[1] = (1 - ca) * axis.x * axis.y - sa * axis.z;
	rotM[2] = (1 - ca) * axis.z * axis.x + sa * axis.y;
	rotM[3] = (1 - ca) * axis.x * axis.y + sa * axis.z;
	rotM[4] = ca + (1 - ca) * axis.y * axis.y;
	rotM[5] = (1 - ca) * axis.y * axis.z - sa * axis.x;
	rotM[6] = (1 - ca) * axis.z * axis.x - sa * axis.y;
	rotM[7] = (1 - ca) * axis.y * axis.z + sa * axis.x;
	rotM[8] = ca + (1 - ca) * axis.z * axis.z;

	// "handmade" multiplication
	_vector3 help(	rotM[0] * x + rotM[1] * y + rotM[2] * z,
					rotM[3] * x + rotM[4] * y + rotM[5] * z,
					rotM[6] * x + rotM[7] * y + rotM[8] * z);
	*this = help;
}
//---------------------------------------------------------------------

inline void _vector3::lerp(const _vector3& v0, float lerpVal)
{
	x = v0.x + ((x - v0.x) * lerpVal);
	y = v0.y + ((y - v0.y) * lerpVal);
	z = v0.z + ((z - v0.z) * lerpVal);
}
//---------------------------------------------------------------------

inline void _vector3::lerp(const _vector3& v0, const _vector3& v1, float lerpVal)
{
	x = v0.x + ((v1.x - v0.x) * lerpVal);
	y = v0.y + ((v1.y - v0.y) * lerpVal);
	z = v0.z + ((v1.z - v0.z) * lerpVal);
}
//---------------------------------------------------------------------

// Find a vector that is orthogonal to self. Self should not be (0,0,0).
// Return value is not normalized.
inline _vector3 _vector3::findortho() const
{
	if (x) return _vector3((-y - z) / x, 1.f, 1.f);
	else if (y) return _vector3(1.f, (-x - z) / y, 1.f);
	else if (z) return _vector3(1.f, 1.f, (-x - y) / z);
	else return _vector3(0.f, 0.f, 0.f);
}
//---------------------------------------------------------------------

inline float _vector3::Distance(const _vector3& v0, const _vector3& v1)
{
	_vector3 v(v1 - v0);
	return n_sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}
//---------------------------------------------------------------------

inline float _vector3::SqDistance2D(const _vector3& v0, const _vector3& v1)
{
	float vx = v1.x - v0.x;
	float vz = v1.z - v0.z;
	return vx * vx + vz * vz;
}
//---------------------------------------------------------------------

inline float _vector3::angle(const _vector3& v0, const _vector3& v1)
{
	_vector3 v0n = v0;
	_vector3 v1n = v1;
	v0n.norm();
	v1n.norm();
	return n_acos(v0n % v1n);
}
//---------------------------------------------------------------------

#endif
