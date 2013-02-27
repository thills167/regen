/*
 * vbo-manager.cpp
 *
 *  Created on: 02.08.2012
 *      Author: daniel
 */

#include "vbo-manager.h"

#include <ogle/utility/gl-util.h>
#include <ogle/utility/string-util.h>
#include <ogle/states/render-state.h>

#define BUFFER_SIZE_2MB 2097152

GLuint VBOManager::defaultBufferSize_ = BUFFER_SIZE_2MB;

VertexBufferObject::Usage VBOManager::defaultUsage_ =
    VertexBufferObject::USAGE_DYNAMIC;

ref_ptr<VertexBufferObject> VBOManager::activeVBO_ =
    ref_ptr<VertexBufferObject>();

map<GLuint, ref_ptr<VertexBufferObject> > VBOManager::bufferIDs_ =
  map<GLuint, ref_ptr<VertexBufferObject> >();

const ref_ptr<VertexBufferObject>& VBOManager::activeBuffer()
{
  return activeVBO_;
}

void VBOManager::set_defaultBufferSize(GLuint v)
{
  defaultBufferSize_ = v;
}
GLuint VBOManager::set_defaultBufferSize()
{
  return defaultBufferSize_;
}

void VBOManager::set_defaultUsage(VertexBufferObject::Usage v)
{
  defaultUsage_ = v;
}
VertexBufferObject::Usage VBOManager::set_defaultUsage()
{
  return defaultUsage_;
}

void VBOManager::createBuffer(
    GLuint bufferSize, GLuint minSize,
    VertexBufferObject::Usage usage)
{
  // check if any buffer has enough space left
  for(map<GLuint, ref_ptr<VertexBufferObject> >::iterator
      it=bufferIDs_.begin(); it!=bufferIDs_.end(); ++it)
  {
    ref_ptr<VertexBufferObject> &buffer = it->second;
    if(buffer->maxContiguousSpace()>=minSize) {
      activeVBO_ = buffer;
      return;
    }
  }
  // create a new target
  activeVBO_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(usage, bufferSize));
  bufferIDs_[activeVBO_->id()] = activeVBO_;
}

void VBOManager::add(const ref_ptr<VertexAttribute> &in)
{
  GLuint attributeSize = in->size();
  GLuint minSize = (attributeSize>defaultBufferSize_ ? attributeSize : defaultBufferSize_);
  ref_ptr<VertexBufferObject> buffer;

  // try to add to same buffer again after removal
  if(in->buffer()>0) {
    buffer = bufferIDs_[in->buffer()];
    remove(in);
  } else {
    buffer = activeVBO_;
  }

  if(buffer.get()==NULL) {
    createBuffer(minSize, attributeSize, defaultUsage_);
    buffer = activeVBO_;
  }
  else if(buffer->maxContiguousSpace()<attributeSize) {
    if(activeVBO_.get()==NULL || activeVBO_->maxContiguousSpace()<attributeSize) {
      createBuffer(minSize, attributeSize, defaultUsage_);
    }
    buffer = activeVBO_;
  }

  VBOBlockIterator sequentialIt = buffer->allocateSequential(in);
  in->set_bufferIterator(sequentialIt);
  bufferIDs_[buffer->id()] = buffer;
}

void VBOManager::remove(const ref_ptr<VertexAttribute> &in)
{
  if(in->buffer()>0 && bufferIDs_.count(in->buffer())>0) {
    ref_ptr<VertexBufferObject> buffer = bufferIDs_[in->buffer()];
    VBOBlockIterator it = in->bufferIterator();
    buffer->free(it);
    in->set_buffer(0);
    if(buffer->maxContiguousSpace()==buffer->bufferSize()) {
      // the buffer is completely empty
      if(activeVBO_.get() != buffer.get()) {
        bufferIDs_.erase(buffer->id());
      }
    }
  }
}
