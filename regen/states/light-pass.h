/*
 * light-pass.h
 *
 *  Created on: 13.03.2013
 *      Author: daniel
 */

#ifndef __LIGHT_PASS_H_
#define __LIGHT_PASS_H_

#include <regen/states/state.h>
#include <regen/states/shader-state.h>
#include <regen/meshes/mesh-state.h>
#include <regen/camera/light-camera.h>

namespace regen {
  /**
   * \brief Deferred shading pass.
   */
  class LightPass : public State
  {
  public:
    /**
     * @param type the light type.
     * @param shaderKey the shader key to include.
     */
    LightPass(Light::Type type, const string &shaderKey);
    /**
     * @param cfg the shader configuration.
     */
    void createShader(const StateConfig &cfg);

    /**
     * Adds a light to the rendering pass.
     * @param light the light.
     * @param lightCamera Light-perspective Camera or null reference.
     * @param shadowTexture ShadowMap or null reference.
     * @param shadowColorTexture Color-ShadowMap or null reference.
     * @param inputs render pass inputs.
     */
    void addLight(
        const ref_ptr<Light> &light,
        const ref_ptr<LightCamera> &lightCamera,
        const ref_ptr<Texture> &shadowTexture,
        const ref_ptr<Texture> &shadowColorTexture,
        const list< ref_ptr<ShaderInput> > &inputs);
    /**
     * @param l a previously added light.
     */
    void removeLight(Light *l);
    /**
     * @return true if no light was added yet.
     */
    GLboolean empty() const;
    /**
     * @param l a light.
     * @return true if the light was previously added.
     */
    GLboolean hasLight(Light *l) const;

    /**
     * @param mode the shadow filtering mode.
     */
    void setShadowFiltering(ShadowFilterMode mode);

    // override
    void enable(RenderState *rs);

  protected:
    struct LightPassLight {
      ref_ptr<Light> light;
      ref_ptr<LightCamera> camera;
      ref_ptr<Texture> shadow;
      ref_ptr<Texture> shadowColor;
      list< ref_ptr<ShaderInput> > inputs;
      list< ShaderInputLocation > inputLocations;
    };

    Light::Type lightType_;
    const string shaderKey_;

    ref_ptr<Mesh> mesh_;
    ref_ptr<ShaderState> shader_;

    list<LightPassLight> lights_;
    map< Light*, list<LightPassLight>::iterator > lightIterators_;

    GLint shadowMapLoc_;
    GLint shadowColorLoc_;
    ShadowFilterMode shadowFiltering_;
    GLuint numShadowLayer_;

    void addInputLocation(LightPassLight &l,
        const ref_ptr<ShaderInput> &in, const string &name);
    void addLightInput(LightPassLight &light);
  };
} // namespace

#endif /* __LIGHT_PASS_H_ */
