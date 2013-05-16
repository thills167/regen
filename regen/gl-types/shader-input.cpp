/*
 * shader-input.cpp
 *
 *  Created on: 15.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>

#include "shader-input.h"
using namespace regen;

ShaderInput::ShaderInput(
    const string &name,
    GLenum dataType,
    GLuint dataTypeBytes,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: name_(name),
  dataType_(dataType),
  dataTypeBytes_(dataTypeBytes),
  stride_(0),
  offset_(0),
  inputSize_(0),
  elementSize_(0),
  elementCount_(elementCount),
  numVertices_(0u),
  numInstances_(0u),
  valsPerElement_(valsPerElement),
  divisor_(0),
  buffer_(0),
  bufferStamp_(0),
  normalize_(normalize),
  isVertexAttribute_(GL_TRUE),
  transpose_(GL_FALSE),
  data_(NULL),
  stamp_(1u),
  isConstant_(GL_FALSE),
  forceArray_(GL_FALSE),
  active_(GL_TRUE)
{
  elementSize_ = dataTypeBytes_*valsPerElement_*elementCount_;
  // make data_ stack root
  dataStack_.push(data_);
  enableAttribute_ = &ShaderInput::enableAttributef;
}
ShaderInput::~ShaderInput()
{
  deallocateData();
  if(bufferIterator_.get()) {
    VBO::free(bufferIterator_.get());
  }
}

GLenum ShaderInput::dataType() const
{ return dataType_; }
GLuint ShaderInput::dataTypeBytes() const
{ return dataTypeBytes_; }

const string& ShaderInput::name() const
{ return name_; }
void ShaderInput::set_name(const string &s)
{ name_ = s; }

GLboolean ShaderInput::active() const
{ return active_; }
void ShaderInput::set_active(GLboolean v)
{ active_ = v; }

GLuint ShaderInput::numInstances() const
{ return numInstances_; }

GLuint ShaderInput::numVertices() const
{ return numVertices_; }
void ShaderInput::set_numVertices(GLuint numVertices)
{ numVertices_ = numVertices; }

GLboolean ShaderInput::isVertexAttribute() const
{ return isVertexAttribute_; }

void ShaderInput::set_isConstant(GLboolean isConstant)
{ isConstant_ = isConstant; }
GLboolean ShaderInput::isConstant() const
{ return isConstant_; }

void ShaderInput::set_forceArray(GLboolean forceArray)
{ forceArray_ = forceArray; }
GLboolean ShaderInput::forceArray() const
{ return forceArray_; }

GLuint ShaderInput::stamp() const
{ return stamp_; }
void ShaderInput::nextStamp()
{ stamp_ += 1; }

void ShaderInput::set_stride(GLuint stride)
{ stride_ = stride; }
GLuint ShaderInput::stride() const
{ return stride_; }

void ShaderInput::set_offset(GLuint offset)
{ offset_ = offset; }
GLuint ShaderInput::offset() const
{ return offset_; }

GLuint ShaderInput::divisor() const
{ return divisor_; }

GLuint ShaderInput::inputSize() const
{ return inputSize_; }
void ShaderInput::set_inputSize(GLuint size)
{ inputSize_ = size; }

GLuint ShaderInput::elementSize() const
{ return elementSize_; }

void ShaderInput::set_elementCount(GLuint v)
{
  elementCount_ = v;
  elementSize_ = dataTypeBytes_*valsPerElement_*elementCount_;
}
GLuint ShaderInput::elementCount() const
{ return elementCount_; }

GLuint ShaderInput::valsPerElement() const
{ return valsPerElement_; }

GLboolean ShaderInput::normalize() const
{ return normalize_; }

void ShaderInput::set_transpose(GLboolean transpose)
{ transpose_ = transpose; }
GLboolean ShaderInput::transpose() const
{ return transpose_; }

void ShaderInput::set_buffer(GLuint buffer, const VBOReference &it)
{
  buffer_ = buffer;
  bufferIterator_ = it;
  bufferStamp_ = stamp_;
}
GLuint ShaderInput::buffer() const
{ return buffer_; }
GLuint ShaderInput::bufferStamp() const
{ return bufferStamp_; }
ref_ptr<VBO::Reference> ShaderInput::bufferIterator()
{ return bufferIterator_; }

/////////////
/////////////
/////////////

void ShaderInput::enableAttributef(GLint location) const
{
  for(register GLuint i=0; i<elementCount_; ++i) {
    GLint loc = location+i;
    glEnableVertexAttribArray( loc );
    glVertexAttribPointer(
        loc,
        valsPerElement_,
        dataType_,
        normalize_,
        stride_,
        BUFFER_OFFSET(offset_));
    if(divisor_!=0) {
      glVertexAttribDivisor(loc, divisor_);
    }
  }
}
void ShaderInput::enableAttributei(GLint location) const
{
  for(register GLuint i=0; i<elementCount_; ++i) {
    GLint loc = location+i;
    glEnableVertexAttribArray( loc );
    // use glVertexAttribIPointer, otherwise OpenGL
    // would convert integers to float
    glVertexAttribIPointer(
        loc,
        valsPerElement_,
        dataType_,
        stride_,
        BUFFER_OFFSET(offset_));
    if(divisor_!=0) {
      glVertexAttribDivisor(loc, divisor_);
    }
  }
}
void ShaderInput::enableAttributeMat4(GLint location) const
{
  for(register GLuint i=0; i<elementCount_*4; i+=4) {
    GLint loc0 = location+i;
    GLint loc1 = location+i+1;
    GLint loc2 = location+i+2;
    GLint loc3 = location+i+3;

    glEnableVertexAttribArray( loc0 );
    glEnableVertexAttribArray( loc1 );
    glEnableVertexAttribArray( loc2 );
    glEnableVertexAttribArray( loc3 );

    glVertexAttribPointer(loc0,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_));
    glVertexAttribPointer(loc1,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*4));
    glVertexAttribPointer(loc2,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*8));
    glVertexAttribPointer(loc3,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*12));

    if(divisor_!=0) {
      glVertexAttribDivisor(loc0, divisor_);
      glVertexAttribDivisor(loc1, divisor_);
      glVertexAttribDivisor(loc2, divisor_);
      glVertexAttribDivisor(loc3, divisor_);
    }
  }
}
void ShaderInput::enableAttributeMat3(GLint location) const
{
  for(register GLuint i=0; i<elementCount_*3; i+=4) {
    GLint loc0 = location+i;
    GLint loc1 = location+i+1;
    GLint loc2 = location+i+2;

    glEnableVertexAttribArray( loc0 );
    glEnableVertexAttribArray( loc1 );
    glEnableVertexAttribArray( loc2 );

    glVertexAttribPointer(loc0,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_));
    glVertexAttribPointer(loc1,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*4));
    glVertexAttribPointer(loc2,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*8));

    if(divisor_!=0) {
      glVertexAttribDivisor(loc0, divisor_);
      glVertexAttribDivisor(loc1, divisor_);
      glVertexAttribDivisor(loc2, divisor_);
    }
  }
}
void ShaderInput::enableAttributeMat2(GLint location) const
{
  for(register GLuint i=0; i<elementCount_*2; i+=4) {
    GLint loc0 = location+i;
    GLint loc1 = location+i+1;

    glEnableVertexAttribArray( loc0 );
    glEnableVertexAttribArray( loc1 );

    glVertexAttribPointer(loc0,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_));
    glVertexAttribPointer(loc1,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*4));

    if(divisor_!=0) {
      glVertexAttribDivisor(loc0, divisor_);
      glVertexAttribDivisor(loc1, divisor_);
    }
  }
}
void ShaderInput::enableAttribute(GLint loc) const
{
  (this->*(this->enableAttribute_))(loc);
}

void ShaderInput::enableUniform1f(GLint loc) const
{
  glUniform1fv(loc, elementCount_, (const GLfloat*)data());
}
void ShaderInput::enableUniform2f(GLint loc) const
{
  glUniform2fv(loc, elementCount_, (const GLfloat*)data());
}
void ShaderInput::enableUniform3f(GLint loc) const
{
  glUniform3fv(loc, elementCount_, (const GLfloat*)data());
}
void ShaderInput::enableUniform4f(GLint loc) const
{
  glUniform4fv(loc, elementCount_, (const GLfloat*)data());
}
void ShaderInput::enableUniformMat3(GLint loc) const
{
  glUniformMatrix3fv(loc, elementCount_, transpose_, (const GLfloat*)data());
}
void ShaderInput::enableUniformMat4(GLint loc) const
{
  glUniformMatrix4fv(loc, elementCount_, transpose_, (const GLfloat*)data());
}

void ShaderInput::enableUniform1d(GLint loc) const
{
  glUniform1dv(loc, elementCount_, (const GLdouble*)data());
}
void ShaderInput::enableUniform2d(GLint loc) const
{
  glUniform2dv(loc, elementCount_, (const GLdouble*)data());
}
void ShaderInput::enableUniform3d(GLint loc) const
{
  glUniform3dv(loc, elementCount_, (const GLdouble*)data());
}
void ShaderInput::enableUniform4d(GLint loc) const
{
  glUniform4dv(loc, elementCount_, (const GLdouble*)data());
}

void ShaderInput::enableUniform1i(GLint loc) const
{
  glUniform1iv(loc, elementCount_, (const GLint*)data());
}
void ShaderInput::enableUniform2i(GLint loc) const
{
  glUniform2iv(loc, elementCount_, (const GLint*)data());
}
void ShaderInput::enableUniform3i(GLint loc) const
{
  glUniform3iv(loc, elementCount_, (const GLint*)data());
}
void ShaderInput::enableUniform4i(GLint loc) const
{
  glUniform4iv(loc, elementCount_, (const GLint*)data());
}

void ShaderInput::enableUniform1ui(GLint loc) const
{
  glUniform1uiv(loc, elementCount_, (const GLuint*)data());
}
void ShaderInput::enableUniform2ui(GLint loc) const
{
  glUniform2uiv(loc, elementCount_, (const GLuint*)data());
}
void ShaderInput::enableUniform3ui(GLint loc) const
{
  glUniform3uiv(loc, elementCount_, (const GLuint*)data());
}
void ShaderInput::enableUniform4ui(GLint loc) const
{
  glUniform4uiv(loc, elementCount_, (const GLuint*)data());
}

void ShaderInput::enableUniform(GLint loc) const
{
  (this->*(this->enableUniform_))(loc);
}

/////////////
/////////////
////////////

void ShaderInput::setUniformDataUntyped(byte *data)
{
  setInstanceData(1,1,data);
  isVertexAttribute_ = GL_FALSE;
}

void ShaderInput::setVertexData(
    GLuint numVertices,
    const byte *vertexData)
{
  isVertexAttribute_ = GL_TRUE;
  numVertices_ = numVertices;
  numInstances_ = 1u;
  divisor_ = 0u;
  GLuint size = elementSize_*numVertices_;
  if(inputSize_ != size) {
    if(data_!=NULL) {
      data_ = (byte*) realloc(data_, size);
    } else {
      data_ = (byte*) malloc(size);
    }
    inputSize_ = size;
  }
  if(vertexData) {
    std::memcpy(data_, vertexData, inputSize_);
  }
  stamp_ += 1;
  // make new data stack root
  dataStack_.popBottom();
  dataStack_.pushBottom(data_);
}

void ShaderInput::setInstanceData(
    GLuint numInstances,
    GLuint divisor,
    const byte *instanceData)
{
  isVertexAttribute_ = GL_TRUE;
  numInstances_ = max(1u,numInstances);
  divisor_ = max(1u,divisor);
  numVertices_ = 1u;
  GLuint size = elementSize_*numInstances_/divisor_;
  if(inputSize_ != size) {
    if(data_!=NULL) {
      data_ = (byte*) realloc(data_, size);
    } else {
      data_ = (byte*) malloc(size);
    }
    inputSize_ = size;
  }
  if(instanceData) {
    std::memcpy(data_, instanceData, inputSize_);
  }
  stamp_ += 1;
  // make new data stack root
  dataStack_.popBottom();
  dataStack_.pushBottom(data_);
}

void ShaderInput::deallocateData()
{
  // set null data pointer
  dataStack_.popBottom();
  dataStack_.pushBottom(NULL);
  // and delete the data
  if(data_!=NULL) {
    free(data_);
    data_ = NULL;
  }
}

GLboolean ShaderInput::hasData()
{
  return data_!=NULL || buffer_!=0;
}

const byte* ShaderInput::data() const
{
  return dataStack_.top();
}
byte* ShaderInput::dataPtr()
{
  return dataStack_.top();
}
byte* ShaderInput::ownedData()
{
  return data_;
}

void ShaderInput::pushData(byte *data)
{
  dataStack_.push(data);
  stamp_ += 1;
}
void ShaderInput::popData()
{
  dataStack_.pop();
  stamp_ += 1;
}

/////////////
/////////////
////////////

ref_ptr<ShaderInput> ShaderInput::create(
    const string &name, GLenum dataType, GLuint valsPerElement)
{
  switch(dataType) {
  case GL_FLOAT:
    switch(valsPerElement) {
    case 16:
      return ref_ptr<ShaderInputMat4>::alloc(name);
    case 9:
      return ref_ptr<ShaderInputMat3>::alloc(name);
    case 4:
      return ref_ptr<ShaderInput4f>::alloc(name);
    case 3:
      return ref_ptr<ShaderInput3f>::alloc(name);
    case 2:
      return ref_ptr<ShaderInput2f>::alloc(name);
    default:
      return ref_ptr<ShaderInput1f>::alloc(name);
    }
    break;
  case GL_DOUBLE:
    switch(valsPerElement) {
    case 4:
      return ref_ptr<ShaderInput4d>::alloc(name);
    case 3:
      return ref_ptr<ShaderInput3d>::alloc(name);
    case 2:
      return ref_ptr<ShaderInput2d>::alloc(name);
    default:
      return ref_ptr<ShaderInput1d>::alloc(name);
    }
    break;
  case GL_BOOL:
  case GL_INT:
    switch(valsPerElement) {
    case 4:
      return ref_ptr<ShaderInput4i>::alloc(name);
    case 3:
      return ref_ptr<ShaderInput3i>::alloc(name);
    case 2:
      return ref_ptr<ShaderInput2i>::alloc(name);
    default:
      return ref_ptr<ShaderInput1i>::alloc(name);
    }
    break;
  case GL_UNSIGNED_INT:
    switch(valsPerElement) {
    case 4:
      return ref_ptr<ShaderInput4ui>::alloc(name);
    case 3:
      return ref_ptr<ShaderInput3ui>::alloc(name);
    case 2:
      return ref_ptr<ShaderInput2ui>::alloc(name);
    default:
      return ref_ptr<ShaderInput1ui>::alloc(name);
    }
    break;
  default:
    return ref_ptr<ShaderInput>();
  }
}

ref_ptr<ShaderInput> ShaderInput::copy(const ref_ptr<ShaderInput> &in, GLboolean copyData)
{
  ref_ptr<ShaderInput> cp = create(in->name(), in->dataType(), in->valsPerElement());
  cp->stride_ = in->stride_;
  cp->offset_ = in->offset_;
  cp->inputSize_ = in->inputSize_;
  cp->elementSize_ = in->elementSize_;
  cp->elementCount_ = in->elementCount_;
  cp->numVertices_ = in->numVertices_;
  cp->numInstances_ = in->numInstances_;
  cp->divisor_ = in->divisor_;
  cp->buffer_ = 0;
  cp->bufferStamp_ = 0;
  cp->normalize_ = in->normalize_;
  cp->isVertexAttribute_ = in->isVertexAttribute_;
  cp->isConstant_ = in->isConstant_;
  cp->transpose_ = in->transpose_;
  cp->stamp_ = in->stamp_;
  cp->forceArray_ = in->forceArray_;

  cp->data_ = (byte*) malloc(cp->inputSize_);
  if(copyData && in->data_!=NULL) {
    std::memcpy(cp->data_, in->data_, cp->inputSize_);
  }
  // make data_ stack root
  cp->dataStack_.push(cp->data_);

  return cp;
}

/////////////
/////////////
/////////////

ShaderInput1f::ShaderInput1f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_FLOAT, sizeof(GLfloat), 1, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform1f;
}
ShaderInput2f::ShaderInput2f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_FLOAT, sizeof(GLfloat), 2, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform2f;
}
ShaderInput3f::ShaderInput3f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_FLOAT, sizeof(GLfloat), 3, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform3f;
}
ShaderInput4f::ShaderInput4f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_FLOAT, sizeof(GLfloat), 4, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform4f;
}

ShaderInputMat3::ShaderInputMat3(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_FLOAT, sizeof(GLfloat), 9, elementCount, normalize)
{
  transpose_ = GL_FALSE;
  enableAttribute_ = &ShaderInput::enableAttributeMat3;
  enableUniform_ = &ShaderInput::enableUniformMat3;
}
ShaderInputMat4::ShaderInputMat4(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_FLOAT, sizeof(GLfloat), 16, elementCount, normalize)
{
  transpose_ = GL_FALSE;
  enableAttribute_ = &ShaderInput::enableAttributeMat4;
  enableUniform_ = &ShaderInput::enableUniformMat4;
}

ShaderInput1d::ShaderInput1d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_DOUBLE, sizeof(GLdouble), 1, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform1d;
}
ShaderInput2d::ShaderInput2d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_DOUBLE, sizeof(GLdouble), 2, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform2d;
}
ShaderInput3d::ShaderInput3d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_DOUBLE, sizeof(GLdouble), 3, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform3d;
}
ShaderInput4d::ShaderInput4d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_DOUBLE, sizeof(GLdouble), 4, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform4d;
}

ShaderInput1i::ShaderInput1i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_INT, sizeof(GLint), 1, elementCount, normalize)
{
  enableAttribute_ = &ShaderInput::enableAttributei;
  enableUniform_ = &ShaderInput::enableUniform1i;
}
ShaderInput2i::ShaderInput2i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_INT, sizeof(GLint), 2, elementCount, normalize)
{
  enableAttribute_ = &ShaderInput::enableAttributei;
  enableUniform_ = &ShaderInput::enableUniform2i;
}
ShaderInput3i::ShaderInput3i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_INT, sizeof(GLint), 3, elementCount, normalize)
{
  enableAttribute_ = &ShaderInput::enableAttributei;
  enableUniform_ = &ShaderInput::enableUniform3i;
}
ShaderInput4i::ShaderInput4i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_INT, sizeof(GLint), 4, elementCount, normalize)
{
  enableAttribute_ = &ShaderInput::enableAttributei;
  enableUniform_ = &ShaderInput::enableUniform4i;
}

ShaderInput1ui::ShaderInput1ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_UNSIGNED_INT, sizeof(GLuint), 1, elementCount, normalize)
{
  enableAttribute_ = &ShaderInput::enableAttributei;
  enableUniform_ = &ShaderInput::enableUniform1ui;
}
ShaderInput2ui::ShaderInput2ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_UNSIGNED_INT, sizeof(GLuint), 2, elementCount, normalize)
{
  enableAttribute_ = &ShaderInput::enableAttributei;
  enableUniform_ = &ShaderInput::enableUniform2ui;
}
ShaderInput3ui::ShaderInput3ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_UNSIGNED_INT, sizeof(GLuint), 3, elementCount, normalize)
{
  enableAttribute_ = &ShaderInput::enableAttributei;
  enableUniform_ = &ShaderInput::enableUniform3ui;
}
ShaderInput4ui::ShaderInput4ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputTyped(name, GL_UNSIGNED_INT, sizeof(GLuint), 4, elementCount, normalize)
{
  enableAttribute_ = &ShaderInput::enableAttributei;
  enableUniform_ = &ShaderInput::enableUniform4ui;
}

/////////////
/////////////
/////////////

VAO::VAO()
: GLObject(glGenVertexArrays, glDeleteVertexArrays)
{
}
