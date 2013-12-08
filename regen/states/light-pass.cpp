/*
 * light-pass.cpp
 *
 *  Created on: 13.03.2013
 *      Author: daniel
 */

#include <regen/states/state-configurer.h>
#include <regen/meshes/cone.h>
#include <regen/meshes/box.h>
#include <regen/states/shadow-map.h>

#include "light-pass.h"
using namespace regen;

LightPass::LightPass(Light::Type type, const string &shaderKey)
: State(), lightType_(type), shaderKey_(shaderKey)
{
  switch(lightType_) {
  case Light::DIRECTIONAL:
    mesh_ = Rectangle::getUnitQuad();
    break;
  case Light::SPOT:
    mesh_ = ConeClosed::getBaseCone();
    joinStates(ref_ptr<CullFaceState>::alloc(GL_FRONT));
    break;
  case Light::POINT:
    mesh_ = Box::getUnitCube();
    joinStates(ref_ptr<CullFaceState>::alloc(GL_FRONT));
    break;
  }
  shadowFiltering_ = ShadowMap::FILTERING_NONE;
  numShadowLayer_ = 1;

  shader_ = ref_ptr<ShaderState>::alloc();
  joinStates(shader_);
}

void LightPass::setShadowFiltering(ShadowMap::FilterMode mode)
{
  GLboolean usedMoments = ShadowMap::useShadowMoments(shadowFiltering_);

  shadowFiltering_ = mode;
  shaderDefine("USE_SHADOW_MAP", "TRUE");
  shaderDefine("USE_SHADOW_SAMPLER", ShadowMap::useShadowSampler(mode) ? "TRUE" : "FALSE");
  shaderDefine("SHADOW_MAP_FILTER", ShadowMap::shadowFilterMode(mode));

  for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  {
    if(!it->sm.get()) continue;
    if(usedMoments != ShadowMap::useShadowMoments(shadowFiltering_))
      it->sm->setComputeMoments();
    it->sm->updateSamplerType(shadowFiltering_, it->light->lightType());
  }
}

void LightPass::setShadowLayer(GLuint numLayer)
{
  numShadowLayer_ = numLayer;
  // change number of layers for added lights
  for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  {
    if(!it->sm.get()) continue;
    it->sm->set_shadowLayer(numLayer);
  }
}

void LightPass::addLight(
    const ref_ptr<Light> &l,
    const ref_ptr<ShadowMap> &sm,
    const list< ref_ptr<ShaderInput> > &inputs)
{
  LightPassLight light;
  lights_.push_back(light);

  list<LightPassLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;

  it->light = l;
  it->sm = sm;
  it->inputs = inputs;
  if(sm.get()!=NULL) sm->updateSamplerType(shadowFiltering_, l->lightType());

  if(sm.get() && ShadowMap::useShadowMoments(shadowFiltering_))
  { sm->setComputeMoments(); }
  if(shader_->shader().get())
  { addLightInput(*it); }
}
void LightPass::removeLight(Light *l)
{
  lights_.erase( lightIterators_[l] );
}
GLboolean LightPass::empty() const
{
  return lights_.empty();
}
GLboolean LightPass::hasLight(Light *l) const
{
  return lightIterators_.count(l)>0;
}

void LightPass::createShader(const StateConfig &cfg)
{
  StateConfigurer _cfg(cfg);
  _cfg.addState(this);
  _cfg.addState(mesh_.get());
  _cfg.define("NUM_SHADOW_LAYER", REGEN_STRING(numShadowLayer_));
  shader_->createShader(_cfg.cfg(), shaderKey_);
  mesh_->updateVAO(RenderState::get(), _cfg.cfg(), shader_->shader());

  for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  { addLightInput(*it); }
  shadowMapLoc_ = shader_->shader()->uniformLocation("shadowTexture");
}

void LightPass::addLightInput(LightPassLight &light)
{
  // clear list of uniform loactions
  light.inputLocations.clear();
  // add user specified uniforms
  for(list< ref_ptr<ShaderInput> >::iterator jt=light.inputs.begin(); jt!=light.inputs.end(); ++jt)
  {
    ref_ptr<ShaderInput> &in = *jt;
    addInputLocation(light, in, in->name());
  }

  // add shadow uniforms
  if(light.sm.get()) {
    addInputLocation(light,light.sm->shadowInverseSize(), "shadowInverseSize");
    addInputLocation(light,light.sm->shadowFar(), "shadowFar");
    addInputLocation(light,light.sm->shadowNear(), "shadowNear");
    addInputLocation(light,light.sm->shadowMat(), "shadowMatrix");
  }

  // add light uniforms
  switch(lightType_) {
  case Light::DIRECTIONAL: {
    addInputLocation(light,light.light->direction(), "lightDirection");
    addInputLocation(light,light.light->diffuse(), "lightDiffuse");
    addInputLocation(light,light.light->specular(), "lightSpecular");
  }
  break;
  case Light::SPOT: {
    addInputLocation(light,light.light->position(), "lightPosition");
    addInputLocation(light,light.light->direction(), "lightDirection");
    addInputLocation(light,light.light->radius(), "lightRadius");
    addInputLocation(light,light.light->coneAngle(), "lightConeAngles");
    addInputLocation(light,light.light->diffuse(), "lightDiffuse");
    addInputLocation(light,light.light->specular(), "lightSpecular");
    addInputLocation(light,light.light->coneMatrix(), "modelMatrix");
  }
  break;
  case Light::POINT: {
    addInputLocation(light,light.light->position(), "lightPosition");
    addInputLocation(light,light.light->radius(), "lightRadius");
    addInputLocation(light,light.light->diffuse(), "lightDiffuse");
    addInputLocation(light,light.light->specular(), "lightSpecular");
  }
  break;
  }
}

void LightPass::addInputLocation(LightPassLight &l,
    const ref_ptr<ShaderInput> &in, const string &name)
{
  Shader *s = shader_->shader().get();
  GLint loc = s->uniformLocation(name);
  if(loc>0) {
    l.inputLocations.push_back( ShaderInputLocation(in, loc) );
  }
}

void LightPass::enable(RenderState *rs)
{
  State::enable(rs);
  GLuint smChannel = rs->reserveTextureChannel();

  for(list<LightPassLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  {
    LightPassLight &l = *it;
    ref_ptr<Texture> shadowTex;

    // activate shadow map if specified
    if(l.sm.get()) {
      switch(shadowFiltering_) {
      case ShadowMap::FILTERING_VSM:
        shadowTex = l.sm->shadowMoments();
        break;
      default:
        shadowTex = l.sm->shadowDepth();
        break;
      }
      shadowTex->begin(rs,smChannel);
      glUniform1i(shadowMapLoc_, smChannel);
    }
    // enable light pass uniforms
    for(list<ShaderInputLocation>::iterator
        jt=l.inputLocations.begin(); jt!=l.inputLocations.end(); ++jt)
    {
      if(jt->uploadStamp != jt->input->stamp()) {
        jt->input->enableUniform(jt->location);
        jt->uploadStamp = jt->input->stamp();
      }
    }

    mesh_->enable(rs);
    mesh_->disable(rs);

    if(shadowTex.get())
    { shadowTex->end(rs,smChannel); }
  }

  rs->releaseTextureChannel();
}
