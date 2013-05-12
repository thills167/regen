/*
 * fbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/gl-types/render-state.h>
#include <regen/gl-types/gl-util.h>
#include <regen/config.h>

#include "fbo.h"
using namespace regen;

static inline void __DrawBuffers(const DrawBuffers &v)
{ glDrawBuffers(v.buffers_.size(),&v.buffers_[0]); }
#ifdef WIN32
static inline void __DrawBuffer(GLenum v)
{ glDrawBuffer(v); }
static inline void __ReadBuffer(GLenum v)
{ glReadBuffer(v); }
#else
#define __DrawBuffer glDrawBuffer
#define __ReadBuffer glReadBuffer
#endif

static inline void attachTexture(
    const ref_ptr<Texture> &tex, GLenum target)
{
  glFramebufferTextureEXT(GL_DRAW_FRAMEBUFFER,
      target, tex->id(), 0);
}
static inline void attachRenderBuffer(
    const ref_ptr<RenderBuffer> &rbo, GLenum target)
{
  glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
      target, GL_RENDERBUFFER, rbo->id());
}

FBO::Screen::Screen()
: drawBuffer_(__DrawBuffer),
  readBuffer_(__ReadBuffer)
{
  RenderState *rs = RenderState::get();
  rs->drawFrameBuffer().push(0);
  drawBuffer_.push(GL_FRONT);
  rs->drawFrameBuffer().pop();
}

FBO::FBO(GLuint width, GLuint height, GLuint depth)
: GLRectangle(glGenFramebuffers, glDeleteFramebuffers),
  drawBuffers_(__DrawBuffers),
  readBuffer_(__ReadBuffer),
  depthAttachmentTarget_(GL_NONE),
  depthAttachmentFormat_(GL_NONE),
  depthAttachmentType_(GL_NONE)
{
  RenderState *rs = RenderState::get();
  GL_ERROR_LOG();
  set_rectangleSize(width,height);
  depth_ = depth;

  viewport_ = ref_ptr<ShaderInput2f>::alloc("viewport");
  viewport_->setUniformData(
      Vec2f((GLfloat)width, (GLfloat)height));
  inverseViewport_ = ref_ptr<ShaderInput2f>::alloc("inverseViewport");
  inverseViewport_->setUniformData(
      Vec2f(1.0/(GLfloat)width, 1.0/(GLfloat)height));
  glViewport_ = Vec4ui(0,0,width,height);

  rs->readFrameBuffer().push(id());
  readBuffer_.push(GL_COLOR_ATTACHMENT0);
  rs->readFrameBuffer().pop();
  GL_ERROR_LOG();
}

void FBO::set_depthAttachment(const ref_ptr<Texture> &tex)
{
  attachTexture(tex, GL_DEPTH_ATTACHMENT);
  depthTexture_ = tex;
}
void FBO::set_depthAttachment(const ref_ptr<RenderBuffer> &rbo)
{
  attachRenderBuffer(rbo, GL_DEPTH_ATTACHMENT);
  depthTexture_ = ref_ptr<Texture>();
}

void FBO::set_stencilTexture(const ref_ptr<Texture> &tex)
{
  attachTexture(tex, GL_STENCIL_ATTACHMENT);
  stencilTexture_ = tex;
}
void FBO::set_stencilTexture(const ref_ptr<RenderBuffer> &rbo)
{
  attachRenderBuffer(rbo, GL_STENCIL_ATTACHMENT);
  stencilTexture_ = ref_ptr<Texture>();
}

void FBO::set_depthStencilTexture(const ref_ptr<Texture> &tex)
{
  attachTexture(tex, GL_DEPTH_STENCIL_ATTACHMENT);
  depthStencilTexture_ = tex;
}
void FBO::set_depthStencilTexture(const ref_ptr<RenderBuffer> &rbo)
{
  attachRenderBuffer(rbo, GL_DEPTH_STENCIL_ATTACHMENT);
  depthStencilTexture_ = ref_ptr<Texture>();
}

void FBO::createDepthTexture(GLenum target, GLenum format, GLenum type)
{
  RenderState *rs = RenderState::get();
  depthAttachmentTarget_ = target;
  depthAttachmentFormat_ = format;
  depthAttachmentType_ = type;

  ref_ptr<Texture> depth;
  if(target == GL_TEXTURE_CUBE_MAP) {
    depth = ref_ptr<TextureCubeDepth>::alloc();
  }
  else if(depth_>1) {
    ref_ptr<Texture3DDepth> depth3D = ref_ptr<Texture3DDepth>::alloc();
    depth3D->set_depth(depth_);
    depth = depth3D;
  }
  else {
    depth = ref_ptr<Texture2DDepth>::alloc();
  }
  depth->set_targetType(target);
  depth->set_rectangleSize(width_, height_);
  depth->set_internalFormat(format);
  depth->set_pixelType(type);

  rs->drawFrameBuffer().push(id()); {
    depth->begin(rs); {
      depth->set_wrapping(GL_REPEAT);
      depth->set_filter(GL_LINEAR, GL_LINEAR);
      depth->set_compare(GL_NONE, GL_EQUAL);
      depth->texImage();
    } depth->end(rs);
    set_depthAttachment(depth);
  } rs->drawFrameBuffer().pop();
}

GLenum FBO::addTexture(const ref_ptr<Texture> &tex)
{
  RenderState *rs = RenderState::get();

  GLenum attachment = GL_COLOR_ATTACHMENT0 + colorBuffers_.buffers_.size();
  colorBuffers_.buffers_.push_back(attachment);
  colorTextures_.push_back(tex);

  rs->drawFrameBuffer().push(id());
  attachTexture(tex, attachment);
  rs->drawFrameBuffer().pop();

  return attachment;
}
ref_ptr<Texture> FBO::addTexture(
    GLuint count,
    GLenum targetType,
    GLenum format,
    GLenum internalFormat,
    GLenum pixelType)
{
  RenderState *rs = RenderState::get();
  ref_ptr<Texture> tex;
  ref_ptr<Texture3D> tex3d;

  switch(targetType) {
  case GL_TEXTURE_RECTANGLE:
    tex = ref_ptr<TextureRectangle>::alloc(count);
    break;

  case GL_TEXTURE_2D_ARRAY:
    tex3d = ref_ptr<Texture2DArray>::alloc(count);
    tex3d->set_depth(depth_);
    tex = tex3d;
    break;

  case GL_TEXTURE_CUBE_MAP:
    tex = ref_ptr<TextureCube>::alloc(count);
    break;

  case GL_TEXTURE_3D:
    tex3d = ref_ptr<Texture3D>::alloc(count);
    tex3d->set_depth(depth_);
    tex = tex3d;
    break;

  case GL_TEXTURE_2D:
    tex = ref_ptr<Texture2D>::alloc(count);
    break;

  default: // GL_TEXTURE_2D:
    REGEN_WARN("Unknown texture type " << targetType << ". Using 2D texture.");
    tex = ref_ptr<Texture2D>::alloc(count);
    break;

  }

  tex->set_rectangleSize(width_, height_);
  tex->set_format(format);
  tex->set_internalFormat(internalFormat);
  tex->set_pixelType(pixelType);
  rs->activeTexture().push(GL_TEXTURE7);
  for(GLuint j=0; j<count; ++j) {
    rs->textures().push(7, tex->textureBind());
    tex->set_wrapping(GL_CLAMP_TO_EDGE);
    tex->set_filter(GL_LINEAR, GL_LINEAR);
    tex->texImage();
    addTexture(tex);
    rs->textures().pop(7);
    tex->nextObject();
  }
  rs->activeTexture().pop();

  return tex;
}

GLenum FBO::addRenderBuffer(const ref_ptr<RenderBuffer> &rbo)
{
  RenderState *rs = RenderState::get();

  GLenum attachment = GL_COLOR_ATTACHMENT0 + colorBuffers_.buffers_.size();
  colorBuffers_.buffers_.push_back(attachment);
  renderBuffers_.push_back(rbo);

  rs->drawFrameBuffer().push(id());
  attachRenderBuffer(rbo, attachment);
  rs->drawFrameBuffer().pop();

  return attachment;
}
ref_ptr<RenderBuffer> FBO::addRenderBuffer(GLuint count)
{
  RenderState *rs = RenderState::get();
  ref_ptr<RenderBuffer> rbo = ref_ptr<RenderBuffer>::alloc(count);

  rbo->set_rectangleSize(width_, height_);
  for(GLuint j=0; j<count; ++j) {
    rbo->begin(rs);
    rbo->storage();
    rbo->end(rs);
    addRenderBuffer(rbo);
    rbo->nextObject();
  }

  return rbo;
}

void FBO::blitCopy(
    FBO &dst,
    GLenum readAttachment,
    GLenum writeAttachment,
    GLbitfield mask,
    GLenum filter)
{
  RenderState *rs = RenderState::get();
  // read from this
  rs->readFrameBuffer().push(id());
  readBuffer_.push(readAttachment);
  // write to dst
  rs->drawFrameBuffer().push(dst.id());
  dst.drawBuffers().push(DrawBuffers(writeAttachment));

  glBlitFramebuffer(
      0, 0, width(), height(),
      0, 0, dst.width(), dst.height(),
      mask, filter);

  dst.drawBuffers().pop();
  rs->drawFrameBuffer().pop();
  readBuffer_.pop();
  rs->readFrameBuffer().pop();
}

void FBO::blitCopyToScreen(
    GLuint screenWidth, GLuint screenHeight,
    GLenum readAttachment,
    GLbitfield mask,
    GLenum filter)
{
  RenderState *rs = RenderState::get();
  // read from this
  rs->readFrameBuffer().push(id());
  readBuffer_.push(readAttachment);
  // write to screen front buffer
  rs->drawFrameBuffer().push(0);
  screen().drawBuffer_.push(GL_FRONT);

  glBlitFramebuffer(
      0, 0, width(), height(),
      0, 0, screenWidth, screenHeight,
      mask, filter);

  rs->drawFrameBuffer().pop();
  readBuffer_.pop();
  rs->readFrameBuffer().pop();
  screen().drawBuffer_.pop();
}

void FBO::resize(GLuint width, GLuint height, GLuint depth)
{
  RenderState *rs = RenderState::get();
  set_rectangleSize(width, height);
  depth_ = depth;

  viewport_->setUniformData(
      Vec2f( (GLfloat)width, (GLfloat)height));
  inverseViewport_->setUniformData(
      Vec2f( 1.0/(GLfloat)width, 1.0/(GLfloat)height));
  glViewport_ = Vec4ui(0,0,width,height);
  rs->drawFrameBuffer().push(id());
  rs->activeTexture().push(GL_TEXTURE7);

  // resize depth attachment
  if(depthTexture_.get()!=NULL) {
    depthTexture_->set_rectangleSize(width_, height_);
    Texture3D *tex3D = dynamic_cast<Texture3D*>(depthTexture_.get());
    if(tex3D!=NULL)
    { tex3D->set_depth(depth); }
    rs->textures().push(7, depthTexture_->textureBind());
    depthTexture_->texImage();
    rs->textures().pop(7);
  }

  // resize stencil attachment
  if(stencilTexture_.get()!=NULL) {
    stencilTexture_->set_rectangleSize(width_, height_);
    Texture3D *tex3D = dynamic_cast<Texture3D*>(stencilTexture_.get());
    if(tex3D!=NULL)
    { tex3D->set_depth(depth); }
    rs->textures().push(7, stencilTexture_->textureBind());
    stencilTexture_->texImage();
    rs->textures().pop(7);
  }

  // resize depth stencil attachment
  if(depthStencilTexture_.get()!=NULL) {
    depthStencilTexture_->set_rectangleSize(width_, height_);
    Texture3D *tex3D = dynamic_cast<Texture3D*>(depthStencilTexture_.get());
    if(tex3D!=NULL)
    { tex3D->set_depth(depth); }
    rs->textures().push(7, depthStencilTexture_->textureBind());
    depthStencilTexture_->texImage();
    rs->textures().pop(7);
  }

  // resize color attachments
  for(vector< ref_ptr<Texture> >::iterator
      it=colorTextures_.begin(); it!=colorTextures_.end(); ++it)
  {
    ref_ptr<Texture> &tex = *it;
    tex->set_rectangleSize(width_, height_);
    Texture3D *tex3D = dynamic_cast<Texture3D*>(tex.get());
    if(tex3D!=NULL)
    { tex3D->set_depth(depth); }
    for(GLuint i=0; i<tex->numObjects(); ++i)
    {
      rs->textures().push(7, tex->textureBind());
      tex->texImage();
      rs->textures().pop(7);
      tex->nextObject();
    }
  }

  // resize rbo attachments
  for(vector< ref_ptr<RenderBuffer> >::iterator
      it=renderBuffers_.begin(); it!=renderBuffers_.end(); ++it)
  {
    ref_ptr<RenderBuffer> &rbo = *it;
    rbo->set_rectangleSize(width_, height_);
    for(GLuint i=0; i<rbo->numObjects(); ++i)
    {
      rbo->begin(rs);
      rbo->storage();
      rbo->end(rs);
      rbo->nextObject();
    }
  }

  rs->activeTexture().pop();
  rs->drawFrameBuffer().pop();
}

GLuint FBO::depth() const
{ return depth_; }
GLenum FBO::depthAttachmentFormat() const
{ return depthAttachmentFormat_; }

const DrawBuffers& FBO::colorBuffers()
{ return colorBuffers_; }

vector< ref_ptr<Texture> >& FBO::colorTextures()
{ return colorTextures_; }
const ref_ptr<Texture>& FBO::firstColorTexture() const
{ return colorTextures_.front(); }
vector< ref_ptr<RenderBuffer> >& FBO::renderBuffers()
{ return renderBuffers_; }
const ref_ptr<Texture>& FBO::depthTexture() const
{ return depthTexture_; }
const ref_ptr<Texture>& FBO::stencilTexture() const
{ return stencilTexture_; }
const ref_ptr<Texture>& FBO::depthStencilTexture() const
{ return depthStencilTexture_; }

const ref_ptr<ShaderInput2f>& FBO::viewport() const
{ return viewport_; }
const Vec4ui& FBO::glViewport() const
{ return glViewport_; }
const ref_ptr<ShaderInput2f>& FBO::inverseViewport() const
{ return inverseViewport_; }

///////////////
///////////////
///////////////

RenderBuffer::RenderBuffer(GLuint numBuffers)
: GLRectangle(glGenRenderbuffers, glDeleteRenderbuffers, numBuffers),
  format_(GL_RGBA)
{
}

void RenderBuffer::set_format(GLenum format)
{ format_ = format; }

void RenderBuffer::begin(RenderState *rs)
{ rs->renderBuffer().push(id()); }
void RenderBuffer::end(RenderState *rs)
{ rs->renderBuffer().pop(); }

void RenderBuffer::storageMS(GLuint numMultisamples) const
{
  glRenderbufferStorageMultisample(
      GL_RENDERBUFFER, numMultisamples, format_, width_, height_);
}

void RenderBuffer::storage() const
{
  glRenderbufferStorage(
      GL_RENDERBUFFER, format_, width_, height_);
}
