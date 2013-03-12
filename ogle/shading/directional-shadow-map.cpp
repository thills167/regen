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
#include <ogle/utility/gl-util.h>

#include "directional-shadow-map.h"
using namespace ogle;

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

/////////////

DirectionalShadowMap::DirectionalShadowMap(
    const ref_ptr<DirectionalLight> &light,
    const ref_ptr<Frustum> &sceneFrustum,
    const ref_ptr<Camera> &sceneCamera,
    GLuint numShadowLayer,
    GLuint shadowMapSize,
    GLdouble splitWeight,
    GLenum depthFormat,
    GLenum depthType)
: ShadowMap(ref_ptr<Light>::cast(light), GL_TEXTURE_2D_ARRAY,
    shadowMapSize, numShadowLayer, depthFormat, depthType),
  numShadowLayer_(numShadowLayer),
  sceneFrustum_(sceneFrustum),
  splitWeight_(splitWeight),
  dirLight_(light),
  sceneCamera_(sceneCamera)
{
  projectionMatrices_ = new Mat4f[numShadowLayer_];
  viewProjectionMatrices_ = new Mat4f[numShadowLayer_];

  // uniforms for shadow sampling
  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      FORMAT_STRING("shadowMatrices"), numShadowLayer_));
  shadowMatUniform_->set_forceArray(GL_TRUE);
  shadowMatUniform_->setUniformDataUntyped(NULL);
  setInput(ref_ptr<ShaderInput>::cast(shadowMatUniform_));

  shadowFarUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(
      FORMAT_STRING("shadowFar"), numShadowLayer_));
  shadowFarUniform_->set_forceArray(GL_TRUE);
  shadowFarUniform_->setUniformDataUntyped(NULL);
  setInput(ref_ptr<ShaderInput>::cast(shadowFarUniform_));

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
}

GLuint DirectionalShadowMap::numShadowLayer() const
{
  return numShadowLayer_;
}
void DirectionalShadowMap::set_numShadowLayer(GLuint numLayer)
{
  if(numShadowLayer_ == numLayer) { return; }

  numShadowLayer_ = numLayer;

  ((DepthTexture3D*)depthTexture_.get())->set_depth(numShadowLayer_);

  shadowMatUniform_->set_elementCount(numLayer);
  shadowMatUniform_->setUniformDataUntyped(NULL);
  shadowFarUniform_->set_elementCount(numLayer);
  shadowFarUniform_->setUniformDataUntyped(NULL);

  delete []projectionMatrices_;
  delete []viewProjectionMatrices_;
  projectionMatrices_ = new Mat4f[numShadowLayer_];
  viewProjectionMatrices_ = new Mat4f[numShadowLayer_];
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
  // Equivalent to getLookAtMatrix(pos=(0,0,0), dir=f, up=(-1,0,0))
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
  shadowFrusta_ = sceneFrustum_->split(numShadowLayer_, splitWeight_);

  const Mat4f &proj = sceneCamera_->projection()->getVertex16f(0);
  GLfloat *farValues = (GLfloat*)shadowFarUniform_->dataPtr();

  for(GLuint i=0; i<numShadowLayer_; ++i)
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

  for(register GLuint i=0; i<numShadowLayer_; ++i)
  {
    Frustum *frustum = shadowFrusta_[i];
    // update frustum points in world space
    frustum->computePoints(
        sceneCamera_->position()->getVertex3f(0),
        sceneCamera_->direction()->getVertex3f(0));
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
    shadowMatrices[i] = viewProjectionMatrices_[i] * Mat4f::bias();
  }
}
void DirectionalShadowMap::update()
{
  if(lightDirectionStamp_ != dirLight_->direction()->stamp()) {
    updateLightDirection();
  }
  // update shadow view and projection matrices
  updateCamera();
}

void DirectionalShadowMap::computeDepth(RenderState *rs)
{
  sceneCamera_->position()->pushData((byte*)&Vec3f::zero().x);
  sceneCamera_->view()->pushData((byte*)viewMatrix_.x);
  for(register GLuint i=0; i<numShadowLayer_; ++i)
  {
    glFramebufferTextureLayer(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT, depthTexture_->id(), 0, i);
    glClear(GL_DEPTH_BUFFER_BIT);

    sceneCamera_->projection()->pushData((byte*)projectionMatrices_[i].x);
    sceneCamera_->viewProjection()->pushData((byte*)viewProjectionMatrices_[i].x);

    traverse(rs);

    sceneCamera_->viewProjection()->popData();
    sceneCamera_->projection()->popData();
  }
  sceneCamera_->view()->popData();
  sceneCamera_->position()->popData();
}

void DirectionalShadowMap::computeMoment(RenderState *rs)
{
  momentsCompute_->enable(rs);
  textureQuad_->draw(1);
  momentsCompute_->disable(rs);
}
