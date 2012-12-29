/*
 * vbo.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _VBO_H_
#define _VBO_H_

#include <list>

#include <ogle/gl-types/buffer-object.h>
#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/utility/ordered-stack.h>

/**
 * Macro for buffer access with offset.
 */
#ifndef BUFFER_OFFSET
  #define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif

/**
 * A block of data in the VBO allocated VRAM.
 */
struct VBOBlock {
  GLuint start;
  GLuint end;
  GLuint size;
  VBOBlock *left;
  VBOBlock *right;
  OrderedStack<VBOBlock*>::Node *node; // null if allocated
};
typedef list<VBOBlock*>::iterator VBOBlockIterator;

/**
 * Vertex Buffer Objects (VBOs) are Buffer Objects that are
 * used for vertex data. Since the storage for buffer objects
 * is allocated by OpenGL, vertex buffer objects are a mechanism
 * for storing vertex data in "fast" memory (i.e. video RAM or AGP RAM,
 * and in the case of PCI Express, video RAM or RAM),
 * thereby allowing for significant increases in vertex throughput
 * between the application and the GPU.
 *
 * Vertex data can be either vertex attributes and/or indices.
 */
class VertexBufferObject : public BufferObject
{
public:
  /**
   * Flag indicating the usage of the data in the vbo
   */
  enum Usage {
    USAGE_DYNAMIC = GL_DYNAMIC_DRAW,
    USAGE_STATIC = GL_STATIC_DRAW,
    USAGE_STREAM = GL_STREAM_DRAW
  };

  VertexBufferObject(Usage usage, GLuint bufferSize,
      GLenum initialTarget=GL_ARRAY_BUFFER);
  ~VertexBufferObject();

  void resize(GLuint bufferSize);

  /**
   * Returns the maximal unallocated contiguous
   * space of the VBO.
   */
  GLuint maxContiguousSpace() const;

  GLuint bufferSize() const;

  /**
   * Check if the VBO can allocate space
   * for the given buffer sizes.
   */
  GLboolean canAllocate(list<GLuint> &sizes, GLuint sizeSum);

  /**
   * Try to allocate space in this VBO for the given
   * attributes. Add the attributes interleaved to the vbo.
   */
  VBOBlockIterator allocateInterleaved(
      const list< ref_ptr<VertexAttribute> > &attributes);
  /**
   * Try to allocate space in this VBO for the given
   * attributes. Add the attributes sequential to the vbo.
   */
  VBOBlockIterator allocateSequential(
      const list< ref_ptr<VertexAttribute> > &attributes);
  /**
   * Free previously allocated space in the VBO.
   */
  void free(VBOBlockIterator &it);

  /**
   * usage is a performance hint.
   * provides info how the buffer object is going to be used:
   * static, dynamic or stream, and read, copy or draw
   */
  Usage usage() const;

  /**
   * Copy the vbo data to another buffer.
   * @param from the vbo handle containing the data
   * @param to the vbo handle to copy the data to
   * @param size size of data to copy in bytes
   * @param offset offset in data vbo
   * @param offset in destination vbo
   */
  static void copy(GLuint from, GLuint to, GLuint size, GLuint offset, GLuint toOffset);

  /**
  * Copy vertex data to the buffer object. Sets all data.
  * Make sure to bind before.
  * The Buffer will reserve the space requested.
  *
  * If data is NULL pointer, then VBO reserves only memory space with the given data size.
  * The NULL pointer also tells OpenGL that the data can be discarded. If you map after
  * setting a NULL pointer, you can work around map synchronizing issues. But only if
  * The complete buffer is updated.
  */
  void set_data(GLuint size, void *data);
  /**
  * Copy vertex data to the buffer object. Sets part of data.
  * Make sure to bind before.
  * Replaces only existing data, no new memory allocated for the buffer.
  */
  void set_data(GLuint offset, GLuint size, void *data);
  /**
   * Get the buffer data.
   */
  void data(GLuint offset, GLuint size, void *data);

  /**
  * map the buffer object into client's memory.
  * Make sure to bind before.
  *
  * If OpenGL is able to map the buffer object into client's address space,
  * map returns the pointer to the buffer. Otherwise it returns NULL.
  *
  * accessFlags can be one of GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE
  *
  * NOTE: causes synchronizing issues. Until mapped no gl* calls allowed.
  */
  GLvoid* map(GLenum accessFlags);
  /**
  * map a range of the buffer object into client's memory.
  * Make sure to bind before.
  *
  * If OpenGL is able to map the buffer object into client's address space,
  * map returns the pointer to the buffer. Otherwise it returns NULL.
  *
  * NOTE: causes synchronizing issues. Until mapped no gl* calls allowed.
  */
  GLvoid* map(GLuint offset, GLuint size, GLenum accessFlags);

  /**
  * Unmaps previously mapped data.
  */
  void unmap();

  /**
   * hook the buffer object with the corresponding ID.
   * Once bind is first called, VBO initializes the buffer with a zero-sized memory buffer.
   */
  void bind(GLenum target);

  /**
   * Calculates the struct size for the attributes in bytes.
   */
  static GLuint attributeStructSize(
      const list< ref_ptr<VertexAttribute> > &attributes);

protected:
  GLenum target_;
  Usage usage_;
  GLuint bufferSize_;

  OrderedStack<VBOBlock*> freeList_;
  list<VBOBlock*> allocatedBlocks_;

  VBOBlockIterator allocateBlock(GLuint blockSize);

  void addAttributesInterleaved(
      GLuint startByte,
      GLuint endByte,
      const list< ref_ptr<VertexAttribute> > &attributes);
  void addAttributesSequential(
      GLuint startByte,
      GLuint endByte,
      const list< ref_ptr<VertexAttribute> > &attributes);
};

#endif /* _VBO_H_ */
