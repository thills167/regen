/*
 * directional-shadow-map.h
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#ifndef DIRECTIONAL_SHADOW_MAP_H_
#define DIRECTIONAL_SHADOW_MAP_H_

#include <ogle/algebra/frustum.h>
#include <ogle/animations/animation.h>
#include <ogle/states/light-state.h>
#include <ogle/states/camera.h>
#include <ogle/render-tree/state-node.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/shadows/shadow-map.h>

/**
 * Implements Parallel Split Shadow Mapping aka Cascade Shadow Mapping.
 * @see http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html
 */
class DirectionalShadowMap : public ShadowMap
{
public:
  /**
   * Sets the number of frustum splits used for all instances
   * of DirectionalShadowMap.
   * You should set this before instantiating.
   */
  static void set_numSplits(GLuint numSplits);
  static GLuint numSplits();

  DirectionalShadowMap(
      ref_ptr<DirectionalLight> &light,
      ref_ptr<Frustum> &sceneFrustum,
      ref_ptr<PerspectiveCamera> &sceneCamera,
      GLuint shadowMapSize,
      GLdouble splitWeight=0.75,
      GLenum internalFormat=GL_DEPTH_COMPONENT24,
      GLenum pixelType=GL_FLOAT);
  ~DirectionalShadowMap();

  void set_splitWeight(GLdouble splitWeight);
  GLuint splitWeight();

  /**
   * Should be called when the light direction changed.
   */
  void updateLightDirection();
  /**
   * Should be called when the scene projection matrix changed.
   */
  void updateProjection();

  ref_ptr<ShaderInputMat4>& shadowMatUniform();
  ref_ptr<ShaderInput1f>& shadowFarUniform();

  // override
  virtual void glAnimate(GLdouble dt);

protected:
  // number of frustum splits
  static GLuint numSplits_;

  // scene frustum and splits
  ref_ptr<Frustum> sceneFrustum_;
  vector<Frustum*> shadowFrusta_;
  GLdouble splitWeight_;

  // shadow casting light
  ref_ptr<DirectionalLight> dirLight_;
  // main camera
  ref_ptr<PerspectiveCamera> sceneCamera_;

  // shadow map update uniforms
  Mat4f viewMatrix_;
  Mat4f *projectionMatrices_;
  Mat4f *viewProjectionMatrices_;
  // sampling uniforms
  ref_ptr<ShaderInputMat4> shadowMatUniform_;
  ref_ptr<ShaderInput1f> shadowFarUniform_;

  ShadowRenderState *rs_;

  void updateCamera();
};

#endif /* DIRECTIONAL_SHADOW_MAP_H_ */
