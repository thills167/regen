/*
 * texture.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_RESOURCE_TEXTURE_H_
#define REGEN_SCENE_RESOURCE_TEXTURE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/resources.h>

#include <regen/gl-types/texture.h>

namespace regen {
namespace scene {
  /**
   * Provides Texture instances from SceneInputNode data.
   */
  class TextureResource : public ResourceProvider<Texture> {
  public:
    /**
     * Parse Texture size.
     * Allowed size modes are 'rel' and 'abs' where
     * 'rel' means that the size should be interpreted as factor
     * for given viewport.
     * @param viewport The viewport
     * @param sizeMode Size mode name
     * @param size The texture size or scale
     * @return Size in pixels for texture.
     */
    static Vec3i getSize(
        const ref_ptr<ShaderInput2i> &viewport,
        const string &sizeMode,
        const Vec3f &size);

    TextureResource();
    // Override
    ref_ptr<Texture> createResource(
        SceneParser *parser, SceneInputNode &input);
  };
}}

#endif /* REGEN_SCENE_RESOURCE_TEXTURE_H_ */
