/*
 * shader-input.h
 *
 *  Created on: 15.08.2012
 *      Author: daniel
 */

#ifndef SHADER_INPUT_H_
#define SHADER_INPUT_H_

#include <string>
using namespace std;

#include <ogle/gl-types/vertex-attribute.h>

enum FragmentInterpolation {
  // means that there is no interpolation.
  // The value given to the fragment shader is based on the provoking vertex conventions
  FRAGMENT_INTERPOLATION_FLAT,
  // means that there will be linear interpolation in window-space.
  FRAGMENT_INTERPOLATION_NOPERSPECTIVE,
  // the default, means to do perspective-correct interpolation.
  FRAGMENT_INTERPOLATION_SMOOTH,
  // only matters when multisampling. If this qualifier is not present,
  // then the value is interpolated to the pixel's center, anywhere in the pixel,
  // or to one of the pixel's samples. This sample may lie outside of the actual
  // primitive being rendered, since a primitive can cover only part of a pixel's area.
  // The centroid qualifier is used to prevent this;
  // the interpolation point must fall within both the pixel's area and the primitive's area.
  FRAGMENT_INTERPOLATION_CENTROID,
  FRAGMENT_INTERPOLATION_DEFAULT
};


/**
 * Provides input to shader programs.
 *
 * Inputs can be constants, uniforms, instanced attributes
 * and per vertex attributes.
 *
 * A constant is a global GLSL variable declared with the "constant"
 * storage qualifier. These are compiled into generated shaders,
 * if the user changes the value it will have no influence until
 * the shader is regenerated.
 *
 * A uniform is a global GLSL variable declared with the "uniform"
 * storage qualifier. These act as parameters that the user
 * of a shader program can pass to that program.
 * They are stored in a program object.
 * Uniforms are so named because they do not change from
 * one execution of a shader program to the next within
 * a particular rendering call.
 *
 * For information about vertex attributes see the documentation of
 * the base class.
 */
class ShaderInput : public VertexAttribute
{
public:
  ShaderInput(
      const string &name,
      GLenum dataType,
      GLuint dataTypeBytes,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
  virtual ~ShaderInput() {}

  virtual istream& operator<<(istream &in) = 0;
  virtual ostream& operator>>(ostream &out) const = 0;

  GLboolean isVertexAttribute() const;

  void set_isConstant(GLboolean isConstant);
  GLboolean isConstant() const;

  void set_interpolationMode(FragmentInterpolation fragmentInterpolation);
  FragmentInterpolation interpolationMode();

  void set_forceArray(GLboolean forceArray);
  GLboolean forceArray();

  /**
   * Binds vertex attribute for active buffer to the
   * given shader location.
   */
  virtual void enableAttribute(GLint loc) const;
  /**
   * Binds uniform to the given shader location (glUniform*).
   */
  virtual void enableUniform(GLint loc) const = 0;

protected:
  GLboolean isConstant_;
  GLboolean forceArray_;
  FragmentInterpolation fragmentInterpolation_;
};

/////////////

class ShaderInputf : public ShaderInput
{
public:
  ShaderInputf(
      const string &name,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
};
class ShaderInput1f : public ShaderInputf
{
public:
  ShaderInput1f(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const GLfloat &data);
};
class ShaderInput2f : public ShaderInputf
{
public:
  ShaderInput2f(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec2f &data);
};
class ShaderInput3f : public ShaderInputf
{
public:
  ShaderInput3f(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec3f &data);
};
class ShaderInput4f : public ShaderInputf
{
public:
  ShaderInput4f(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec4f &data);
};

class ShaderInputd : public ShaderInput
{
public:
  ShaderInputd(
      const string &name,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
};
class ShaderInput1d : public ShaderInputd
{
public:
  ShaderInput1d(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const GLdouble &data);
};
class ShaderInput2d : public ShaderInputd
{
public:
  ShaderInput2d(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec2d &data);
};
class ShaderInput3d : public ShaderInputd
{
public:
  ShaderInput3d(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec3d &data);
};
class ShaderInput4d : public ShaderInputd
{
public:
  ShaderInput4d(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec4d &data);
};

class ShaderInputi : public ShaderInput
{
public:
  ShaderInputi(
      const string &name,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
  virtual void enableAttribute(GLint loc) const;
};
class ShaderInput1i : public ShaderInputi
{
public:
  ShaderInput1i(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const GLint &data);
};
class ShaderInput2i : public ShaderInputi
{
public:
  ShaderInput2i(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec2i &data);
};
class ShaderInput3i : public ShaderInputi
{
public:
  ShaderInput3i(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec3i &data);
};
class ShaderInput4i : public ShaderInputi
{
public:
  ShaderInput4i(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec4i &data);
};

class ShaderInputui : public ShaderInput
{
public:
  ShaderInputui(
      const string &name,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
  virtual void enableAttribute(GLint loc) const;
};
class ShaderInput1ui : public ShaderInputui
{
public:
  ShaderInput1ui(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const GLuint &data);
};
class ShaderInput2ui : public ShaderInputui
{
public:
  ShaderInput2ui(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec2ui &data);
};
class ShaderInput3ui : public ShaderInputui
{
public:
  ShaderInput3ui(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec3ui &data);
};
class ShaderInput4ui : public ShaderInputui
{
public:
  ShaderInput4ui(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Vec4ui &data);
};

class ShaderInputMat : public ShaderInputf
{
public:
  ShaderInputMat(
      const string &name,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
  void set_transpose(GLboolean transpose);
  GLboolean transpose() const;
protected:
  GLboolean transpose_;
};
class ShaderInputMat3 : public ShaderInputMat
{
public:
  ShaderInputMat3(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableAttribute(GLint loc) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Mat3f &data);
};
class ShaderInputMat4 : public ShaderInputMat
{
public:
  ShaderInputMat4(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  virtual void enableAttribute(GLint loc) const;
  virtual void enableUniform(GLint loc) const;
  void setUniformData(const Mat4f &data);
};

///////////

class PositionShaderInput : public ShaderInput3f
{
public:
  PositionShaderInput(GLboolean normalize=GL_FALSE);
};

class TangentShaderInput : public ShaderInput4f
{
public:
  TangentShaderInput(GLboolean normalize=GL_FALSE);
};

class NormalShaderInput : public ShaderInput3f
{
public:
  NormalShaderInput(GLboolean normalize=GL_FALSE);
};

class TexcoShaderInput : public ShaderInputf
{
public:
  TexcoShaderInput(
      GLuint channel,
      GLuint valsPerElement=3,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  virtual istream& operator<<(istream &in);
  virtual ostream& operator>>(ostream &out) const;
  GLuint channel() const;
  virtual void enableUniform(GLint loc) const;
protected:
  GLuint channel_;
};

#endif /* SHADER_INPUT_H_ */
