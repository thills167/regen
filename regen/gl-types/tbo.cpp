/*
 * tbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "tbo.h"
using namespace regen;

#include <regen/gl-types/gl-util.h>

TBO::TBO(GLenum texelFormat)
: Texture()
{
  targetType_ = GL_TEXTURE_BUFFER;
  samplerType_ = "samplerBuffer";
  texelFormat_ = texelFormat;
}

void TBO::attach(const ref_ptr<VBO> &vbo, VBOReference &ref)
{
  attachedVBO_ = vbo;
  attachedVBORef_ = ref;
#ifdef GL_ARB_texture_buffer_range
  glTexBufferRange(
      targetType_,
      texelFormat_,
      ref->bufferID(),
      ref->address(),
      ref->allocatedSize());
#else
  glTexBuffer(targetType_, texelFormat_, ref->bufferID());
#endif
  GL_ERROR_LOG();
}
void TBO::attach(GLuint storage)
{
  attachedVBO_ = ref_ptr<VBO>();
  attachedVBORef_ = VBOReference();
  glTexBuffer(targetType_, texelFormat_, storage);
  GL_ERROR_LOG();
}
void TBO::attach(GLuint storage, GLuint offset, GLuint size)
{
  attachedVBO_ = ref_ptr<VBO>();
  attachedVBORef_ = VBOReference();
#ifdef GL_ARB_texture_buffer_range
  glTexBufferRange(targetType_, texelFormat_, storage, offset, size);
#else
  glTexBuffer(targetType_, texelFormat_, storage);
#endif
  GL_ERROR_LOG();
}

void TBO::texImage() const
{
}
