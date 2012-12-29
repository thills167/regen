/*
 * buffer-object.h
 *
 *  Created on: 09.12.2011
 *      Author: daniel
 */

#ifndef BUFFER_OBJECT_H_
#define BUFFER_OBJECT_H_

#include <GL/glew.h>
#include <GL/gl.h>
using namespace std;

/**
 * Base class for buffer objects (FBO/VBO/...).
 * Each buffer can generate multiple gl buffers,
 * the active buffer can be changed with nextBuffer()
 * and set_bufferIndex().
 */
class BufferObject
{
public:
  /**
   * Obtain n buffers.
   */
  typedef void (*CreateBufferFunc)(GLsizei,GLuint*);
  /**
   * Release n buffers.
   */
  typedef void (*ReleaseBufferFunc)(GLsizei,const GLuint*);

  BufferObject(
      CreateBufferFunc createBuffers,
      ReleaseBufferFunc releaseBuffers,
      GLuint numBuffers=1);
  ~BufferObject();

  /**
   * Switch to the next allocated buffer.
   * Next bind() call will bind the activated buffer.
   */
  void nextBuffer();
  /**
   * Returns the currently active buffer index.
   */
  GLuint bufferIndex() const;
  /**
   * Sets the index of the active buffer.
   */
  void set_bufferIndex(GLuint bufferIndex);
  /**
   * Returns number of buffers allocation
   * for this Bufferobject.
   */
  GLuint numBuffers() const;
  /**
   * GL handle for currently active buffer.
   */
  GLuint id() const;
  /**
   * Array of GL handles allocated for this buffer.
   */
  GLuint* ids() const;
protected:
  GLuint *ids_;
  GLuint numBuffers_;
  GLuint bufferIndex_;
  ReleaseBufferFunc releaseBuffers_;

  /**
   * copy not allowed.
   */
  BufferObject(const BufferObject &other) {}
  BufferObject& operator=(const BufferObject &other) { return *this; }
};

/**
 * A two dimensional rectangular buffer.
 */
class RectBufferObject : public BufferObject {
public:
  RectBufferObject(
      CreateBufferFunc createBuffers,
      ReleaseBufferFunc releaseBuffers,
      GLuint numBuffers=1);
  /**
   * Set the buffer size.
   */
  void set_size(GLuint width, GLuint height);
  /**
   * Width of the buffer.
   */
  GLuint width() const;
  /**
   * Height of the buffer.
   */
  GLuint height() const;
protected:
  GLuint width_;
  GLuint height_;
};

#endif /* BUFFER_OBJECT_H_ */
