/*
 * shader.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef _SHADER_H_
#define _SHADER_H_

#include <map>
#include <set>
using namespace std;

#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/shader-input.h>

/**
 * Maps vertex attribute to shader location.
 */
struct ShaderAttributeLocation
{
  ref_ptr<VertexAttribute> att;
  GLint location;
  ShaderAttributeLocation(const ref_ptr<VertexAttribute> &_att, GLint _location)
  : att(_att), location(_location) {}
};
/**
 * Maps input to shader location.
 */
struct ShaderInputLocation
{
  ref_ptr<ShaderInput> input;
  GLint location;
  ShaderInputLocation(const ref_ptr<ShaderInput> &_input, GLint _location)
  : input(_input), location(_location) {}
};
/**
 * Maps texture to shader location.
 */
struct ShaderTextureLocation
{
  GLint location;
  GLint *channel;
  ShaderTextureLocation(GLint *_channel, GLint _location)
  : location(_location), channel(_channel) {}
};

/**
 * Encapsulates a GLSL program, helps
 * compiling and linking together the
 * shader stages.
 */
class Shader
{
public:
  /**
   * Create a new shader or return an identical shader that
   * was loaded before.
   */
  static ref_ptr<Shader> create(
      const map<string, string> &shaderConfig,
      const map<string,string> &functions,
      const map<string, ref_ptr<ShaderInput> > &specifiedInput,
      map<GLenum, string> &code);
  static ref_ptr<Shader> create(
      const map<string, string> &shaderConfig,
      const map<string,string> &functions,
      map<GLenum, string> &code);
  /**
   * Loads stages and prepends a header and a body to the code.
   * #include directives are resolved and preProcessCode() is called.
   */
  static void load(
      const string &shaderHeader,
      map<GLenum,string> &shaderCode,
      const map<string,string> &functions,
      const map<string, ref_ptr<ShaderInput> > &specifiedInput);
  /**
   * Loads stage and prepends a header and a body to the code.
   * #include directives are resolved.
   */
  static string load(const string &shaderCode,
      const map<string,string> &functions=map<string,string>());

  static void printLog(
      GLuint shader,
      GLenum shaderType,
      const char *shaderCode,
      GLboolean success);

  /////////////

  /**
   * Share GL resource with other shader.
   * Each shader has an individual configuration only GL resources
   * are shared.
   */
  Shader(Shader&);
  /**
   * Construct pre-compiled shader.
   * link() must be called to use this shader.
   * Note: make sure IO names in stages match each other.
   */
  Shader(
      const map<GLenum, string> &shaderCode,
      map<GLenum, ref_ptr<GLuint> > &shaderObjects);
  /**
   * Create a new shader with given stage map.
   * compile() and link() must be called to use this shader.
   */
  Shader(const map<GLenum, string> &shaderNames);
  ~Shader();

  /**
   * Compiles and attaches shader stages.
   */
  GLboolean compile();
  /**
   * Link together previous compiled stages.
   * Note: For MRT you must call setOutputs before and for
   * transform feedback you must call setTransformFeedback before.
   */
  GLboolean link();

  /**
   * The program object.
   */
  GLint id() const;

  /**
   * If instanced attributes were added to the shader this will
   * return the number of instances these attributes expect.
   * Note: The number of instances must be eual for all instanced
   * attributes.
   */
  GLuint numInstances() const;

  /**
   * Returns true if the given name is a valid vertex attribute name.
   */
  GLboolean isAttribute(const string &name) const;
  /**
   * Returns the locations for a given vertex attribute name or -1 if the name is not known.
   */
  GLint attributeLocation(const string &name);

  /**
   * Returns true if the given name is a valid uniform name.
   */
  GLboolean isUniform(const string &name) const;
  /**
   * Returns the location for a given uniform name or -1 if the name is not known.
   */
  GLint uniformLocation(const string &name);
  /**
   * Returns true if the given name is a valid uniform name and
   * the uniform has some data set (no null pointer data).
   */
  GLboolean hasUniformData(const string &name) const;

  /**
   * Returns true if the given name is a valid sampler name.
   */
  GLboolean isSampler(const string &name) const;
  /**
   * Returns the location for a given sampler name or -1 if the name is not known.
   */
  GLint samplerLocation(const string &name);

  /**
   * Returns map of inputs for this shader.
   * Each attribute and uniform will appear in this map after the
   * program was linked with a NULL data pointer.
   * You can overwrite these with setInput or you can allocate data
   * for the inputs as returned by this function.
   */
  const map<string, ref_ptr<ShaderInput> >& inputs() const;
  /**
   * Returns input with given name.
   */
  ref_ptr<ShaderInput> input(const string &name);
  /**
   * Set a single shader input. Inputs are automatically
   * setup when the shader is enabled.
   */
  void setInput(const ref_ptr<ShaderInput> &in);
  /**
   * Set a set of shader inputs for this program.
   */
  void setInputs(const map<string, ref_ptr<ShaderInput> > &inputs);
  /**
   * Set a single texture for this program.
   * channel must point to the channel the texture is bound to.
   */
  void setTexture(GLint *channel, const string &name);

  /**
   * Returns shader stage GL handle from enumeration.
   * Enumaretion may be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER,
   * GL_GEOMETRY_SHADER, ...
   * Returns a NULL reference if no such shader stage is used.
   */
  ref_ptr<GLuint> stage(GLenum stage) const;
  /**
   * Returns shader stage GLSL code from enumeration.
   * Enumeration may be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER,
   * GL_GEOMETRY_SHADER, ...
   */
  const string& stageCode(GLenum stage) const;
  /**
   * Returns true if the given stage enumeration is used
   * in this program.
   * Enumeration may be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER,
   * GL_GEOMETRY_SHADER, ...
   */
  GLboolean hasStage(GLenum stage) const;

  /**
   * Must be done before linking for transform feedback.
   */
  void setTransformFeedback(const list<string> &transformFeedback, GLenum attributeLayout);

  /**
   * Upload inputs that were added by setInput() or setInputs().
   */
  void uploadInputs();
  /**
   * Upload given texture channel.
   * Note: avoid it, because it uses hashtable lookup.
   */
  void uploadTexture(GLint channel, const string &name);
  /**
   * Upload given attribute access information.
   * Note: avoid it, because it uses hashtable lookup.
   */
  void uploadAttribute(const ShaderInput *in);
  /**
   * Upload given uniform value.
   * Note: avoid it, because it uses hashtable lookup.
   */
  void uploadUniform(const ShaderInput *in);

protected:
  // the GL shader handle that can be shared by multiple Shader's
  ref_ptr<GLuint> id_;
  GLuint numInstances_;

  // shader codes without replaced input prefix
  map<GLenum, string> shaderCodes_;
  // compiled shader objects
  map<GLenum, ref_ptr<GLuint> > shaders_;

  // location maps
  map<string, GLint> samplerLocations_;
  map<string, GLint> uniformLocations_;
  map<string, GLint> attributeLocations_;

  // setup uniforms and attributes
  list<ShaderInputLocation> attributes_;
  list<ShaderInputLocation> uniforms_;
  list<ShaderTextureLocation> textures_;
  // available inputs
  map<string, ref_ptr<ShaderInput> > inputs_;

  list<string> transformFeedback_;
  GLenum transformfeedbackLayout_;

  void setupInputLocations();
};

#endif /* _SHADER_H_ */
