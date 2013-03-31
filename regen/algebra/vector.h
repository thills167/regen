/*
 * vector.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef ___VECTOR_H_
#define ___VECTOR_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <iostream>
#include <cmath>
#include <cassert>
#include <list>
using namespace std;

namespace regen {
/**
 * \brief A 2D vector.
 */
template<typename T> class Vec2 {
public:
  T x; /**< the x component. **/
  T y; /**< the y component. **/

  Vec2() : x(0), y(0) {}
  /** Set-component constructor. */
  Vec2(T _x, T _y) : x(_x), y(_y) {}
  /** @param _x value that is applied to all components. */
  Vec2(T _x) : x(_x), y(_x) {}
  /** copy constructor. */
  Vec2(const Vec2 &b) : x(b.x), y(b.y) {}

  /** copy operator. */
  inline void operator=(const Vec2 &b)
  { x=b.x; y=b.y; }
  /**
   * @param b another vector
   * @return true if all values are equal
   */
  inline bool operator==(const Vec2 &b) const
  { return x==b.x && y==b.y; }
  /**
   * @param b another vector
   * @return false if all values are equal
   */
  inline bool operator!=(const Vec2 &b) const
  { return !operator==(b); }

  /**
   * @return vector with each component negated.
   */
  inline Vec2 operator-() const
  { return Vec2(-x,-y); }

  /**
   * @param b vector to add.
   * @return the vector sum.
   */
  inline Vec2 operator+(const Vec2 &b) const
  { return Vec2(x+b.x, y+b.y ); }
  /**
   * @param b vector to subtract.
   * @return the vector difference.
   */
  inline Vec2 operator-(const Vec2 &b) const
  { return Vec2(x-b.x, y-b.y ); }
  /**
   * @param b vector to multiply.
   * @return the vector product.
   */
  inline Vec2 operator*(const Vec2 &b) const
  { return Vec2(x*b.x, y*b.y ); }
  /**
   * @param b vector to divide.
   * @return the vector product.
   */
  inline Vec2 operator/(const Vec2 &b) const
  { return Vec2(x/b.x, y/b.y ); }
  /**
   * @param b scalar to multiply.
   * @return the vector product.
   */
  inline Vec2 operator*(const T &b) const
  { return Vec2(x*b, y*b ); }
  /**
   * @param b scalar to divide.
   * @return the vector product.
   */
  inline Vec2 operator/(const T &b) const
  { return Vec2(x/b, y/b ); }
  /**
   * @param b vector to add.
   */
  inline void operator+=(const Vec2 &b)
  { x+=b.x; y+=b.y; }
  /**
   * @param b vector to subtract.
   */
  inline void operator-=(const Vec2 &b)
  { x-=b.x; y-=b.y; }
  /**
   * @param b vector to multiply.
   */
  inline void operator*=(const Vec2 &b)
  { x*=b.x; y*=b.y; }
  /**
   * @param b vector to divide.
   */
  inline void operator/=(const Vec2 &b)
  { x/=b.x; y/=b.y; } \
  /**
   * @param b scalar to multiply.
   */
  inline void operator*=(const T &b)
  { x*=b; y*=b; }
  /**
   * @param b scalar to divide.
   */
  inline void operator/=(const T &b)
  { x/=b; y/=b; }

  /**
   * @return vector length.
   */
  inline GLfloat length() const
  { return sqrt(pow(x,2) + pow(y,2)); }
  /**
   * Normalize this vector.
   */
  inline void normalize()
  { *this /= length(); }
};
/**
 * \brief A 2D vector of float values.
 */
typedef Vec2<GLfloat> Vec2f;
/**
 * \brief A 2D vector of double values.
 */
typedef Vec2<GLdouble> Vec2d;
/**
 * \brief A 2D vector of int values.
 */
typedef Vec2<GLint> Vec2i;
/**
 * \brief A 2D vector of unsigned int values.
 */
typedef Vec2<GLuint> Vec2ui;
/**
 * \brief A 2D vector of bool values.
 */
typedef Vec2<GLboolean> Vec2b;

/**
 * \brief A 3D vector.
 */
