/*
 * shader-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-util.h>
#include <ogle/shading/light-state.h>
#include <ogle/states/material-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/gl-types/glsl-io-processor.h>
#include <ogle/gl-types/glsl-directive-processor.h>
#include <ogle/gl-types/gl-enum.h>

#include "shader-state.h"
using namespace ogle;

ShaderState::ShaderState(const ref_ptr<Shader> &shader)
: State(), shader_(shader) {}

ShaderState::ShaderState()
: State() {}

void ShaderState::loadStage(
    const map<string, string> &shaderConfig,
    const string &effectName,
    map<GLenum,string> &code,
    GLenum stage)
{
  string stageName = GLEnum::glslStageName(stage);
  string effectKey = FORMAT_STRING(effectName << "." << GLEnum::glslStagePrefix(stage));
  string ignoreKey = FORMAT_STRING("IGNORE_" << stageName);

  map<string, string>::const_iterator it = shaderConfig.find(ignoreKey);
  if(it!=shaderConfig.end() && it->second=="TRUE") { return; }

  code[stage] = GLSLDirectiveProcessor::include(effectKey);
  // failed to include ?
  if(code[stage].empty()) { code.erase(stage); }
}

GLboolean ShaderState::createShader(const Config &cfg, const string &shaderKey)
{
  const map<string, ref_ptr<ShaderInput> > specifiedInput = cfg.inputs_;
  const list<const TextureState*> &textures = cfg.textures_;
  const map<string, string> &shaderConfig = cfg.defines_;
  const map<string, string> &shaderFunctions = cfg.functions_;
  map<GLenum,string> code;

  for(GLint i=0; i<GLEnum::glslStageCount(); ++i) {
    loadStage(shaderConfig, shaderKey, code, GLEnum::glslStages()[i]);
  }

  ref_ptr<Shader> shader = Shader::create(
      cfg.version(),
      shaderConfig,
      shaderFunctions,
      specifiedInput,
      code);
  // setup transform feedback attributes
  shader->setTransformFeedback(cfg.feedbackAttributes_, cfg.feedbackMode_, cfg.feedbackStage_);

  if(!shader->compile()) {
    ERROR_LOG("Shader '" << shaderKey << "' failed to compiled.");
    return GL_FALSE;
  }
  if(!shader->link()) {
    ERROR_LOG("Shader '" << shaderKey << "' failed to link.");
  }

  shader->setInputs(specifiedInput);
  for(list<const TextureState*>::const_iterator
      it=textures.begin(); it!=textures.end(); ++it)
  {
    const TextureState *s = *it;
    if(!s->name().empty()) {
      shader->setTexture(s->channel(), s->name());
    }
  }

  shader_ = shader;

  INFO_LOG("Shader '" << shaderKey << "' compiled.");

  return GL_TRUE;
}

const ref_ptr<Shader>& ShaderState::shader() const
{
  return shader_;
}
void ShaderState::set_shader(ref_ptr<Shader> shader)
{
  shader_ = shader;
}

void ShaderState::enable(RenderState *state)
{
  state->shader().push(shader_.get());
  State::enable(state);
}
void ShaderState::disable(RenderState *state)
{
  State::disable(state);
  state->shader().pop();
}