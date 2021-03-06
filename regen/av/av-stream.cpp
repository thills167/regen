/*
 * av-stream.cpp
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#include <regen/config.h>
#include <regen/utility/threading.h>

#include "av-stream.h"
using namespace regen;

AudioVideoStream::AudioVideoStream(AVStream *stream, GLint index,  GLuint chachedBytesLimit)
: stream_(NULL),
  index_(-1),
  cachedBytes_(0),
  chachedBytesLimit_(chachedBytesLimit),
  isActive_(GL_FALSE)
{
  open(stream,index,GL_TRUE);
}
AudioVideoStream::AudioVideoStream(GLuint chachedBytesLimit)
: stream_(NULL),
  index_(-1),
  cachedBytes_(0),
  chachedBytesLimit_(chachedBytesLimit),
  isActive_(GL_FALSE)
{
}

GLint AudioVideoStream::index() const
{ return index_; }
AVCodecContext* AudioVideoStream::codec() const
{ return codecCtx_; }

void AudioVideoStream::setInactive()
{
  isActive_ = GL_FALSE;
}

void AudioVideoStream::open(AVStream *stream, GLint index, GLboolean initial)
{
  if(!initial) {
    clearQueue();
  }
  // Get a pointer to the codec context for the video stream
  codecCtx_ = stream->codec;

  // Find the decoder for the video stream
  codec_ = avcodec_find_decoder(codecCtx_->codec_id);
  if(codec_ == NULL)
  {
    throw new Error("Unsupported codec!");
  }
  // Open codec
  if(avcodec_open2(codecCtx_, codec_, NULL) < 0)
  {
    throw new Error("Could not open codec.");
  }
  stream_ = stream;
  index_ = index;
  cachedBytes_ = 0;
  isActive_ = GL_TRUE;
}

GLuint AudioVideoStream::numFrames()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  return decodedFrames_.size();
}

void AudioVideoStream::pushFrame(AVFrame *frame, GLuint frameSize)
{
  {
    boost::lock_guard<boost::mutex> lock(decodingLock_);
    cachedBytes_ += frameSize;
  }
  GLuint cachedBytes, numCachedFrames;
  if(chachedBytesLimit_ > 0.0f) {
    while(isActive_) {
      cachedBytes = cachedBytes_;
      numCachedFrames = decodedFrames_.size();
      if(cachedBytes < chachedBytesLimit_ || numCachedFrames<3) {
        break;
      }
      else {
        usleepRegen(20000);
      }
    }
  }
  if(isActive_) {
    boost::lock_guard<boost::mutex> lock(decodingLock_);
    decodedFrames_.push(frame);
    frameSizes_.push(frameSize);
  }
}

AVFrame* AudioVideoStream::frontFrame()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  return decodedFrames_.front();
}

void AudioVideoStream::popFrame()
{
  boost::lock_guard<boost::mutex> lock(decodingLock_);
  decodedFrames_.pop();
  cachedBytes_ -= frameSizes_.front();
  frameSizes_.pop();
}
