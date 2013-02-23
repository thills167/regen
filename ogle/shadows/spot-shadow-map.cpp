/*
 * spot-shadow-map.cpp
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>

#include "spot-shadow-map.h"

SpotShadowMap::SpotShadowMap(
    const ref_ptr<SpotLight> &light,
    const ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint shadowMapSize,
    GLenum depthFormat,
    GLenum depthType)
: ShadowMap(ref_ptr<Light>::cast(light), GL_TEXTURE_2D,
    shadowMapSize, 1, depthFormat, depthType),
  spotLight_(light),
  sceneCamera_(sceneCamera)
{
  // uniforms for shadow sampling
  shadowFarUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowFar"<<light->id())));
  shadowFarUniform_->setUniformData(200.0f);

  shadowNearUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowNear"<<light->id())));
  shadowNearUniform_->setUniformData(0.1f);

  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      FORMAT_STRING("shadowMatrix"<<light->id())));
  shadowMatUniform_->setUniformDataUntyped(NULL);

  updateLight();
}

const ref_ptr<ShaderInput1f>& SpotShadowMap::near() const
{
  return shadowNearUniform_;
}
const ref_ptr<ShaderInput1f>& SpotShadowMap::far() const
{
  return shadowFarUniform_;
}
const ref_ptr<ShaderInputMat4>& SpotShadowMap::shadowMatUniform() const
{
  return shadowMatUniform_;
}

void SpotShadowMap::updateLight()
{
  const Vec3f &pos = spotLight_->position()->getVertex3f(0);
  const Vec3f &dir = spotLight_->spotDirection()->getVertex3f(0);
  const Vec2f &a = light_->radius()->getVertex2f(0);
  shadowFarUniform_->setVertex1f(0, a.y);

  viewMatrix_ = Mat4f::lookAtMatrix(pos, dir, UP_VECTOR);

  const Vec2f &coneAngle = spotLight_->coneAngle()->getVertex2f(0);
  projectionMatrix_ = Mat4f::projectionMatrix(
      2.0*360.0*acos(coneAngle.y)/(2.0*M_PI), 1.0f,
      shadowNearUniform_->getVertex1f(0),
      shadowFarUniform_->getVertex1f(0));
  viewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
  // transforms world space coordinates to homogenous light space
  shadowMatUniform_->getVertex16f(0) = viewProjectionMatrix_ * biasMatrix_;

  lightPosStamp_ = spotLight_->position()->stamp();
  lightDirStamp_ = spotLight_->spotDirection()->stamp();
  lightRadiusStamp_ = spotLight_->radius()->stamp();
}
void SpotShadowMap::update()
{
  if(lightPosStamp_ != spotLight_->position()->stamp() ||
      lightDirStamp_ != spotLight_->spotDirection()->stamp() ||
      lightRadiusStamp_ != spotLight_->radius()->stamp())
  {
    updateLight();
  }
}

void SpotShadowMap::computeDepth()
{
  Mat4f &view = sceneCamera_->viewUniform()->getVertex16f(0);
  Mat4f &proj = sceneCamera_->projectionUniform()->getVertex16f(0);
  Mat4f &viewproj = sceneCamera_->viewProjectionUniform()->getVertex16f(0);
  Mat4f sceneView = view;
  Mat4f sceneProj = proj;
  Mat4f sceneViewProj = viewproj;
  view = viewMatrix_;
  proj = projectionMatrix_;
  viewproj = viewProjectionMatrix_;

  traverse(&depthRenderState_);

  view = sceneView;
  proj = sceneProj;
  viewproj = sceneViewProj;
}

void SpotShadowMap::computeMoment()
{
  momentsCompute_->enable(&filteringRenderState_);
  shadowNearUniform_->enableUniform(momentsNear_);
  shadowFarUniform_->enableUniform(momentsFar_);
  textureQuad_->draw(1);
  momentsCompute_->disable(&filteringRenderState_);
}
