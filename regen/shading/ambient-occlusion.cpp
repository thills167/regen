/*
 * ambient-occlusion.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include <regen/textures/texture-loader.h>
#include <regen/states/shader-configurer.h>

#include "ambient-occlusion.h"
using namespace regen;

AmbientOcclusion::AmbientOcclusion(const ref_ptr<Texture> &input, GLfloat sizeScale)
: FilterSequence(input, GL_FALSE), sizeScale_(sizeScale)
{
  blurSigma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  blurSigma_->setUniformData(2.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(blurSigma_));

  blurNumPixels_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("numBlurPixels"));
  blurNumPixels_->setUniformData(4);
  joinShaderInput(ref_ptr<ShaderInput>::cast(blurNumPixels_));

  aoSamplingRadius_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoSamplingRadius"));
  aoSamplingRadius_->setUniformData(30.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoSamplingRadius_));

  aoBias_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoBias"));
  aoBias_->setUniformData(0.05);
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoBias_));

  aoAttenuation_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("aoAttenuation"));
  aoAttenuation_->setUniformData( Vec2f(0.5,1.0) );
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoAttenuation_));

  ref_ptr<Texture> noise = TextureLoader::load("res/textures/random_normals.png");
  joinStatesFront(ref_ptr<State>::manage(new TextureState(noise, "aoNoiseTexture")));

  setClearColor(Vec4f(0.0));
  set_format(GL_RGBA);
  set_internalFormat(GL_INTENSITY);
  set_pixelType(GL_BYTE);
  addFilter(ref_ptr<Filter>::manage(new Filter("ssao", sizeScale_)));
  addFilter(ref_ptr<Filter>::manage(new Filter("blur.horizontal")));
  addFilter(ref_ptr<Filter>::manage(new Filter("blur.vertical")));
}

const ref_ptr<Texture>& AmbientOcclusion::aoTexture() const
{
  return output();
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::aoSamplingRadius() const
{
  return aoSamplingRadius_;
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::aoBias() const
{
  return aoBias_;
}
const ref_ptr<ShaderInput2f>& AmbientOcclusion::aoAttenuation() const
{
  return aoAttenuation_;
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::blurSigma() const
{
  return blurSigma_;
}
const ref_ptr<ShaderInput1i>& AmbientOcclusion::blurNumPixels() const
{
  return blurNumPixels_;
}
