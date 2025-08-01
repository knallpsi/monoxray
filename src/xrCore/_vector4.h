#ifndef _VECTOR4_H
#define _VECTOR4_H
#pragma once

template <class T>
struct _vector4
{
	typedef T TYPE;
	typedef _vector4<T> Self;
	typedef Self& SelfRef;
	typedef const Self& SelfCRef;
public:
	T x, y, z, w;

	IC T& operator[](int i) { return *((T*)this + i); }
	IC T& operator[](int i) const { return *((T*)this + i); }

	IC SelfRef set(T _x, T _y, T _z, T _w = 1)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
		return *this;
	}

	IC SelfRef set(const Self& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		w = v.w;
		return *this;
	}

	IC SelfRef add(T _x, T _y, T _z, T _w = 1)
	{
		x += _x;
		y += _y;
		z += _z;
		w += _w;
		return *this;
	}

	IC SelfRef add(const Self& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		w += v.w;
		return *this;
	}

	IC SelfRef add(T s)
	{
		x += s;
		y += s;
		z += s;
		w += s;
		return *this;
	}

	IC SelfRef add(const Self& a, const Self& v)
	{
		x = a.x + v.x;
		y = a.y + v.y;
		z = a.z + v.z;
		w = a.w + v.w;
		return *this;
	}

	IC SelfRef add(const Self& a, T s)
	{
		x = a.x + s;
		y = a.y + s;
		z = a.z + s;
		w = a.w + s;
		return *this;
	}

	IC SelfRef sub(T _x, T _y, T _z, T _w = 1)
	{
		x -= _x;
		y -= _y;
		z -= _z;
		w -= _w;
		return *this;
	}

	IC SelfRef sub(const Self& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= v.w;
		return *this;
	}

	IC SelfRef sub(T s)
	{
		x -= s;
		y -= s;
		z -= s;
		w -= s;
		return *this;
	}

	IC SelfRef sub(const Self& a, const Self& v)
	{
		x = a.x - v.x;
		y = a.y - v.y;
		z = a.z - v.z;
		w = a.w - v.w;
		return *this;
	}

	IC SelfRef sub(const Self& a, T s)
	{
		x = a.x - s;
		y = a.y - s;
		z = a.z - s;
		w = a.w - s;
		return *this;
	}

	IC SelfRef mul(T _x, T _y, T _z, T _w = 1)
	{
		x *= _x;
		y *= _y;
		z *= _z;
		w *= _w;
		return *this;
	}

	IC SelfRef mul(const Self& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		w *= v.w;
		return *this;
	}

	IC SelfRef mul(T s)
	{
		x *= s;
		y *= s;
		z *= s;
		w *= s;
		return *this;
	}

	IC SelfRef mul(const Self& a, const Self& v)
	{
		x = a.x * v.x;
		y = a.y * v.y;
		z = a.z * v.z;
		w = a.w * v.w;
		return *this;
	}

	IC SelfRef mul(const Self& a, T s)
	{
		x = a.x * s;
		y = a.y * s;
		z = a.z * s;
		w = a.w * s;
		return *this;
	}

	IC SelfRef div(T _x, T _y, T _z, T _w = 1)
	{
		x /= _x;
		y /= _y;
		z /= _z;
		w /= _w;
		return *this;
	}

	IC SelfRef div(const Self& v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
		w /= v.w;
		return *this;
	}

	IC SelfRef div(T s)
	{
		x /= s;
		y /= s;
		z /= s;
		w /= s;
		return *this;
	}

	IC SelfRef div(const Self& a, const Self& v)
	{
		x = a.x / v.x;
		y = a.y / v.y;
		z = a.z / v.z;
		w = a.w / v.w;
		return *this;
	}

	IC SelfRef div(const Self& a, T s)
	{
		x = a.x / s;
		y = a.y / s;
		z = a.z / s;
		w = a.w / s;
		return *this;
	}

	// Clamp vector3
	IC SelfRef clamp(const Self& min, const Self& max)
	{
		::clamp(x, min.x, max.x);
		::clamp(y, min.y, max.y);
		::clamp(z, min.z, max.z);
		::clamp(w, min.w, max.w);
		return *this;
	}

	IC SelfRef clamp(const Self& _v)
	{
		Self v;
		v.x = _abs(_v.x);
		v.y = _abs(_v.y);
		v.z = _abs(_v.z);
		v.z = _abs(_v.w);
		::clamp(x, -v.x, v.x);
		::clamp(y, -v.y, v.y);
		::clamp(z, -v.z, v.z);
		::clamp(w, -v.w, v.w);
		return *this;
	}

	IC BOOL similar(const Self& v, T E = EPS_L)
	{
		return _abs(x - v.x) < E && _abs(y - v.y) < E && _abs(z - v.z) < E && _abs(w - v.w) < E;
	};

	IC T magnitude_sqr() { return x * x + y * y + z * z + w * w; }
	IC T magnitude() { return _sqrt(magnitude_sqr()); }
	IC SelfRef normalize() { return mul(1 / magnitude()); }

	IC SelfRef normalize_as_plane() { return mul(1 / _sqrt(x * x + y * y + z * z)); }

	IC SelfRef lerp(const Self& p1, const Self& p2, T t)
	{
		T invt = 1.f - t;
		x = p1.x * invt + p2.x * t;
		y = p1.y * invt + p2.y * t;
		z = p1.z * invt + p2.z * t;
		w = p1.w * invt + p2.w * t;
		return *this;
	}
};

typedef _vector4<float> Fvector4;
typedef _vector4<double> Dvector4;
typedef _vector4<s32> Ivector4;
#ifndef __BORLANDC__
typedef __declspec(align(16)) _vector4<float> Fvector4a;
typedef __declspec(align(16)) _vector4<double> Dvector4a;
typedef __declspec(align(16)) _vector4<s32> Ivector4a;
#endif

template <class T>
BOOL _valid(const _vector4<T>& v) { return _valid((T)v.x) && _valid((T)v.y) && _valid((T)v.z) && _valid((T)v.w); }

#endif
