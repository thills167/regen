/*
 * bones.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/states/texture-state.h>

#include "bones.h"
using namespace regen;

Bones::Bones(list< ref_ptr<AnimationNode> > &bones, GLuint numBoneWeights)
: State(), Animation(GL_TRUE,GL_FALSE), bones_(bones)
{
  // vbo holding 4 rgba values for each bone matrix
  boneMatrixVBO_ = ref_ptr<VertexBufferObject>::manage(new VertexBufferObject(
      VertexBufferObject::USAGE_DYNAMIC,
      sizeof(GLfloat)*16*bones_.size()));
  // attach vbo to texture
  boneMatrixTex_ = ref_ptr<TextureBufferObject>::manage(
      new TextureBufferObject(GL_RGBA32F));
  boneMatrixTex_->startConfig();
  boneMatrixTex_->attach(boneMatrixVBO_);
  boneMatrixTex_->stopConfig();

  // and make the tbo available
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(boneMatrixTex_), "boneMatrices"));
  texState->set_mapping(TextureState::MAPPING_CUSTOM);
  texState->set_mapTo(TextureState::MAP_TO_CUSTOM);
  joinStates(ref_ptr<State>::cast(texState));

  numBoneWeights_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("numBoneWeights"));
  numBoneWeights_->setUniformData(numBoneWeights);
  joinShaderInput(ref_ptr<ShaderInput>::cast(numBoneWeights_));

  boneMatrixData_ = new Mat4f[bones.size()];
  // prepend '#define HAS_BONES' to loaded shaders
  shaderDefine("HAS_BONES", "TRUE");

  // initially calculate the bone matrices
  glAnimate(RenderState::get(), 0.0f);
}
Bones::~Bones()
{
  delete []boneMatrixData_;
}

GLint Bones::numBoneWeights() const
{
  return numBoneWeights_->getVertex1i(0);
}

void Bones::glAnimate(RenderState *rs, GLdouble dt)
{
  register GLuint i=0;
  for(list< ref_ptr<AnimationNode> >::iterator
      it=bones_.begin(); it!=bones_.end(); ++it)
  {
    // the bone matrix is actually calculated in the animation thread
    // by NodeAnimation.
    boneMatrixData_[i] = (*it)->boneTransformationMatrix();
    i += 1;
  }
  rs->textureBuffer().push(boneMatrixVBO_->id());
  boneMatrixVBO_->set_bufferData(GL_TEXTURE_BUFFER,
      boneMatrixVBO_->bufferSize(), &boneMatrixData_[0].x);
  rs->textureBuffer().pop();
}
