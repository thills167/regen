/*
 * deferred.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include <ogle/states/shader-configurer.h>

#include "deferred.h"
using namespace ogle;

DeferredShading::DeferredShading()
: State(), hasAmbient_(GL_FALSE), hasShaderConfig_(GL_FALSE)
{
  // accumulate light using add blending
  joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));

  ambientState_ = ref_ptr<DeferredAmbientLight>::manage(new DeferredAmbientLight);

  dirState_ = ref_ptr<LightPass>::manage(
      new LightPass(LightPass::DIRECTIONAL, "shading.deferred.directional"));
  dirShadowState_ = ref_ptr<LightPass>::manage(
      new LightPass(LightPass::DIRECTIONAL, "shading.deferred.directional"));
  dirShadowState_->setShadowFiltering(ShadowMap::FILTERING_NONE);

  pointState_ = ref_ptr<LightPass>::manage(
      new LightPass(LightPass::POINT, "shading.deferred.point"));
  pointShadowState_ = ref_ptr<LightPass>::manage(
      new LightPass(LightPass::POINT, "shading.deferred.point"));
  pointShadowState_->setShadowFiltering(ShadowMap::FILTERING_NONE);

  spotState_ = ref_ptr<LightPass>::manage(
      new LightPass(LightPass::SPOT, "shading.deferred.spot"));
  spotShadowState_ = ref_ptr<LightPass>::manage(
      new LightPass(LightPass::SPOT, "shading.deferred.spot"));
  spotShadowState_->setShadowFiltering(ShadowMap::FILTERING_NONE);

  lightSequence_ = ref_ptr<StateSequence>::manage(new StateSequence);
  joinStates(ref_ptr<State>::cast(lightSequence_));
}

void DeferredShading::createShader(ShaderState::Config &cfg)
{
  {
    ShaderConfigurer _cfg(cfg);
    _cfg.addState(this);
    shaderCfg_ = _cfg.cfg();
    hasShaderConfig_ = GL_TRUE;
  }

  if(!dirState_->empty()) {
    dirState_->createShader(shaderCfg_);
  }
  if(!pointState_->empty()) {
    pointState_->createShader(shaderCfg_);
  }
  if(!spotState_->empty()) {
    spotState_->createShader(shaderCfg_);
  }
  if(!dirShadowState_->empty()) {
    dirShadowState_->createShader(shaderCfg_);
  }
  if(!pointShadowState_->empty()) {
    pointShadowState_->createShader(shaderCfg_);
  }
  if(!spotShadowState_->empty()) {
    spotShadowState_->createShader(shaderCfg_);
  }
  if(hasAmbient_) {
    ambientState_->createShader(shaderCfg_);
  }
}

void DeferredShading::set_gBuffer(
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<Texture> &norWorldTexture,
    const ref_ptr<Texture> &diffuseTexture,
    const ref_ptr<Texture> &specularTexture)
{
  if(gDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(gDepthTexture_));
    disjoinStates(ref_ptr<State>::cast(gDiffuseTexture_));
    disjoinStates(ref_ptr<State>::cast(gSpecularTexture_));
    disjoinStates(ref_ptr<State>::cast(gNorWorldTexture_));
  }

  gDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depthTexture, "gDepthTexture"));
  joinStatesFront(ref_ptr<State>::cast(gDepthTexture_));

  gNorWorldTexture_ = ref_ptr<TextureState>::manage(new TextureState(norWorldTexture, "gNorWorldTexture"));
  joinStatesFront(ref_ptr<State>::cast(gNorWorldTexture_));

  gDiffuseTexture_ = ref_ptr<TextureState>::manage(new TextureState(diffuseTexture, "gDiffuseTexture"));
  joinStatesFront(ref_ptr<State>::cast(gDiffuseTexture_));

  gSpecularTexture_ = ref_ptr<TextureState>::manage(new TextureState(specularTexture, "gSpecularTexture"));
  joinStatesFront(ref_ptr<State>::cast(gSpecularTexture_));
}

ref_ptr<LightPass> DeferredShading::getLightState(
    const ref_ptr<Light> &light, const ref_ptr<ShadowMap> &shadowMap)
{
  if(dynamic_cast<DirectionalLight*>(light.get())) {
    return (shadowMap.get() ? dirShadowState_ : dirState_);
  }
  else if(dynamic_cast<PointLight*>(light.get())) {
    return (shadowMap.get() ? pointShadowState_ : pointState_);
  }
  else if(dynamic_cast<SpotLight*>(light.get())) {
    return (shadowMap.get() ? spotShadowState_ : spotState_);
  }
  else {
    return ref_ptr<LightPass>();
  }
}

void DeferredShading::setUseAmbientLight()
{
  if(!hasAmbient_) {
    lightSequence_->joinStates(ref_ptr<State>::cast(ambientState_));
    hasAmbient_ = GL_TRUE;
  }
}

void DeferredShading::addLight(
    const ref_ptr<Light> &light,
    const ref_ptr<ShadowMap> &shadowMap)
{
  ref_ptr<LightPass> lightState = getLightState(light,shadowMap);
  if(!lightState.get()) {
    WARN_LOG("Unknown light type.");
    return;
  }
  if(lightState->empty()) {
    lightSequence_->joinStates(ref_ptr<State>::cast(lightState));
    if(hasShaderConfig_) {
      lightState->createShader(shaderCfg_);
    }
  }
  list< ref_ptr<ShaderInput> > inputs; // no additional input needed
  lightState->addLight(light, shadowMap, inputs);
}
void DeferredShading::addLight(const ref_ptr<Light> &light)
{
  addLight(light, ref_ptr<ShadowMap>());
}

void DeferredShading::removeLight(Light *l,
    const ref_ptr<LightPass> &lightState)
{
  lightState->removeLight(l);
  if(lightState->empty()) {
    lightSequence_->disjoinStates(ref_ptr<State>::cast(lightState));
  }
}
void DeferredShading::removeLight(Light *l)
{
  if(dirState_->hasLight(l)) {
    removeLight(l, ref_ptr<LightPass>::cast(dirState_));
  }
  if(dirShadowState_->hasLight(l)) {
    removeLight(l, ref_ptr<LightPass>::cast(dirShadowState_));
  }
  if(pointState_->hasLight(l)) {
    removeLight(l, ref_ptr<LightPass>::cast(pointState_));
  }
  if(pointShadowState_->hasLight(l)) {
    removeLight(l, ref_ptr<LightPass>::cast(pointShadowState_));
  }
  if(spotState_->hasLight(l)) {
    removeLight(l, ref_ptr<LightPass>::cast(spotState_));
  }
  if(spotShadowState_->hasLight(l)) {
    removeLight(l, ref_ptr<LightPass>::cast(spotShadowState_));
  }
}

const ref_ptr<LightPass>& DeferredShading::dirState() const
{ return dirState_; }
const ref_ptr<LightPass>& DeferredShading::dirShadowState() const
{ return dirShadowState_; }
const ref_ptr<LightPass>& DeferredShading::pointState() const
{ return pointState_; }
const ref_ptr<LightPass>& DeferredShading::pointShadowState() const
{ return pointShadowState_; }
const ref_ptr<LightPass>& DeferredShading::spotState() const
{ return spotState_; }
const ref_ptr<LightPass>& DeferredShading::spotShadowState() const
{ return spotShadowState_; }
const ref_ptr<DeferredAmbientLight>& DeferredShading::ambientState() const
{ return ambientState_; }
