/*
 * directional-shadow-map.cpp
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#include <cfloat>

#include <ogle/utility/string-util.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/utility/gl-error.h>

#include "directional-shadow-map.h"

//#define DEBUG_SHADOW_MAPS

static inline Vec2f findZRange(
    const Mat4f &mat, const Vec3f *frustumPoints)
{
  Vec2f range;
  // find the z-range of the current frustum as seen from the light
  // in order to increase precision
#define TRANSFORM_Z(vec) mat.x[2]*vec.x + mat.x[6]*vec.y + mat.x[10]*vec.z + mat.x[14]
  // note that only the z-component is needed and thus
  // the multiplication can be simplified from mat*vec4f(frustumPoints[0], 1.0f) to..
  GLfloat buf = TRANSFORM_Z(frustumPoints[0]);
  range.x = buf;
  range.y = buf;
  for(GLint i=1; i<8; ++i)
  {
    buf = TRANSFORM_Z(frustumPoints[i]);
    if(buf > range.y) { range.y = buf; }
    if(buf < range.x) { range.x = buf; }
  }
#undef TRANSFORM_Z
  return range;
}

//////////

GLuint DirectionalShadowMap::numSplits_ = 3;

void DirectionalShadowMap::set_numSplits(GLuint numSplits)
{
  numSplits_ = numSplits;
}
GLuint DirectionalShadowMap::numSplits()
{
  return numSplits_;
}

/////////////

static ref_ptr<Texture> dirShadowDepth(GLenum internalFormat, GLenum pixelType)
{
  ref_ptr<Texture> tex = ref_ptr<Texture>::manage(new DepthTexture3D(1));
  tex->set_internalFormat(internalFormat);
  tex->set_pixelType(pixelType);
  tex->set_targetType(GL_TEXTURE_2D_ARRAY);
  return tex;
}

DirectionalShadowMap::DirectionalShadowMap(
    const ref_ptr<DirectionalLight> &light,
    const ref_ptr<Frustum> &sceneFrustum,
    const ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint shadowMapSize,
    GLdouble splitWeight,
    GLenum internalFormat,
    GLenum pixelType)
: ShadowMap(ref_ptr<Light>::cast(light), dirShadowDepth(internalFormat, pixelType)),
  sceneFrustum_(sceneFrustum),
  splitWeight_(splitWeight),
  dirLight_(light),
  sceneCamera_(sceneCamera)
{
  shadowMap_->set_samplerType("sampler2DArrayShadow");
  // texture array with a layer for each slice
  ((DepthTexture3D*)texture_.get())->set_depth(numSplits_);
  texture_->set_size(shadowMapSize, shadowMapSize);
  // on nvidia linear filtering gives 2x2 PCF for 'free'
  texture_->set_filter(GL_LINEAR,GL_LINEAR);
  texture_->texImage();
  shadowMapSize_->setUniformData((float)shadowMapSize);

  projectionMatrices_ = new Mat4f[numSplits_];
  viewProjectionMatrices_ = new Mat4f[numSplits_];

  // uniforms for shadow sampling
  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      FORMAT_STRING("shadowMatrices"), numSplits_));
  shadowMatUniform_->set_forceArray(GL_TRUE);
  shadowMatUniform_->setUniformDataUntyped(NULL);

  shadowFarUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowFar"), numSplits_));
  shadowFarUniform_->set_forceArray(GL_TRUE);
  shadowFarUniform_->setUniformDataUntyped(NULL);

  // custom render state hopefully saves some cpu time
  rs_ = new ShadowRenderState(ref_ptr<Texture>::cast(texture_));

  updateLightDirection();
  updateProjection();
}
DirectionalShadowMap::~DirectionalShadowMap()
{
  for(vector<Frustum*>::iterator
      it=shadowFrusta_.begin(); it!=shadowFrusta_.end(); ++it) { delete *it; }
  shadowFrusta_.clear();
  delete[] projectionMatrices_;
  delete[] viewProjectionMatrices_;
  delete rs_;
}

void DirectionalShadowMap::set_splitWeight(GLdouble splitWeight)
{
  splitWeight_ = splitWeight;
}
GLdouble DirectionalShadowMap::splitWeight() const
{
  return splitWeight_;
}

const ref_ptr<ShaderInputMat4>& DirectionalShadowMap::shadowMatUniform() const
{
  return shadowMatUniform_;
}
const ref_ptr<ShaderInput1f>& DirectionalShadowMap::shadowFarUniform() const
{
  return shadowFarUniform_;
}

void DirectionalShadowMap::updateLightDirection()
{
  const Vec3f &dir = dirLight_->direction()->getVertex3f(0);
  Vec3f f(-dir.x, -dir.y, -dir.z);
  f.normalize();
  Vec3f s( 0.0f, -f.z, f.y );
  s.normalize();
  // Equivalent to getLookAtMatrix(pos=(0,0,0), dir=-dir, up=(-1,0,0))
  viewMatrix_ = Mat4f(
      0.0f, s.y*f.z - s.z*f.y, -f.x, 0.0f,
       s.y,           s.z*f.x, -f.y, 0.0f,
       s.z,          -s.y*f.x, -f.z, 0.0f,
      0.0f,              0.0f, 0.0f, 1.0f
  );
  lightDirectionStamp_ = dirLight_->direction()->stamp();
}

void DirectionalShadowMap::updateProjection()
{
  for(vector<Frustum*>::iterator
      it=shadowFrusta_.begin(); it!=shadowFrusta_.end(); ++it) { delete *it; }
  shadowFrusta_ = sceneFrustum_->split(numSplits_, splitWeight_);

  const Mat4f &proj = sceneCamera_->projection();
  GLfloat *farValues = (GLfloat*)shadowFarUniform_->dataPtr();

  for(GLuint i=0; i<numSplits_; ++i)
  {
    Frustum *frustum = shadowFrusta_[i];
    // frustum_->far() is originally in eye space - tell's us how far we can see.
    // Here we compute it in camera homogeneous coordinates. Basically, we calculate
    // proj * (0, 0, far, 1)^t and then normalize to [0; 1]
    farValues[i] = 0.5*(-frustum->far() * proj(2,2) + proj(3,2)) / frustum->far() + 0.5;
  }
}

void DirectionalShadowMap::updateCamera()
{
  Mat4f *shadowMatrices = (Mat4f*)shadowMatUniform_->dataPtr();

  for(register GLuint i=0; i<numSplits_; ++i)
  {
    Frustum *frustum = shadowFrusta_[i];
    // update frustum points in world space
    frustum->calculatePoints(
        sceneCamera_->position(), sceneCamera_->direction(), UP_VECTOR);
    const Vec3f *frustumPoints = frustum->points();

    // get the projection matrix with the new z-bounds
    // note the inversion because the light looks at the neg. z axis
    Vec2f zRange = findZRange(viewMatrix_, frustumPoints);
    projectionMatrices_[i] = Mat4f::orthogonalMatrix(
        -1.0, 1.0, -1.0, 1.0, -zRange.y, -zRange.x);

    // find the extends of the frustum slice as projected in light's homogeneous coordinates
    Vec2f xRange(FLT_MAX,FLT_MIN);
    Vec2f yRange(FLT_MAX,FLT_MIN);
    Mat4f mvpMatrix = (viewMatrix_ * projectionMatrices_[i]).transpose();
    for(register GLuint j=0; j<8; ++j)
    {
        Vec4f transf = mvpMatrix * frustumPoints[j];
        transf.x /= transf.w;
        transf.y /= transf.w;
        if (transf.x > xRange.y) { xRange.y = transf.x; }
        if (transf.x < xRange.x) { xRange.x = transf.x; }
        if (transf.y > yRange.y) { yRange.y = transf.y; }
        if (transf.y < yRange.x) { yRange.x = transf.y; }
    }
    projectionMatrices_[i] = projectionMatrices_[i] *
        Mat4f::cropMatrix(xRange.x, xRange.y, yRange.x, yRange.y);

    viewProjectionMatrices_[i] = viewMatrix_ * projectionMatrices_[i];
    // transforms world space coordinates to homogenous light space
    shadowMatrices[i] = viewProjectionMatrices_[i] * biasMatrix_;
  }
}

void DirectionalShadowMap::glAnimate(GLdouble dt)
{
  if(lightDirectionStamp_ != dirLight_->direction()->stamp()) {
    updateLightDirection();
  }
  // update shadow view and projection matrices
  updateCamera();

  enable(rs_);
  rs_->enable();

  Mat4f &view = sceneCamera_->viewUniform()->getVertex16f(0);
  Mat4f &proj = sceneCamera_->projectionUniform()->getVertex16f(0);
  Mat4f &viewproj = sceneCamera_->viewProjectionUniform()->getVertex16f(0);
  Mat4f sceneView = view;
  Mat4f sceneProj = proj;
  Mat4f sceneViewProj = viewproj;
  view = viewMatrix_;

  for(register GLuint i=0; i<numSplits_; ++i)
  {
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_->id(), 0, i);
    proj = projectionMatrices_[i];
    viewproj = viewProjectionMatrices_[i];
    traverse(rs_);
  }

  view = sceneView;
  proj = sceneProj;
  viewproj = sceneViewProj;

  disable(rs_);

#ifdef DEBUG_SHADOW_MAPS
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  drawDebugHUD(
      GL_TEXTURE_2D_ARRAY,
      GL_COMPARE_R_TO_TEXTURE,
      numSplits_,
      texture_->id(),
      "shadow_mapping.debugDirectional.fs");
#endif
}
