/*
 * render-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef RENDER_STATE_H_
#define RENDER_STATE_H_

#include <stack>
#include <map>

#include <ogle/gl-types/vbo.h>
#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/shader.h>
#include <ogle/gl-types/texture.h>

#include <ogle/utility/stack.h>

class StateNode;
class State;
class MeshState;

class RenderState
{
public:
  RenderState();
  ~RenderState();

  virtual GLboolean isNodeHidden(StateNode *node);
  virtual GLboolean isStateHidden(State *state);

  virtual GLboolean useTransformFeedback() const;
  virtual void set_useTransformFeedback(GLboolean);

  Stack<VertexBufferObject*> vbos;
  Stack<FrameBufferObject*> fbos;
  Stack<Shader*> shaders;
  set< Stack< Texture* >* > activeTextures;

  virtual void pushMesh(MeshState *mesh);
  virtual void popMesh();

  virtual void pushShaderInput(ShaderInput *att);
  virtual void popShaderInput(const string &name);
  virtual GLuint numInstances() const;

  virtual void pushVBO(VertexBufferObject *vbo);
  virtual void popVBO();

  virtual void pushFBO(FrameBufferObject *tex);
  virtual void popFBO();

  virtual void pushShader(Shader *tex);
  virtual void popShader();

  virtual void pushTexture(GLuint unit, Texture *tex);
  virtual void popTexture(GLuint unit);

  virtual GLuint nextTextureUnit();
  virtual void releaseTextureUnit();

protected:
  Stack< Texture* > *textureArray;
  GLint maxTextureUnits_;
  GLint textureCounter_;

  GLboolean useTransformFeedback_;

  map< string, Stack<ShaderInput*> > inputs_;
  list<ShaderInput*> uniforms_;
  list<ShaderInput*> attributes_;
  list<ShaderInput*> constants_;
};

#endif /* RENDER_STATE_H_ */
