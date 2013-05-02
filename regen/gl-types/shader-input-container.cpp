/*
 * shader-input-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-util.h>

#include "shader-input-container.h"
using namespace regen;

ShaderInputContainer::ShaderInputContainer(VertexBufferObject::Usage usage)
: numVertices_(0), numInstances_(1), numIndices_(0)
{
  inputBuffer_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(usage));
}
ShaderInputContainer::ShaderInputContainer(
    const ref_ptr<ShaderInput> &in, const string &name, VertexBufferObject::Usage usage)
: numVertices_(0), numInstances_(1), numIndices_(0)
{
  inputBuffer_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(usage));
  setInput(in,name);
}

ShaderInputContainer::~ShaderInputContainer()
{
  while(!inputs_.empty())
  { removeInput(inputs_.begin()->name_); }
}

const ref_ptr<VertexBufferObject>& ShaderInputContainer::inputBuffer() const
{ return inputBuffer_; }

GLuint ShaderInputContainer::numVertices() const
{ return numVertices_; }
void ShaderInputContainer::set_numVertices(GLuint v)
{ numVertices_ = v; }
GLuint ShaderInputContainer::numInstances() const
{ return numInstances_; }

ref_ptr<ShaderInput> ShaderInputContainer::getInput(const string &name) const
{
  for(ShaderInputList::const_iterator it=inputs_.begin(); it!=inputs_.end(); ++it)
  {
    if(name.compare(it->name_) == 0) return it->in_;
  }
  return ref_ptr<ShaderInput>();
}

GLboolean ShaderInputContainer::hasInput(const string &name) const
{ return inputMap_.count(name)>0; }
const ShaderInputList& ShaderInputContainer::inputs() const
{ return inputs_; }

void ShaderInputContainer::beginUpload(DataLayout layout)
{
  uploadLayout_ = layout;
}
void ShaderInputContainer::endUpload()
{
  if(uploadLayout_ == SEQUENTIAL)
  { inputBuffer_->allocSequential(uploadInputs_); }
  else if(uploadLayout_ == INTERLEAVED)
  { inputBuffer_->allocInterleaved(uploadInputs_); }
  uploadInputs_.clear();
  uploadLayout_ = LAYOUT_LAST;
}

ShaderInputList::const_iterator ShaderInputContainer::setInput(
    const ref_ptr<ShaderInput> &in, const string &name)
{
  string inputName = (name.empty() ? in->name() : name);

  if(in->numVertices()>1)
  { numVertices_ = in->numVertices(); }
  if(in->numInstances()>1)
  { numInstances_ = in->numInstances(); }

  if(inputMap_.count(inputName)>0) {
    removeInput(inputName);
  } else { // insert into map of known attributes
    inputMap_.insert(inputName);
  }

  inputs_.push_front(NamedShaderInput(in, inputName));

  if(uploadLayout_ != LAYOUT_LAST)
  { uploadInputs_.push_front(in); }

  return inputs_.begin();
}

void ShaderInputContainer::setIndices(const ref_ptr<VertexAttribute> &indices, GLuint maxIndex)
{
  indices_ = indices;
  numIndices_ = indices_->numVertices();
  maxIndex_ = maxIndex;
  inputBuffer_->alloc(indices_);
}

GLuint ShaderInputContainer::numIndices() const
{ return numIndices_; }
GLuint ShaderInputContainer::maxIndex()
{ return maxIndex_; }
const ref_ptr<VertexAttribute>& ShaderInputContainer::indices() const
{ return indices_; }
GLuint ShaderInputContainer::indexBuffer() const
{ return indices_.get() ? indices_->buffer() : 0; }

void ShaderInputContainer::removeInput(const ref_ptr<ShaderInput> &in)
{
  inputMap_.erase(in->name());
  removeInput(in->name());
}

void ShaderInputContainer::removeInput(const string &name)
{
  ShaderInputList::iterator it;
  for(it=inputs_.begin(); it!=inputs_.end(); ++it) {
    if(it->name_ == name) { break; }
  }
  if(it==inputs_.end()) { return; }

  if(uploadLayout_ != LAYOUT_LAST)
  {
    VBOReference ref = it->in_->bufferIterator();
    if(ref.get()) {
      inputBuffer_->free(ref.get());
      it->in_->set_buffer(0u,VBOReference());
    }
  }

  inputs_.erase(it);
}
