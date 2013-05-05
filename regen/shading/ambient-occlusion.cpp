/*
 * ambient-occlusion.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include <regen/textures/texture-loader.h>
#include <regen/utility/filesystem.h>

#include "ambient-occlusion.h"
using namespace regen;

AmbientOcclusion::AmbientOcclusion(const ref_ptr<Texture> &input, GLfloat sizeScale)
: FilterSequence(input, GL_FALSE), sizeScale_(sizeScale)
{
  blurSigma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  blurSigma_->setUniformData(2.0f);
  joinShaderInput(blurSigma_);

  blurNumPixels_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("numBlurPixels"));
  blurNumPixels_->setUniformData(4);
  joinShaderInput(blurNumPixels_);

  aoSamplingRadius_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoSamplingRadius"));
  aoSamplingRadius_->setUniformData(30.0);
  joinShaderInput(aoSamplingRadius_);

  aoBias_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoBias"));
  aoBias_->setUniformData(0.05);
  joinShaderInput(aoBias_);

  aoAttenuation_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("aoAttenuation"));
  aoAttenuation_->setUniformData( Vec2f(0.5,1.0) );
  joinShaderInput(aoAttenuation_);

  PathChoice randomNorPath;
  randomNorPath.choices_.push_back(filesystemPath(
      REGEN_SOURCE_DIR, "regen/res/textures/random_normals.png", "/"));
  randomNorPath.choices_.push_back(filesystemPath(
      REGEN_INSTALL_PREFIX, "share/regen/res/textures/random_normals.png", "/"));
  ref_ptr<Texture> noise = TextureLoader::load(randomNorPath.firstValidPath());
  joinStatesFront(ref_ptr<State>::manage(new TextureState(noise, "aoNoiseTexture")));

  set_format(GL_RED);
  set_internalFormat(GL_R16);
  set_pixelType(GL_BYTE);
  addFilter(ref_ptr<Filter>::manage(new Filter("ssao", sizeScale_)));
  addFilter(ref_ptr<Filter>::manage(new Filter("blur.horizontal")));
  addFilter(ref_ptr<Filter>::manage(new Filter("blur.vertical")));
}

const ref_ptr<Texture>& AmbientOcclusion::aoTexture() const
{ return output(); }
const ref_ptr<ShaderInput1f>& AmbientOcclusion::aoSamplingRadius() const
{ return aoSamplingRadius_; }
const ref_ptr<ShaderInput1f>& AmbientOcclusion::aoBias() const
{ return aoBias_; }
const ref_ptr<ShaderInput2f>& AmbientOcclusion::aoAttenuation() const
{ return aoAttenuation_; }
const ref_ptr<ShaderInput1f>& AmbientOcclusion::blurSigma() const
{ return blurSigma_; }
const ref_ptr<ShaderInput1i>& AmbientOcclusion::blurNumPixels() const
{ return blurNumPixels_; }
