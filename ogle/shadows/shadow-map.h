/*
 * shadow-map.h
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#ifndef SHADOW_MAP_H_
#define SHADOW_MAP_H_

#include <ogle/animations/animation.h>
#include <ogle/states/camera.h>
#include <ogle/states/light-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/render-tree/state-node.h>

class ShadowRenderState : public RenderState
{
public:
  ShadowRenderState(ref_ptr<Texture> texture);

  virtual void enable();

  virtual void set_shadowViewProjectionMatrices(Mat4f *mat) {};

  virtual void pushTexture(TextureState *tex);

  // no transform feedback
  virtual void set_useTransformFeedback(GLboolean) {}
  // shadow map fbo is used
  virtual void pushFBO(FrameBufferObject *tex) {}
  virtual void popFBO() {}

protected:
  GLuint fbo_;
  ref_ptr<Texture> texture_;
};

class LayeredShadowRenderState : public ShadowRenderState
{
public:
  LayeredShadowRenderState(
      ref_ptr<Texture> texture,
      GLuint maxNumBones,
      GLuint numShadowLayer);

  virtual void enable();

  virtual void set_shadowViewProjectionMatrices(Mat4f *mat);

  virtual void set_modelMat(Mat4f *mat);
  virtual void set_boneMatrices(Mat4f *mat, GLuint numWeights, GLuint numBones);
  virtual void set_viewMatrix(Mat4f *mat);
  virtual void set_ignoreViewRotation(GLboolean v);
  virtual void set_ignoreViewTranslation(GLboolean v);
  virtual void set_useTesselation(GLboolean v);
  virtual void set_projectionMatrix(Mat4f *mat);

  virtual void pushShaderInput(ShaderInput *att);
  virtual void popShaderInput(const string&) {};
  // update shader overwrites object shaders
  virtual void pushShader(Shader *tex) {}
  virtual void popShader() {}

protected:
  ref_ptr<ShaderState> updateShader_;

  GLuint numShadowLayer_;
  GLuint maxNumBones_;
  GLint modelMatLoc_;
  GLint numBoneWeightsLoc_;
  GLint boneMatricesLoc_;
  GLint viewMatrixLoc_;
  GLint ignoreViewRotationLoc_;
  GLint ignoreViewTranslationLoc_;
  GLint shadowVPMatricesLoc_;
  GLint projectionMatrixLoc_;
  GLint useTesselationLoc_;
  GLint posLocation_;
  GLint boneWeightsLocation_;
  GLint boneIndicesLocation_;
};

class ShadowMap : public Animation, public State
{
public:
  enum FilterMode {
    // just take a single texel
    SINGLE,
    // Bilinear weighted 4-tap filter
    PCF_4TAB,
    PCF_8TAB_RAND,
    // Gaussian 3x3 filter
    PCF_GAUSSIAN
  };

  static Mat4f biasMatrix_;

  ShadowMap(ref_ptr<Light> light, ref_ptr<Texture> texture);

  void set_filteringMode(FilterMode mode);

  void set_shadowMapSize(GLuint shadowMapSize);
  void set_internalFormat(GLenum internalFormat);
  void set_pixelType(GLenum pixelType);

  /**
   * Offset the geometry slightly to prevent z-fighting
   * Note that this introduces some light-leakage artifacts.
   */
  void setPolygonOffset(GLfloat factor=1.1, GLfloat units=4096.0);
  /**
   * Moves shadow acne to back faces. But it results in light bleeding
   * artifacts for some models.
   */
  void setCullFrontFaces(GLboolean v=GL_TRUE);

  /**
   * Adds a shadow caster tree to this shadow map.
   * You can just add the render tree of the perspective pass here.
   * But this tree might contain unneeded states (for example color maps).
   * A more optimized application may want to handle a special tree
   * for the shadow map traversal containing only relevant geometry
   * and states.
   */
  void addCaster(ref_ptr<StateNode> &caster);
  void removeCaster(StateNode *caster);

  /**
   * Traverse all added shadow caster.
   */
  void traverse(RenderState *rs);

  ref_ptr<TextureState>& shadowMap();

  // override
  virtual void animate(GLdouble dt);

  void drawDebugHUD(
      GLenum textureTarget,
      GLenum textureCompareMode,
      GLuint numTextures,
      GLuint textureID,
      const string &fragmentShader);

protected:
  ref_ptr<Light> light_;

  ref_ptr<Texture> texture_;
  ref_ptr<TextureState> shadowMap_;
  ref_ptr<ShaderInput1f> shadowMapSize_;

  list< ref_ptr<StateNode> > caster_;
  ref_ptr<State> cullState_;
  ref_ptr<State> polygonOffsetState_;

  ref_ptr<ShaderState> debugShader_;
  GLint debugLayerLoc_;
  GLint debugTextureLoc_;
};

#endif /* SHADOW_MAP_H_ */
