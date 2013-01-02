/*
 * audio-stream.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef AUDIO_STREAM_H_
#define AUDIO_STREAM_H_

#include <ogle/av/av-stream.h>
#include <ogle/av/audio-source.h>
#include <ogle/av/audio-buffer.h>

#include <ogle/utility/ref-ptr.h>

/* Define the number of buffers and buffer size (in bytes) to use. 3 buffers is
 * a good amount (one playing, one ready to play, another being filled). 32256
 * is a good length per buffer, as it fits 1, 2, 4, 6, 7, 8, 12, 14, 16, 24,
 * 28, and 32 bytes-per-frame sizes. */
#define NUM_AUDIO_STREAM_BUFFERS 3
#define AUDIO_STREAM_BUFFER_SIZE 32256

class AudioStreamError : public runtime_error {
public:
  AudioStreamError(const string &message)
  : runtime_error(message)
  {}
};

/**
 * libav stream that provides OpenAL audio source.
 */
class AudioStream : public AudioVideoStream
{
public:
  AudioStream(AVStream *stream, int index, unsigned int chachedBytesLimit);
  virtual ~AudioStream();

  /**
   * OpenAL audio source.
   */
  const ref_ptr<AudioSource>& audioSource();

  // override
  virtual void decode(AVPacket *packet);
  virtual void clearQueue();

protected:
  static int64_t basetime_;
  static int64_t filetime_;

  ref_ptr<AudioSource> audioSource_;
  ALenum alType_;
  ALenum alChannelLayout_;
  ALenum alFormat_;
  ALint rate_;

};

#endif /* AUDIO_STREAM_H_ */
