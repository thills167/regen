/*
 * render-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "render-state.h"

#include <ogle/utility/gl-error.h>
#include <ogle/states/state.h>
#include <ogle/states/mesh-state.h>
#include <ogle/render-tree/state-node.h>

RenderState::RenderState()
: textureCounter_(-1),
  useTransformFeedback_(GL_FALSE)
{
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &maxTextureUnits_);
  textureArray = new Stack<ShaderTexture>[maxTextureUnits_];
}

GLboolean RenderState::isNodeHidden(StateNode *node)
{
  return node->isHidden();
}
GLboolean RenderState::isStateHidden(State *state)
{
  return state->isHidden();
}

RenderState::~RenderState()
{
  delete[] textureArray;
}

GLboolean RenderState::useTransformFeedback() const
{
  return useTransformFeedback_;
}
void RenderState::set_useTransformFeedback(GLboolean toggle)
{
  useTransformFeedback_ = toggle;
}

void RenderState::pushMesh(MeshState *mesh)
{
  mesh->draw(numInstances());
}
void RenderState::popMesh()
{
}

void RenderState::pushVBO(VertexBufferObject *vbo)
{
  vbos.push(vbo);
  vbo->bind(GL_ARRAY_BUFFER);
  vbo->bind(GL_ELEMENT_ARRAY_BUFFER);
}
void RenderState::popVBO()
{
  vbos.pop();
  if(!vbos.isEmpty()) {
    // re-enable VBO from parent node
    VertexBufferObject *parent = vbos.top();
    parent->bind(GL_ARRAY_BUFFER);
    parent->bind(GL_ELEMENT_ARRAY_BUFFER);
  }
}

void RenderState::pushFBO(FrameBufferObject *fbo)
{
  fbos.push(fbo);
  fbo->bind();
  fbo->set_viewport();
}
void RenderState::popFBO()
{
  fbos.pop();
  if(!fbos.isEmpty()) {
    // re-enable FBO from parent node
    FrameBufferObject *parent = fbos.top();
    parent->bind();
    parent->set_viewport();
  }
}

void RenderState::pushShader(Shader *shader)
{
  shaders.push(shader);
  glUseProgram(shader->id());
  shader->applyInputs();
  for(set< Stack<ShaderTexture>* >::const_iterator
      it=activeTextures.begin(); it!=activeTextures.end(); ++it)
  {
    shader->applyTexture((*it)->top());
  }
}
void RenderState::popShader()
{
  shaders.pop();
  if(!shaders.isEmpty()) {
    // re-enable Shader from parent node
    Shader *parent = shaders.top();
    glUseProgram(parent->id());
    parent->applyInputs();
    for(set< Stack<ShaderTexture>* >::const_iterator
        it=activeTextures.begin(); it!=activeTextures.end(); ++it)
    {
      parent->applyTexture((*it)->top());
    }
  }
}

//////////////////////////

GLuint RenderState::nextTextureUnit()
{
  if(textureCounter_ < maxTextureUnits_) {
    textureCounter_ += 1;
  }
  return textureCounter_;
}
void RenderState::releaseTextureUnit()
{
  Stack<ShaderTexture> &queue = textureArray[textureCounter_];
  if(queue.isEmpty()) {
    textureCounter_ -= 1;
  }
}

void RenderState::pushTexture(GLuint unit, Texture *tex)
{
  Stack<ShaderTexture> &queue = textureArray[unit];
  queue.push(ShaderTexture(tex,unit));
  activeTextures.insert(&queue);

  glActiveTexture(GL_TEXTURE0 + unit);
  tex->bind();
  if(!shaders.isEmpty()) {
    shaders.top()->applyTexture(ShaderTexture(tex, unit));
  }
}
void RenderState::popTexture(GLuint unit)
{
  Stack<ShaderTexture> &queue = textureArray[unit];
  queue.pop();

  if(!queue.isEmpty()) {
    glActiveTexture(GL_TEXTURE0 + unit);
    queue.top().tex->bind();
    if(!shaders.isEmpty()) {
      shaders.top()->applyTexture(queue.top());
    }
  } else {
    activeTextures.erase(&queue);
  }
}

//////////////////////////

void RenderState::pushShaderInput(ShaderInput *in)
{
  if(shaders.isEmpty()) { return; }
  Shader *activeShader = shaders.top();

  inputs_[in->name()].push(in);

  if(in->numVertices()>1 || in->numInstances()>1)
  {
    activeShader->applyAttribute(in);
  }
  else if(!in->isConstant())
  {
    activeShader->applyUniform(in);
  }
}
void RenderState::popShaderInput(const string &name)
{
  if(shaders.isEmpty()) { return; }
  Shader *activeShader = shaders.top();

  Stack<ShaderInput*> &inputStack = inputs_[name];
  inputStack.pop();

  // reactivate new top stack member
  if(!inputStack.isEmpty())
  {
    ShaderInput *reactivated = inputStack.topPtr();
    // re-apply input
    if(reactivated->numVertices()>1 || reactivated->numInstances()>1)
    {
      activeShader->applyAttribute(reactivated);
    }
    else if(!reactivated->isConstant())
    {
      activeShader->applyUniform(reactivated);
    }
  }
}
GLuint RenderState::numInstances() const
{
  if(shaders.isEmpty()) {
    return 1;
  } else {
    return shaders.top()->numInstances();
  }
}
