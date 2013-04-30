/*
 * mesh-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef __MESH_H_
#define __MESH_H_

#include <regen/states/shader-input-state.h>
#include <regen/states/feedback-state.h>
#include <regen/gl-types/vertex-attribute.h>
#include <regen/gl-types/vbo.h>

namespace regen {
/**
 * \brief A collection of vertices, edges and faces that defines the shape of an object in 3D space.
 *
 * When this State is enabled the actual draw call is done. Make sure to setup shader
 * and server side states before.
 */
class Mesh : public ShaderInputState
{
public:
  /**
   * @param primitive face primitive of this mesh.
   * @param useAutoUpload automatically upload data to GL when setInput is called.
   * @param usage VBO usage.
   */
  Mesh(GLenum primitive,
      GLboolean useAutoUpload=GL_TRUE,
      VertexBufferObject::Usage usage=VertexBufferObject::USAGE_DYNAMIC);

  /**
   * @return face primitive of this mesh.
   */
  GLenum primitive() const;
  /**
   * @param primitive face primitive of this mesh.
   */
  void set_primitive(GLenum primitive);

  /**
   * Sets the index attribute.
   * @param indices the index attribute.
   * @param maxIndex maximal index in the index array.
   */
  void setIndices(const ref_ptr<VertexAttribute> &indices, GLuint maxIndex);

  /**
   * @return number of indices to vertex data.
   */
  GLuint numIndices() const;
  /**
   * @return the maximal index in the index buffer.
   */
  GLuint maxIndex();
  /**
   * @return indexes to the vertex data of this primitive set.
   */
  const ref_ptr<VertexAttribute>& indices() const;
  /**
   * @return index buffer used by this mesh.
   */
  GLuint indexBuffer() const;

  /**
   * @return the position attribute.
   */
  ref_ptr<ShaderInput> positions() const;
  /**
   * @return the normal attribute.
   */
  ref_ptr<ShaderInput> normals() const;
  /**
   * @return the color attribute.
   */
  ref_ptr<ShaderInput> colors() const;

  /**
   * Add attributes to capture to this state.
   * @return the transform feedback state.
   */
  const ref_ptr<FeedbackState>& feedbackState();

  /**
   * Render primitives from array data.
   */
  void draw(GLuint numInstances);

  // override
  virtual void enable(RenderState*);

protected:
  GLenum primitive_;
  GLuint numIndices_;
  GLuint maxIndex_;
  ref_ptr<VertexAttribute> indices_;

  GLenum feedbackPrimitive_;

  ref_ptr<FeedbackState> feedbackState_;
  GLuint feedbackCount_;

  void (Mesh::*draw_)(GLuint numInstances);
  void drawArrays(GLuint numInstances);
  void drawElements(GLuint numInstances);
};
} // namespace

#endif /* __MESH_H_ */
