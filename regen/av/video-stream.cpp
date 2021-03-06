/*
 * video-stream.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

extern "C" {
  #include <libswscale/swscale.h>
}

#include <regen/utility/logging.h>

#include "video-stream.h"
using namespace regen;

#define GL_RGB_PIXEL_FORMAT PIX_FMT_RGB24

VideoStream::VideoStream(AVStream *stream, GLint index, GLuint chachedBytesLimit)
: AudioVideoStream(stream, index, chachedBytesLimit)
{
  stream_ = stream;
  width_ = codecCtx_->width;
  height_ = codecCtx_->height;
  if(width_<1 || height_<1) throw new Error("invalid video size");

  REGEN_DEBUG("init video stream" <<
      " width=" << width_ <<
      " height=" << height_ <<
      " pix_fmt=" << codecCtx_->pix_fmt <<
      " bit_rate=" << codecCtx_->bit_rate <<
      ".");

  // get sws context for converting from YUV to RGB
  swsCtx_ = sws_getContext(
      codecCtx_->width,
      codecCtx_->height,
      codecCtx_->pix_fmt,
      codecCtx_->width,
      codecCtx_->height,
      GL_RGB_PIXEL_FORMAT,
      SWS_FAST_BILINEAR,
      NULL, NULL, NULL);
  currFrame_ = avcodec_alloc_frame();
}
VideoStream::~VideoStream()
{
  clearQueue();
  av_free(currFrame_);
}

void VideoStream::clearQueue()
{
  while(decodedFrames_.size()>0) {
    AVFrame *f = frontFrame();
    popFrame();
    delete (float*)f->opaque;
    av_free(f);
  }
}

void VideoStream::decode(AVPacket *packet)
{
  int frameFinished = 0;
  // Decode video frame
  avcodec_decode_video2(codecCtx_, currFrame_, &frameFinished, packet);
  // Did we get a video frame?
  if(!frameFinished) return;
  // YUV to RGB conversation. could be done on GPU with pixel shader. Maybe later....
  AVFrame *rgb = avcodec_alloc_frame();
  int numBytes = avpicture_get_size(
      GL_RGB_PIXEL_FORMAT,
      codecCtx_->width,
      codecCtx_->height);
  if(numBytes < 1) {
    av_free(rgb);
    av_free(currFrame_);
    currFrame_ = avcodec_alloc_frame();
    return;
  }
  uint8_t *buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
  numBytes = avpicture_fill(
      (AVPicture *)rgb,
      buffer,
      GL_RGB_PIXEL_FORMAT,
      codecCtx_->width,
      codecCtx_->height);
  if(numBytes < 1) {
    av_free(rgb);
    av_free(buffer);
    av_free(currFrame_);
    currFrame_ = avcodec_alloc_frame();
    return;
  }
  sws_scale(swsCtx_,
      currFrame_->data,
      currFrame_->linesize,
      0,
      codecCtx_->height,
      rgb->data,
      rgb->linesize);
  // remember timestamp in frame
  float *dt = new float;
  *dt = packet->dts*av_q2d(stream_->time_base);
  rgb->opaque = dt;
  // free package and put the frame in queue of decoded frames
  av_free_packet(packet);
  av_free(currFrame_);
  currFrame_ = avcodec_alloc_frame();
  pushFrame(rgb, numBytes);
}

GLint VideoStream::width() const
{ return width_; }
GLint VideoStream::height() const
{ return height_; }

AVStream *VideoStream::stream()
{ return stream_; }

GLenum VideoStream::texInternalFormat() const
{ return GL_RGB; }
GLenum VideoStream::texFormat() const
{ return GL_RGB; }
GLenum VideoStream::texPixelType() const
{ return GL_UNSIGNED_BYTE; }
