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

namespace ogle {
/**
 * \brief Provides input to shader programs.
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
  /**
   * Factory function.
   * @param name the input name.
   * @param dataType the input data type.
   * @param valsPerElement number of values per element.
   * @return the created ShaderInput.
   */
  static ref_ptr<ShaderInput> create(
      const string &name, GLenum dataType, GLuint valsPerElement);

  ShaderInput(
      const string &name,
      GLenum dataType,
      GLuint dataTypeBytes,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
  virtual ~ShaderInput() {}

  /**
   * Read ShaderInput.
   */
  virtual istream& operator<<(istream &in) = 0;
  /**
   * Write ShaderInput.
   */
  virtual ostream& operator>>(ostream &out) const = 0;

  /**
   * Returns true if this input is a vertex attribute or
   * an instanced attribute.
   */
  GLboolean isVertexAttribute() const;

  /**
   * Constants can not change the value during the lifetime
   * of the shader program.
   */
  void set_isConstant(GLboolean isConstant);
  /**
   * Constants can not change the value during the lifetime
   * of the shader program.
   */
  GLboolean isConstant() const;

  /**
   * Uniforms with a single array element will appear
   * with [1] in the generated shader if forceArray is true.
   * Note: attributes can not be arrays.
   */
  void set_forceArray(GLboolean forceArray);
  /**
   * Uniforms with a single array element will appear
   * with [1] in the generated shader if forceArray is true.
   * Note: attributes can not be arrays.
   */
  GLboolean forceArray() const;

  /**
   * @param data the input data.
   */
  void setUniformDataUntyped(byte *data);
  /**
   * Binds vertex attribute for active buffer to the
   * given shader location.
   */
  void enableAttribute(GLint loc) const;
  /**
   * Binds uniform to the given shader location (glUniform*).
   */
  void enableUniform(GLint loc) const;

  void enableUniform1f(GLint loc) const;
  void enableUniform2f(GLint loc) const;
  void enableUniform3f(GLint loc) const;
  void enableUniform4f(GLint loc) const;
  void enableUniform1i(GLint loc) const;
  void enableUniform2i(GLint loc) const;
  void enableUniform3i(GLint loc) const;
  void enableUniform4i(GLint loc) const;
  void enableUniform1d(GLint loc) const;
  void enableUniform2d(GLint loc) const;
  void enableUniform3d(GLint loc) const;
  void enableUniform4d(GLint loc) const;
  void enableUniform1ui(GLint loc) const;
  void enableUniform2ui(GLint loc) const;
  void enableUniform3ui(GLint loc) const;
  void enableUniform4ui(GLint loc) const;
  void enableUniformMat3(GLint loc) const;
  void enableUniformMat4(GLint loc) const;

protected:
  GLboolean isConstant_;
  GLboolean forceArray_;

  void (VertexAttribute::*enableAttribute_)(GLint loc) const;
  void (ShaderInput::*enableUniform_)(GLint loc) const;
};

/////////////

/**
 * \brief Provides float input to shader programs.
 */
class ShaderInputf : public ShaderInput
{
public:
  ShaderInputf(
      const string &name,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
};
/**
 * \brief Provides 1D float input to shader programs.
 */
class ShaderInput1f : public ShaderInputf
{
public:
  ShaderInput1f(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const GLfloat &data);
};
/**
 * \brief Provides 2D float input to shader programs.
 */
class ShaderInput2f : public ShaderInputf
{
public:
  ShaderInput2f(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec2f &data);
};
/**
 * \brief Provides 3D float input to shader programs.
 */
class ShaderInput3f : public ShaderInputf
{
public:
  ShaderInput3f(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec3f &data);
};
/**
 * \brief Provides 4D float input to shader programs.
 */
class ShaderInput4f : public ShaderInputf
{
public:
  ShaderInput4f(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec4f &data);
};

/**
 * \brief Provides double input to shader programs.
 */
class ShaderInputd : public ShaderInput
{
public:
  ShaderInputd(
      const string &name,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
};
/**
 * \brief Provides 1D double input to shader programs.
 */
class ShaderInput1d : public ShaderInputd
{
public:
  ShaderInput1d(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const GLdouble &data);
};
/**
 * \brief Provides 2D double input to shader programs.
 */
class ShaderInput2d : public ShaderInputd
{
public:
  ShaderInput2d(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec2d &data);
};
/**
 * \brief Provides 3D double input to shader programs.
 */
class ShaderInput3d : public ShaderInputd
{
public:
  ShaderInput3d(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec3d &data);
};
/**
 * \brief Provides 4D double input to shader programs.
 */
class ShaderInput4d : public ShaderInputd
{
public:
  ShaderInput4d(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec4d &data);
};

/**
 * \brief Provides int input to shader programs.
 */
class ShaderInputi : public ShaderInput
{
public:
  ShaderInputi(
      const string &name,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
};
/**
 * \brief Provides 1D int input to shader programs.
 */
class ShaderInput1i : public ShaderInputi
{
public:
  ShaderInput1i(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const GLint &data);
};
/**
 * \brief Provides 2D int input to shader programs.
 */
class ShaderInput2i : public ShaderInputi
{
public:
  ShaderInput2i(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec2i &data);
};
/**
 * \brief Provides 3D int input to shader programs.
 */
class ShaderInput3i : public ShaderInputi
{
public:
  ShaderInput3i(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec3i &data);
};
/**
 * \brief Provides 4D int input to shader programs.
 */
class ShaderInput4i : public ShaderInputi
{
public:
  ShaderInput4i(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec4i &data);
};

/**
 * \brief Provides unsigned int input to shader programs.
 */
class ShaderInputui : public ShaderInput
{
public:
  ShaderInputui(
      const string &name,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
};
/**
 * \brief Provides 1D unsigned int input to shader programs.
 */
class ShaderInput1ui : public ShaderInputui
{
public:
  ShaderInput1ui(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const GLuint &data);
};
/**
 * \brief Provides 2D unsigned int input to shader programs.
 */
class ShaderInput2ui : public ShaderInputui
{
public:
  ShaderInput2ui(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec2ui &data);
};
/**
 * \brief Provides 3D unsigned int input to shader programs.
 */
class ShaderInput3ui : public ShaderInputui
{
public:
  ShaderInput3ui(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec3ui &data);
};
/**
 * \brief Provides 4D unsigned int input to shader programs.
 */
class ShaderInput4ui : public ShaderInputui
{
public:
  ShaderInput4ui(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Vec4ui &data);
};

/**
 * \brief Provides float matrix input to shader programs.
 */
class ShaderInputMat : public ShaderInputf
{
public:
  ShaderInputMat(
      const string &name,
      GLuint valsPerElement,
      GLuint elementCount,
      GLboolean normalize);
};
/**
 * \brief Provides 3x3 matrix input to shader programs.
 */
class ShaderInputMat3 : public ShaderInputMat
{
public:
  ShaderInputMat3(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Mat3f &data);
};
/**
 * \brief Provides 4x4 matrix input to shader programs.
 */
class ShaderInputMat4 : public ShaderInputMat
{
public:
  ShaderInputMat4(
      const string &name,
      GLuint elementCount=1,
      GLboolean normalize=GL_FALSE);
  istream& operator<<(istream &in);
  ostream& operator>>(ostream &out) const;
  /**
   * @param data the input data.
   */
  void setUniformData(const Mat4f &data);
};

} // end ogle namespace

#endif /* SHADER_INPUT_H_ */
