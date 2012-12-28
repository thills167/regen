/*
 * shader-configuration.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef SHADER_CONFIGURATION_H_
#define SHADER_CONFIGURATION_H_

#include <set>
#include <list>
using namespace std;

#include <ogle/gl-types/shader.h>
#include <ogle/gl-types/shader-input.h>
#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/tesselation-config.h>

class State;

class ShaderConfig
{
public:
  ShaderConfig();

  const map<string, string>& functions() const;
  void defineFunction(const string &name, const string &value);

  const map<string,string>& defines() const;
  void define(const string &name, const string &value);

  void setVersion(const string &version);

  void setUseFog(GLboolean toggle=GL_TRUE);

  void setIgnoreCameraRotation(GLboolean toggle=GL_TRUE);
  void setIgnoreCameraTranslation(GLboolean toggle=GL_TRUE);

  void setTesselationCfg(const TesselationConfig &tessCfg);
  const TesselationConfig& tessCfg() const;

  void setBones(GLuint numBoneWeights, GLuint numBones);

  void setMaterial(State *material);
  const State* material() const;

  void addLight(State *light);
  list<State*>& lights();

  void addTexture(State *tex);
  list<State*>& textures();

  void setShaderInput(const ref_ptr<ShaderInput>&);
  const map< string, ref_ptr<ShaderInput> >& inputs() const;

  void setTransformFeedbackAttributes(list< ref_ptr<VertexAttribute> >&,
      GLenum attributeLayout=GL_SEPARATE_ATTRIBS);
  const list< ref_ptr<VertexAttribute> >& transformFeedbackAttributes() const;
  GLenum transformFeedbackMode() const;

protected:
  map<string,string> defines_;
  map<string,string> functions_;

  State* material_;

  list<State*> lights_;
  list<State*> textures_;

  map< string, ref_ptr<ShaderInput> > inputs_;

  list< ref_ptr<VertexAttribute> > transformFeedbackAttributes_;
  GLenum transformFeedbackMode_;

  TesselationConfig tessCfg_;
};

#endif /* SHADER_CONFIGURATION_H_ */
