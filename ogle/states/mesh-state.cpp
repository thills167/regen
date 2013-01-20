/*
 * attribute-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "mesh-state.h"

#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>
#include <ogle/states/vbo-state.h>
#include <ogle/states/render-state.h>
#include <ogle/states/vbo-state.h>

// #define DEBUG_TRANSFORM_FEEDBACK

class TransformFeedbackState : public State
{
public:
  TransformFeedbackState(
      const list< ref_ptr<VertexAttribute> > &atts,
      const GLenum &transformFeedbackPrimitive,
      const ref_ptr<VertexBufferObject> &transformFeedbackBuffer)
  : State(),
    usedTF_(GL_FALSE),
    atts_(atts),
    transformFeedbackPrimitive_(transformFeedbackPrimitive),
    transformFeedbackBuffer_(transformFeedbackBuffer)
  {
#ifdef DEBUG_TRANSFORM_FEEDBACK
    glGenQueries(1, &debugQuery_);
#endif
  }
  ~TransformFeedbackState()
  {
#ifdef DEBUG_TRANSFORM_FEEDBACK
    glDeleteQueries(1, &debugQuery_);
#endif
  }
  virtual void enable(RenderState *state)
  {
    usedTF_ = state->useTransformFeedback();
    if(!usedTF_) {
      state->set_useTransformFeedback(GL_TRUE);
      // VertexBufferObject *vbo = state->vbos.top();
      GLint bufferIndex=0;
      for(list< ref_ptr<VertexAttribute> >::const_iterator
          it=atts_.begin(); it!=atts_.end(); ++it)
      {
        const ref_ptr<VertexAttribute> &att = *it;
        glBindBufferRange(
            GL_TRANSFORM_FEEDBACK_BUFFER,
            bufferIndex++,
            transformFeedbackBuffer_->id(),
            att->offset(),
            att->size());
      }
      glBeginTransformFeedback(transformFeedbackPrimitive_);
#ifdef DEBUG_TRANSFORM_FEEDBACK
      glBeginQuery(GL_PRIMITIVES_GENERATED, debugQuery_);
#endif
    }
    State::enable(state);
  }
  virtual void disable(RenderState *state)
  {
    State::disable(state);
    if(!usedTF_) {
#ifdef DEBUG_TRANSFORM_FEEDBACK
      glEndQuery(GL_PRIMITIVES_GENERATED);
#endif
      glEndTransformFeedback();
      for(GLuint bufferIndex=0u; bufferIndex<atts_.size(); ++bufferIndex)
      {
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, bufferIndex, 0);
      }
#ifdef DEBUG_TRANSFORM_FEEDBACK
      GLint numPrimitivesGenerated;
      glGetQueryObjectiv(debugQuery_, GL_QUERY_RESULT, &numPrimitivesGenerated);
      DEBUG_LOG("GL_PRIMITIVES_GENERATED=" << numPrimitivesGenerated);
#endif
      state->set_useTransformFeedback(GL_FALSE);
    }
  }
protected:
#ifdef DEBUG_TRANSFORM_FEEDBACK
  GLuint debugQuery_;
#endif
  GLboolean usedTF_;
  const list< ref_ptr<VertexAttribute> > &atts_;
  const GLenum &transformFeedbackPrimitive_;
  const ref_ptr<VertexBufferObject> &transformFeedbackBuffer_;
};

MeshState::MeshState(GLenum primitive)
: ShaderInputState(),
  primitive_(primitive)
{
  vertices_ = inputs_.end();
  normals_ = inputs_.end();
  colors_ = inputs_.end();

  set_primitive(primitive);

  transformFeedbackState_ = ref_ptr<State>::manage(
      new TransformFeedbackState(tfAttributes_, tfPrimitive_, tfVBO_));
}

GLenum MeshState::primitive() const
{
  return primitive_;
}
GLenum MeshState::transformFeedbackPrimitive() const
{
  return tfPrimitive_;
}
void MeshState::set_primitive(GLenum primitive)
{
  primitive_ = primitive;
  switch(primitive_) {
  case GL_PATCHES:
    tfPrimitive_ = GL_TRIANGLES;
    break;
  case GL_POINTS:
    tfPrimitive_ = GL_POINTS;
    break;
  case GL_LINES:
  case GL_LINE_LOOP:
  case GL_LINE_STRIP:
  case GL_LINES_ADJACENCY:
  case GL_LINE_STRIP_ADJACENCY:
    tfPrimitive_ = GL_LINES;
    break;
  case GL_TRIANGLES:
  case GL_TRIANGLE_STRIP:
  case GL_TRIANGLE_FAN:
  case GL_TRIANGLES_ADJACENCY:
  case GL_TRIANGLE_STRIP_ADJACENCY:
    tfPrimitive_ = GL_TRIANGLES;
    break;
  default:
    tfPrimitive_ = GL_TRIANGLES;
    break;
  }
}

GLuint MeshState::numVertices() const
{
  return numVertices_;
}

const ShaderInputIteratorConst& MeshState::vertices() const
{
  return vertices_;
}
const ShaderInputIteratorConst& MeshState::normals() const
{
  return normals_;
}
const ShaderInputIteratorConst& MeshState::colors() const
{
  return colors_;
}

ShaderInputIteratorConst MeshState::setInput(const ref_ptr<ShaderInput> &in)
{
  if(in->numVertices()>1) {
    // it is a per vertex attribute
    numVertices_ = in->numVertices();
  }

  ShaderInputIteratorConst it = ShaderInputState::setInput(in);

  if(in->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = it;
  } else if(in->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = it;
  } else if(in->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = it;
  }

  return it;
}
void MeshState::removeInput(const ref_ptr<ShaderInput> &in)
{
  if(in->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = inputs_.end();
  } else if(in->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = inputs_.end();
  } else if(in->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = inputs_.end();
  }
  ShaderInputState::removeInput(in);
}

/////////////

void MeshState::draw(GLuint numInstances)
{
  if(numInstances>1) {
    glDrawArraysInstancedEXT(primitive_, 0, numVertices_, numInstances);
  }
  else {
    glDrawArrays(primitive_, 0, numVertices_);
  }
}

void MeshState::drawTransformFeedback(GLuint numInstances)
{
  if(numInstances>1) {
    glDrawArraysInstancedEXT(tfPrimitive_, 0, numVertices_, numInstances);
  }
  else {
    glDrawArrays(tfPrimitive_, 0, numVertices_);
  }
}

/////////////

const ref_ptr<VertexBufferObject>& MeshState::transformFeedbackBuffer()
{
  return tfVBO_;
}

AttributeIteratorConst MeshState::getTransformFeedbackAttribute(const string &name) const
{
  AttributeIteratorConst it;
  for(it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      return it;
    }
  }
  return it;
}
GLboolean MeshState::hasTransformFeedbackAttribute(const string &name) const
{
  return tfAttributeMap_.count(name)>0;
}

list< ref_ptr<VertexAttribute> >* MeshState::tfAttributesPtr()
{
  return &tfAttributes_;
}
const list< ref_ptr<VertexAttribute> >& MeshState::tfAttributes() const
{
  return tfAttributes_;
}

AttributeIteratorConst MeshState::setTransformFeedbackAttribute(const ref_ptr<ShaderInput> &in)
{
  if(tfAttributeMap_.count(in->name())>0) {
    removeTransformFeedbackAttribute(in->name());
  } else { // insert into map of known attributes
    tfAttributeMap_[in->name()] = in;
  }

  if(tfAttributes_.empty()) {
    joinStates(transformFeedbackState_);
  }

  in->set_size(numVertices_ * in->elementSize());
  in->set_numVertices(numVertices_);

  tfAttributes_.push_front(ref_ptr<VertexAttribute>::cast(in));

  return tfAttributes_.begin();
}

void MeshState::removeTransformFeedbackAttribute(const ref_ptr<ShaderInput> &in)
{
  removeTransformFeedbackAttribute(in->name());
}

void MeshState::removeTransformFeedbackAttribute(const string &name)
{
  tfAttributeMap_.erase(name);
  for(list< ref_ptr<VertexAttribute> >::iterator
      it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      tfAttributes_.erase(it);
      return;
    }
  }

  if(tfAttributes_.size()==0) {
    disjoinStates(transformFeedbackState_);
  }
}

void MeshState::updateTransformFeedbackBuffer()
{
  if(tfAttributes_.empty()) { return; }
  tfVBO_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(
      VertexBufferObject::USAGE_STREAM,
      VertexBufferObject::attributeStructSize(tfAttributes_)
  ));
  tfVBO_->allocateSequential(tfAttributes_);
}

//////////

void MeshState::enable(RenderState *state)
{
  ShaderInputState::enable(state);
  state->pushMesh(this);
}
void MeshState::disable(RenderState *state)
{
  state->popMesh();
  ShaderInputState::disable(state);
}

////////////

IndexedMeshState::IndexedMeshState(GLenum primitive)
: MeshState(primitive),
  numIndices_(0u)
{
}

GLuint IndexedMeshState::numIndices() const
{
  return numIndices_;
}

GLuint IndexedMeshState::maxIndex()
{
  return maxIndex_;
}

const ref_ptr<VertexAttribute>& IndexedMeshState::indices() const
{
  return indices_;
}

void IndexedMeshState::draw(GLuint numInstances)
{
  if(numInstances>1) {
    glDrawElementsInstancedEXT(primitive_, numIndices_, indices_->dataType(),
        BUFFER_OFFSET(indices_->offset()), numInstances);
  }
  else {
    glDrawElements(primitive_, numIndices_, indices_->dataType(),
        BUFFER_OFFSET(indices_->offset()));
  }
}

void IndexedMeshState::drawTransformFeedback(GLuint numInstances)
{
  if(numInstances>1) {
    glDrawArraysInstanced(tfPrimitive_, 0, numIndices_, numInstances);
  }
  else {
    glDrawArrays(tfPrimitive_, 0, numIndices_);
  }
}

void IndexedMeshState::setIndices(const ref_ptr<VertexAttribute> &indices, GLuint maxIndex)
{
  indices_ = indices;
  numIndices_ = indices_->numVertices();
  maxIndex_ = maxIndex;
}

void IndexedMeshState::setFaceIndicesui(GLuint *faceIndices, GLuint numFaceIndices, GLuint numFaces)
{
  numIndices_ = numFaces*numFaceIndices;
  maxIndex_ = 0;

  // find max index
  for(GLuint i=0; i<numIndices_; ++i)
  {
    GLuint &index = faceIndices[i];
    if(index>maxIndex_) { maxIndex_=index; }
  }

  if(indices_.get()!=NULL) {
    VBOManager::remove(indices_);
  }
  indices_ = ref_ptr<VertexAttribute>::manage(new VertexAttribute(
      "i", GL_UNSIGNED_INT, sizeof(GLuint), 1, 1, GL_FALSE));
  indices_->setVertexData(numIndices_, (byte*)faceIndices);
  VBOManager::addSequential(indices_);
}

AttributeIteratorConst IndexedMeshState::setTransformFeedbackAttribute(const ref_ptr<ShaderInput> &in)
{
  AttributeIteratorConst it = MeshState::setTransformFeedbackAttribute(in);

  in->set_size(numIndices_ * in->elementSize());
  in->set_numVertices(numIndices_);

  return it;
}

////////////

TFMeshState::TFMeshState(ref_ptr<MeshState> attState)
: State(),
  attState_(attState)
{

}

void TFMeshState::enable(RenderState *state)
{
  if(state->useTransformFeedback()) { return; }

  State::enable(state);

  for(list< ref_ptr<VertexAttribute> >::iterator
      it=attState_->tfAttributesPtr()->begin(); it!=attState_->tfAttributesPtr()->end(); ++it)
  {
    state->pushShaderInput((ShaderInput*)it->get());
  }

  attState_->drawTransformFeedback(state->numInstances());
}

void TFMeshState::disable(RenderState *state)
{
  if(state->useTransformFeedback()) { return; }
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it=attState_->tfAttributes().begin(); it!=attState_->tfAttributes().end(); ++it)
  {
    state->popShaderInput((*it)->name());
  }

  State::disable(state);
}
