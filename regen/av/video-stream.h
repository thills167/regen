/*
 * video-stream.h
 *
 *  Created on: 08.04.2012
 *      Author: daniel
 */

#ifndef VIDEO_STREAM_H_
#define VIDEO_STREAM_H_

#include <GL/glew.h>
#include <regen/av/av-stream.h>

namespace regen {
  /**
   * \brief ffmpeg stream that provides texture data for GL texture.
   */
  class VideoStream : public AudioVideoStream
  {
  public:
    /**
     * @param stream the stream object.
     * @param index the stream index.
     * @param chachedBytesLimit size limit for pre loading.
     */
    VideoStream(AVStream *stream, GLint index, GLuint chachedBytesLimit);
    virtual ~VideoStream();

    /**
     * The stream handle as provided to the constructor.
     */
    AVStream *stream();

    /**
     * Video width in pixels.
     */
    GLint width() const;
    /**
     * Video height in pixels.
     */
    GLint height() const;

    /**
     * Format for GL texture to match frame data.
     */
    GLenum texInternalFormat() const;
    /**
     * Format for GL texture to match frame data.
     */
    GLenum texFormat() const;
    /**
     * Pixel type for GL texture to match frame data.
     */
    GLenum texPixelType() const;

    // override
    void decode(AVPacket *packet);
    void clearQueue();

  protected:
    struct SwsContext *swsCtx_;
    AVFrame *currFrame_;

    AVStream *stream_;
    GLint width_, height_;
  };
} // namespace

#endif /* VIDEO_STREAM_H_ */
