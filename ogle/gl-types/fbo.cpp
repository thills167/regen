/*
 * fbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "fbo.h"

FrameBufferObject::FrameBufferObject(
    GLuint width, GLuint height,
    GLenum depthAttachmentFormat)
: RectBufferObject(glGenFramebuffers, glDeleteFramebuffers),
  depthAttachmentFormat_(depthAttachmentFormat)
{
  set_size(width,height);
  bind();
  if(depthAttachmentFormat_!=GL_NONE) {
    depthTexture_ = ref_ptr<DepthTexture2D>::manage(new DepthTexture2D);
    depthTexture_->set_size(width_, height_);
    depthTexture_->set_internalFormat(depthAttachmentFormat_);
    depthTexture_->bind();
    depthTexture_->set_wrapping(GL_REPEAT);
    depthTexture_->set_filter(GL_LINEAR, GL_LINEAR);
    depthTexture_->set_compare(GL_NONE, GL_EQUAL);
    depthTexture_->set_depthMode(GL_LUMINANCE);
    depthTexture_->texImage();
    set_depthAttachment(*depthTexture_.get());
  }
}

ref_ptr<DepthTexture2D> FrameBufferObject::depthTexture()
{
  return depthTexture_;
}
GLenum FrameBufferObject::depthAttachmentFormat() const
{
  return depthAttachmentFormat_;
}
list< ref_ptr<Texture> >& FrameBufferObject::colorBuffer()
{
  return colorBuffer_;
}
ref_ptr<Texture>& FrameBufferObject::firstColorBuffer()
{
  return colorBuffer_.front();
}

ref_ptr<Texture> FrameBufferObject::addRectangleTexture(GLuint count,
    GLenum format, GLenum internalFormat)
{
  ref_ptr<Texture> tex = ref_ptr<Texture>::manage(new TextureRectangle(count));
  tex->set_size(width_, height_);
  tex->set_format(format);
  tex->set_internalFormat(internalFormat);
  for(GLint j=0; j<count; ++j) {
    tex->bind();
    tex->set_wrapping(GL_REPEAT);
    tex->set_filter(GL_LINEAR, GL_LINEAR);
    tex->texImage();
    addColorAttachment(*tex.get());
    tex->nextBuffer();
  }
  colorBuffer_.push_back(tex);
  return tex;
}

ref_ptr<Texture> FrameBufferObject::addTexture(GLuint count,
    GLenum format, GLenum internalFormat)
{
  ref_ptr<Texture> tex = ref_ptr<Texture>::manage(new Texture2D(count));
  tex->set_size(width_, height_);
  tex->set_format(format);
  tex->set_internalFormat(internalFormat);
  for(GLint j=0; j<count; ++j) {
    tex->bind();
    tex->set_wrapping(GL_CLAMP_TO_EDGE);
    tex->set_filter(GL_LINEAR, GL_LINEAR);
    tex->texImage();
    addColorAttachment(*tex.get());
    tex->nextBuffer();
  }
  colorBuffer_.push_back(tex);
  return tex;
}

ref_ptr<RenderBufferObject> FrameBufferObject::addRenderBuffer(GLuint count)
{
  ref_ptr<RenderBufferObject> rbo = ref_ptr<RenderBufferObject>::manage(new RenderBufferObject(count));
  rbo->set_size(width_, height_);
  for(GLint j=0; j<count; ++j) {
    rbo->bind();
    rbo->storage();
    addColorAttachment(*rbo.get());
    rbo->nextBuffer();
  }
  renderBuffer_.push_back(rbo);
  return rbo;
}

void FrameBufferObject::resize(
    GLuint width, GLuint height)
{
  set_size(width, height);
  bind();
  if(depthTexture_.get()!=NULL) {
    depthTexture_->set_size(width_, height_);
    depthTexture_->bind();
    depthTexture_->texImage();
  }
  for(list< ref_ptr<Texture> >::iterator
      it=colorBuffer_.begin(); it!=colorBuffer_.end(); ++it)
  {
    ref_ptr<Texture> &tex = *it;
    tex->set_size(width_, height_);
    for(GLuint i=0; i<tex->numBuffers(); ++i)
    {
      tex->bind();
      tex->texImage();
      tex->nextBuffer();
    }
  }
  for(list< ref_ptr<RenderBufferObject> >::iterator
      it=renderBuffer_.begin(); it!=renderBuffer_.end(); ++it)
  {
    ref_ptr<RenderBufferObject> &rbo = *it;
    rbo->set_size(width_, height_);
    for(GLuint i=0; i<rbo->numBuffers(); ++i)
    {
      rbo->bind();
      rbo->storage();
      rbo->nextBuffer();
    }
  }
}
