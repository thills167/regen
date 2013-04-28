/*
 * shader-input-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef SHADER_INPUT_STATE_H_
#define SHADER_INPUT_STATE_H_

#include <regen/states/state.h>
#include <regen/gl-types/shader-input.h>
#include <regen/gl-types/vbo.h>

namespace regen {
/**
 * \brief Container State for shader input data.
 */
class ShaderInputState : public State
{
public:
  /**
   * \brief ShaderInput plus optional name overwrite.
   */
  struct Named {
    /**
     * @param in the shader input data.
     * @param name the name overwrite.
     */
    Named(ref_ptr<ShaderInput> in, const string &name)
    : in_(in), name_(name) {}
    /** the shader input data. */
    ref_ptr<ShaderInput> in_;
    /** the name overwrite. */
    string name_;
  };
  /**
   * ShaderInput container.
   */
  typedef list<Named> InputContainer;
  /**
   * ShaderInput container iterator.
   */
  typedef InputContainer::const_iterator InputItConst;
  /**
   * ShaderInput container iterator.
   */
  typedef InputContainer::iterator InputIt;

  ShaderInputState(VertexBufferObject::Usage usage=VertexBufferObject::USAGE_DYNAMIC);
  /**
   * @param in shader input data.
   * @param name shader input name overwrite.
   */
  ShaderInputState(const ref_ptr<ShaderInput> &in, const string &name="",
      VertexBufferObject::Usage usage=VertexBufferObject::USAGE_DYNAMIC);
  ~ShaderInputState();

  VertexBufferObject& inputBuffer() const;

  /**
   * Auto add to VBO when setInput() is called ?
   * If you need any special attribute layout you should set this to false.
   * Initially it's true.
   */
  void set_useAutoUpload(GLboolean v);
  GLboolean useAutoUpload() const;

  /**
   * @return Number of vertices of added input data.
   */
  GLuint numVertices() const;
  /**
   * @return Number of instances of added input data.
   */
  GLuint numInstances() const;

  /**
   * @return Previously added shader inputs.
   */
  const InputContainer& inputs() const;

  /**
   * @param name the shader input name.
   * @return true if an input data with given name was added before.
   */
  GLboolean hasInput(const string &name) const;

  /**
   * @param name the shader input name.
   * @return input data with specified name.
   */
  ref_ptr<ShaderInput> getInput(const string &name) const;

  /**
   * @param in the shader input data.
   * @param name the shader input name.
   * @return iterator of data container
   */
  InputItConst setInput(const ref_ptr<ShaderInput> &in, const string &name="");

protected:
  InputContainer inputs_;
  set<string> inputMap_;
  GLuint numVertices_;
  GLuint numInstances_;

  GLboolean useAutoUpload_;
  ref_ptr<VertexBufferObject> inputBuffer_;

  void removeInput(const string &name);
  void removeInput(const ref_ptr<ShaderInput> &att);
};

} // namespace

#endif /* ATTRIBUTE_STATE_H_ */
