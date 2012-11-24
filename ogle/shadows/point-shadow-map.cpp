/*
 * point-shadow-map.cpp
 *
 *  Created on: 20.11.2012
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>

#include "point-shadow-map.h"

// #define DEBUG_SHADOW_MAPS

PointShadowMap::PointShadowMap(
    ref_ptr<PointLight> &light,
    ref_ptr<PerspectiveCamera> &sceneCamera,
    GLuint shadowMapSize)
: ShadowMap(),
  light_(light),
  sceneCamera_(sceneCamera),
  compareMode_(GL_COMPARE_R_TO_TEXTURE)
{
  // create a 3d depth texture - each frustum slice gets one layer
  texture_ = ref_ptr<CubeMapDepthTexture>::manage(new CubeMapDepthTexture);
  texture_->set_internalFormat(GL_DEPTH_COMPONENT24);
  texture_->set_pixelType(GL_FLOAT);
  texture_->bind();
  texture_->set_size(shadowMapSize, shadowMapSize);
  texture_->set_filter(GL_LINEAR,GL_LINEAR);
  texture_->set_wrapping(GL_CLAMP_TO_EDGE);
  texture_->set_compare(compareMode_, GL_LEQUAL);
  texture_->texImage();
  // create depth only render target for updating the shadow maps
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glDrawBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  viewMatrices_ = new Mat4f[6];
  // uniforms for shadow sampling
  shadowMatUniform_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4(
      FORMAT_STRING("shadowMatrices"<<light->id()), 6));
  shadowMatUniform_->setInstanceData(1, 1, NULL);

  shadowMap_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(texture_)));
  shadowMap_->set_name(FORMAT_STRING("shadowMap"<<light->id()));
  shadowMap_->set_mapping(MAPPING_CUSTOM);
  shadowMap_->setMapTo(MAP_TO_CUSTOM);

  updateLight();

  light_->joinShaderInput(
      ref_ptr<ShaderInput>::cast(shadowMatUniform()));
  light_->joinStates(
      ref_ptr<State>::cast(shadowMap()));
  light_->shaderDefine(
      FORMAT_STRING("LIGHT"<<light_->id()<<"_HAS_SM"), "TRUE");
}

ref_ptr<ShaderInputMat4>& PointShadowMap::shadowMatUniform()
{
  return shadowMatUniform_;
}
ref_ptr<TextureState>& PointShadowMap::shadowMap()
{
  return shadowMap_;
}

void PointShadowMap::updateLight()
{
  static Mat4f staticBiasMatrix = Mat4f(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0 );
  static const Vec3f dir[6] = {
      Vec3f( 1.0f, 0.0f, 0.0f),
      Vec3f(-1.0f, 0.0f, 0.0f),
      Vec3f( 0.0f, 1.0f, 0.0f),
      Vec3f( 0.0f,-1.0f, 0.0f),
      Vec3f( 0.0f, 0.0f, 1.0f),
      Vec3f( 0.0f, 0.0f,-1.0f)
  };
  // have to change up vector for top and bottom face
  // for getLookAtMatrix
  static const Vec3f up[6] = {
      Vec3f( 0.0f, -1.0f, 0.0f),
      Vec3f( 0.0f, -1.0f, 0.0f),
      Vec3f( 0.0f, 0.0f, -1.0f),
      Vec3f( 0.0f, 0.0f, -1.0f),
      Vec3f( 0.0f, -1.0f, 0.0f),
      Vec3f( 0.0f, -1.0f, 0.0f)
  };
  // everything below this attenuation does not appear in the shadow map
  static const GLfloat farAttenuation = 0.01f;

  Mat4f *shadowMatrices = (Mat4f*)shadowMatUniform_->dataPtr();
  const Vec3f &pos = light_->position()->getVertex3f(0);
  const Vec3f &a = light_->attenuation()->getVertex3f(0);

  // adjust far value for better precision
  GLfloat far;
  // find far value where light attenuation reaches threshold,
  // insert farAttenuation in the attenuation equation and solve
  // equation for distance
  GLdouble p2 = a.y/(2.0*a.z);
  far = -p2 + sqrt(p2*p2 - (a.x/farAttenuation - 1.0/(farAttenuation*a.z)));
  // hard limit to 200.0 z range
  if(far>200.0) far=200.0;

  projectionMatrix_ = projectionMatrix(
      90.0, // fov
      1.0f, // shadow map aspect
      0.1f, // near
      far);

  for(register GLuint i=0; i<6; ++i) {
    viewMatrices_[i] = getLookAtMatrix(pos, dir[i], up[i]);
    // transforms world space coordinates to homogenous light space
    shadowMatrices[i] =
        viewMatrices_[i] * projectionMatrix_ * staticBiasMatrix;
  }
}

void PointShadowMap::updateShadow()
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glDrawBuffer(GL_NONE);
  glViewport(0, 0, texture_->width(), texture_->height());

  // remember scene view and projection
  Mat4f sceneView = sceneCamera_->viewUniform()->getVertex16f(0);
  Mat4f sceneProjection = sceneCamera_->projectionUniform()->getVertex16f(0);
  sceneCamera_->projectionUniform()->setVertex16f(0, projectionMatrix_);

  for(register GLuint i=0; i<6; ++i) {
    // make the current depth map a rendering target
    glFramebufferTexture2D(GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,
        texture_->id(), 0);
    // clear the depth texture from last time
    glClear(GL_DEPTH_BUFFER_BIT);
    sceneCamera_->viewUniform()->setVertex16f(0, viewMatrices_[i]);
    // tree traverse
    traverse();
  }

  sceneCamera_->viewUniform()->setVertex16f(0, sceneView);
  sceneCamera_->projectionUniform()->setVertex16f(0, sceneProjection);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifdef DEBUG_SHADOW_MAPS
  drawDebugHUD();
#endif
}

void PointShadowMap::drawDebugHUD()
{
    static GLint layerLoc;
    static GLint textureLoc;
    static ref_ptr<ShaderState> debugShader;
    if(debugShader.get() == NULL) {
      debugShader = ref_ptr<ShaderState>::manage(new ShaderState);
      map<string, string> shaderConfig;
      map<GLenum, string> shaderNames;
      shaderNames[GL_FRAGMENT_SHADER] = "shadow-mapping.debugPoint.fs";
      shaderNames[GL_VERTEX_SHADER] = "shadow-mapping.debug.vs";
      debugShader->createSimple(shaderConfig,shaderNames);
      debugShader->shader()->compile();
      debugShader->shader()->link();

      layerLoc = glGetUniformLocation(debugShader->shader()->id(), "in_shadowLayer");
      textureLoc = glGetUniformLocation(debugShader->shader()->id(), "in_shadowMap");
    }

    glDisable(GL_DEPTH_TEST);

    glUseProgram(debugShader->shader()->id());
    glUniform1i(textureLoc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture( GL_TEXTURE_CUBE_MAP, texture_->id() );
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    for(GLuint i=0; i<6; ++i) {
      glViewport(130*i, 0, 128, 128);
      glUniform1f(layerLoc, float(i));

      glBegin(GL_QUADS);
      glVertex3f(-1.0, -1.0, 0.0);
      glVertex3f( 1.0, -1.0, 0.0);
      glVertex3f( 1.0,  1.0, 0.0);
      glVertex3f(-1.0,  1.0, 0.0);
      glEnd();
    }

    // reset states
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, compareMode_);
    glEnable(GL_DEPTH_TEST);
}
