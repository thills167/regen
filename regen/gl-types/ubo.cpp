/*
 * ubo.cpp
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#include "ubo.h"
using namespace regen;

UniformBufferObject::UniformBufferObject()
: BufferObject(glGenBuffers,glDeleteBuffers),
  blockSize_(0)
{
}

UniformBufferObject::Layout UniformBufferObject::layout() const
{
  return layout_;
}

void UniformBufferObject::set_layout(Layout layout)
{
  layout_ = layout;
}

GLuint UniformBufferObject::getBlockIndex(GLuint shader, char* blockName) const
{
  return glGetUniformBlockIndex(shader, blockName);
}

void UniformBufferObject::bindBlock(
    GLuint shader, GLuint blockIndex, GLuint bindingPoint) const
{
  glUniformBlockBinding(shader, blockIndex, bindingPoint);
}

void UniformBufferObject::setData(byte *data, GLuint size)
{
  blockSize_ = size;
  glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

void UniformBufferObject::setSubData(byte *data, GLuint offset, GLuint size) const
{
  glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
}