template<typename T> class Vec3 {
public:
  T x; /**< the x component. **/
  T y; /**< the y component. **/
  T z; /**< the z component. **/

  Vec3() : x(0), y(0), z(0) {}
  /** Set-component constructor. */
  Vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
  /** @param _x value that is applied to all components. */
  Vec3(T _x) : x(_x), y(_x), z(_x) {}
  /** copy constructor. */
  Vec3(const Vec3 &b) : x(b.x), y(b.y), z(b.z) {}

  /** copy operator. */
  inline void operator=(const Vec3 &b)
  { x=b.x; y=b.y; z=b.z; }
  /**
   * @param b another vector
   * @return true if all values are equal
   */
  inline bool operator==(const Vec3 &b) const
  { return x==b.x && y==b.y && z==b.z; }
  /**
   * @param b another vector
   * @return false if all values are equal
   */
  inline bool operator!=(const Vec3 &b) const
  { return !operator==(b); }

  /**
   * @return vector with each component negated.
   */
  inline Vec3 operator-() const
  { return Vec3(-x,-y,-z); }

  /**
   * @param b vector to add.
   * @return the vector sum.
   */
  inline Vec3 operator+(const Vec3 &b) const
  { return Vec3(x+b.x, y+b.y, z+b.z ); }
  /**
   * @param b vector to subtract.
   * @return the vector difference.
   */
  inline Vec3 operator-(const Vec3 &b) const
  { return Vec3(x-b.x, y-b.y, z-b.z ); }
  /**
   * @param b vector to multiply.
   * @return the vector product.
   */
  inline Vec3 operator*(const Vec3 &b) const
  { return Vec3(x*b.x, y*b.y, z*b.z ); }
  /**
   * @param b vector to divide.
   * @return the vector product.
   */
  inline Vec3 operator/(const Vec3 &b) const
  { return Vec3(x/b.x, y/b.y, z/b.z ); }
  /**
   * @param b scalar to multiply.
   * @return the vector product.
   */
  inline Vec3 operator*(const T &b) const
  { return Vec3(x*b, y*b, z*b ); }
  /**
   * @param b scalar to divide.
   * @return the vector product.
   */
  inline Vec3 operator/(const T &b) const
  { return Vec3(x/b, y/b, z/b ); }
  /**
   * @param b vector to add.
   */
  inline void operator+=(const Vec3 &b)
  { x+=b.x; y+=b.y; z+=b.z; }
  /**
   * @param b vector to subtract.
   */
  inline void operator-=(const Vec3 &b)
  { x-=b.x; y-=b.y; z-=b.z; }
  /**
   * @param b vector to multiply.
   */
  inline void operator*=(const Vec3 &b)
  { x*=b.x; y*=b.y; z*=b.z; }
  /**
   * @param b vector to divide.
   */
  inline void operator/=(const Vec3 &b)
  { x/=b.x; y/=b.y; z/=b.z; }
  /**
   * @param b scalar to multiply.
   */
  inline void operator*=(const T &b)
  { x*=b; y*=b; z*=b; }
  /**
   * @param b scalar to divide.
   */
  inline void operator/=(const T &b)
  { x/=b; y/=b; z/=b; }

  /**
   * @return vector length.
   */
  inline GLfloat length() const
  { return sqrt(pow(x,2) + pow(y,2) + pow(z,2)); }
  /**
   * Normalize this vector.
   */
  inline void normalize()
  { *this /= length(); }

  /**
   * Computes the cross product between two vectors.
   * The result vector is perpendicular to both of the vectors being multiplied.
   * @param b another vector.
   * @return the cross product.
   * @see http://en.wikipedia.org/wiki/Cross_product
   */
  inline Vec3 cross(const Vec3 &b) const
  {
    return Vec3(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
  }
  /**
   * Computes the dot product between two vectors.
   * The dot product is equal to the acos of the angle
   * between those vectors.
   * @param b another vector.
   * @return the dot product.
   */
  inline T dot(const Vec3 &b) const
  {
    return x*b.x + y*b.y + z*b.z;
  }

  /**
   * Rotates this vector around x/y/z axis.
   * @param angle
   * @param x_
   * @param y_
   * @param z_
   */
  inline void rotate(GLfloat angle, GLfloat x_, GLfloat y_, GLfloat z_)
  {
    GLfloat c = cos(angle);
    GLfloat s = sin(angle);
    Vec3<GLfloat> rotated(
            (x_*x_*(1-c) + c)    * x
          + (x_*y_*(1-c) - z_*s) * y
          + (x_*z_*(1-c) + y_*s) * z,
            (y_*x_*(1-c) + z_*s) * x
          + (y_*y_*(1-c) + c)    * y
          + (y_*z_*(1-c) - x_*s) * z,
            (x_*z_*(1-c) - y_*s) * x
          + (y_*z_*(1-c) + x_*s) * y
          + (z_*z_*(1-c) + c)    * z
    );
    x = rotated.x;
    y = rotated.y;
    z = rotated.z;
  }

  /**
   * Compares vectors components.
   * @return true if all components are nearly equal.
   */
  inline GLboolean isApprox(const Vec3 &b, T delta) const
  {
    return abs(x-b.x)<delta && abs(y-b.y)<delta && abs(z-b.z)<delta;
  }

  /**
   * @return static zero vector.
   */
  static const Vec3& zero()
  {
    static Vec3 zero_(0);
    return zero_;
  }
  /**
   * @return static one vector.
   */
  static const Vec3& one()
  {
    static Vec3 one_(1);
    return one_;
  }
  /**
   * @return static up vector.
   */
  static const Vec3& up()
  {
    static Vec3 up_(0,1,0);
    return up_;
  }
};
/**
 * \brief A 3D vector of float values.
 */
typedef Vec3<GLfloat> Vec3f;
/**
 * \brief A 3D vector of double values.
 */
typedef Vec3<GLdouble> Vec3d;
/**
 * \brief A 3D vector of int values.
 */
typedef Vec3<GLint> Vec3i;
/**
 * \brief A 3D vector of unsigned int values.
 */
typedef Vec3<GLuint> Vec3ui;
/**
 * \brief A 3D vector of bool values.
 */
typedef Vec3<GLboolean> Vec3b;

/**
 * \brief A 4D vector.
 */
template<typename T> class Vec4 {
public:
  T x; /**< the x component. **/
  T y; /**< the y component. **/
  T z; /**< the z component. **/
  T w; /**< the w component. **/

  Vec4() : x(0), y(0), z(0), w(0) {}
  /** Set-component constructor. */
  Vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
  /** @param _x value that is applied to all components. */
  Vec4(T _x) : x(_x), y(_x), z(_x), w(_x) {}
  /** copy constructor. */
  Vec4(const Vec4 &b) : x(b.x), y(b.y), z(b.z), w(b.w) {}

  /** copy operator. */
  inline void operator=(const Vec4 &b)
  { x=b.x; y=b.y; z=b.z; w=b.w; }
  /**
   * @param b another vector
   * @return true if all values are equal
   */
  inline bool operator==(const Vec4 &b) const
  { return x==b.x && y==b.y && z==b.z && w==b.w; }
  /**
   * @param b another vector
   * @return false if all values are equal
   */
  inline bool operator!=(const Vec4 &b) const
  { return !operator==(b); }

  /**
   * @return vector with each component negated.
   */
  inline Vec4 operator-() const
  { return Vec4(-x,-y,-z,-w); }

  /**
   * @param b vector to add.
   * @return the vector sum.
   */
  inline Vec4 operator+(const Vec4 &b) const
  { return Vec4(x+b.x, y+b.y, z+b.z, w+b.w ); }
  /**
   * @param b vector to subtract.
   * @return the vector sum.
   */
  inline Vec4 operator-(const Vec4 &b) const
  { return Vec4(x-b.x, y-b.y, z-b.z, w-b.w ); }
  /**
   * @param b vector to multiply.
   * @return the vector product.
   */
  inline Vec4 operator*(const Vec4 &b) const
  { return Vec4(x*b.x, y*b.y, z*b.z, w*b.w ); }
  /**
   * @param b vector to divide.
   * @return the vector product.
   */
  inline Vec4 operator/(const Vec4 &b) const
  { return Vec4(x/b.x, y/b.y, z/b.z, w/b.w ); }
  /**
   * @param b scalar to multiply.
   * @return the vector-scalar product.
   */
  inline Vec4 operator*(const T &b) const
  { return Vec4(x*b, y*b, z*b, w*b ); }
  /**
   * @param b scalar to divide.
   * @return the vector-scalar product.
   */
  inline Vec4 operator/(const T &b) const
  { return Vec4(x/b, y/b, z/b, w/b ); }
  /**
   * @param b vector to add.
   */
  inline void operator+=(const Vec4 &b)
  { x+=b.x; y+=b.y; z+=b.z; w+=b.w; }
  /**
   * @param b vector to subtract.
   */
  inline void operator-=(const Vec4 &b)
  { x-=b.x; y-=b.y; z-=b.z; w-=b.w; }
  /**
   * @param b vector to multiply.
   */
  inline void operator*=(const Vec4 &b)
  { x*=b.x; y*=b.y; z*=b.z; w*=b.w; }
  /**
   * @param b vector to divide.
   */
  inline void operator/=(const Vec4 &b)
  { x/=b.x; y/=b.y; z/=b.z; w/=b.w; }
  /**
   * @param b scalar to multiply.
   */
  inline void operator*=(const T &b)
  { x*=b; y*=b; z*=b; w*=b; }
  /**
   * @param b scalar to divide.
   */
  inline void operator/=(const T &b)
  { x/=b; y/=b; z/=b; w/=b; }

  /**
   * @return Vec4f casted to Vec3f.
   */
  inline Vec3<T>& toVec3()
  {
    return *((Vec3<T>*)this);
  }
  /**
   * Compares vectors components.
   * @return true if all components are nearly equal.
   */
  inline GLboolean isApprox(const Vec4 &b, T delta) const
  {
    return abs(x-b.x)<delta && abs(y-b.y)<delta && abs(z-b.z)<delta && abs(w-b.w)<delta;
  }
};
/**
 * \brief A 4D vector of float values.
 */
typedef Vec4<GLfloat> Vec4f;
/**
 * \brief A 4D vector of double values.
 */
typedef Vec4<GLdouble> Vec4d;
/**
 * \brief A 3D vector of int values.
 */
typedef Vec4<GLint> Vec4i;
/**
 * \brief A 3D vector of unsigned int values.
 */
typedef Vec4<GLuint> Vec4ui;
/**
 * \brief A 3D vector of bool values.
 */
typedef Vec4<GLboolean> Vec4b;

/**
 * \brief An n-dimensional vector of float values.
 */
struct VecXf {
  GLfloat *v; /**< Components. **/
  GLuint size; /**< Number of components. **/

  VecXf()
  : v(NULL), size(0u) {}
  /**
   * Set-component constructor.
   */
  VecXf(GLfloat *_v, GLuint _size)
  : v(_v), size(_size) {}

  /**
   * Compares vectors components.
   * @return true if all components are nearly equal.
   */
  inline GLboolean isApprox(const VecXf &b, GLfloat delta=1e-6)
  {
    if(size == b.size) return GL_FALSE;
    for(GLuint i=0; i<size; ++i) {
      if(abs(v[i]-b.v[i]) > delta) return GL_FALSE;
    }
    return true;
  }
};

/**
 * \brief A n-dimensional vector of bool values.
 */
struct VecXb {
  GLboolean *v; /**< Components. **/
  GLuint size; /**< Number of components. **/

  VecXb()
  : v(NULL), size(0u) {}
  /**
   * Set-component constructor.
   */
  VecXb(GLboolean *_v, GLuint _size)
  : v(_v), size(_size) {}
};

// TODO: include in defines above

ostream& operator<<(ostream& os, const Vec2f& v);
ostream& operator<<(ostream& os, const Vec3f& v);
ostream& operator<<(ostream& os, const Vec4f& v);
ostream& operator<<(ostream& os, const Vec2d& v);
ostream& operator<<(ostream& os, const Vec3d& v);
ostream& operator<<(ostream& os, const Vec4d& v);
ostream& operator<<(ostream& os, const Vec2i& v);
ostream& operator<<(ostream& os, const Vec3i& v);
ostream& operator<<(ostream& os, const Vec4i& v);
ostream& operator<<(ostream& os, const Vec2ui& v);
ostream& operator<<(ostream& os, const Vec3ui& v);
ostream& operator<<(ostream& os, const Vec4ui& v);

istream& operator>>(istream& in, Vec2f &v);
istream& operator>>(istream& in, Vec3f &v);
istream& operator>>(istream& in, Vec4f &v);
istream& operator>>(istream& in, Vec2d &v);
istream& operator>>(istream& in, Vec3d &v);
istream& operator>>(istream& in, Vec4d &v);
istream& operator>>(istream& in, Vec2i &v);
istream& operator>>(istream& in, Vec3i &v);
istream& operator>>(istream& in, Vec4i &v);
istream& operator>>(istream& in, Vec2ui &v);
istream& operator>>(istream& in, Vec3ui &v);
istream& operator>>(istream& in, Vec4ui &v);

inline bool isApprox(const float &a, const float &b, float delta=1e-6)
{
  return abs(a-b)<=delta;
}

Vec4f calculateTangent(Vec3f *vertices, Vec2f *texco, Vec3f &normal);

#define DEGREE_TO_RAD 57.29577951308232

GLdouble mix(GLdouble x, GLdouble y, GLdouble a);
GLfloat clamp(GLfloat x, GLfloat min, GLfloat max);

} // namespace

#endif /* ___VECTOR_H_ */
