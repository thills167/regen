/*
 * assimp-loader.h
 *
 *  Created on: 24.10.2011
 *      Author: daniel
 */

#ifndef ASSIMP_LOADER_H_
#define ASSIMP_LOADER_H_

#include <stdexcept>

#include <regen/meshes/mesh-state.h>
#include <regen/shading/light-state.h>
#include <regen/states/material-state.h>
#include <regen/animations/animation.h>
#include <regen/animations/bones.h>
#include <regen/states/camera.h>

#include <regen/animations/animation-node.h>

namespace regen {
  /**
   * Configuration of animations defined in assets.
   */
  struct AssimpAnimationConfig {
    AssimpAnimationConfig()
    : useAnimation(GL_TRUE),
      numInstances(0u),
      forceStates(GL_TRUE),
      ticksPerSecond(20.0),
      postState(NodeAnimation::BEHAVIOR_LINEAR),
      preState(NodeAnimation::BEHAVIOR_LINEAR)
    {}
    /**
     * If false animations are ignored n the asset.
     */
    GLboolean useAnimation;
    /**
     * Number of animation copies that
     * should be created. Can be used in combination
     * with instanced rendering.
     */
    GLuint numInstances;
    /**
     * Flag indicating if pre/post states should be forced.
     */
    GLboolean forceStates;
    /**
     * Animation ticks per second. Influences how fast
     * a animation plays.
     */
    GLfloat ticksPerSecond;
    /**
     * Behavior when an animation stops.
     */
    NodeAnimation::Behavior postState;
    /**
     * Behavior when an animation starts.
     */
    NodeAnimation::Behavior preState;
  };

  /**
   * \brief Load meshes using the Open Asset import Library.
   *
   * Loading of lights,materials,meshes and bone animations
   * is supported.
   * @see http://assimp.sourceforge.net/
   */
  class AssimpImporter
  {
  public:
    /**
     * \brief Something went wrong processing the model file.
     */
    class Error : public runtime_error {
    public:
      /**
       * @param message the error message.
       */
      Error(const string &message) : runtime_error(message) {}
    };

    /**
     * @param assimpFile the file to import.
     * @param texturePath base directory for textures defined in the imported file.
     * @param assimpFlags import flags passed to assimp.
     */
    AssimpImporter(const string &assimpFile,
        const string &texturePath,
        const AssimpAnimationConfig &animConfig=AssimpAnimationConfig(),
        GLint assimpFlags=-1);
    ~AssimpImporter();

    /**
     * @return list of lights defined in the assimp file.
     */
    vector< ref_ptr<Light> >& lights();
    /**
     * @return list of materials defined in the assimp file.
     */
    vector< ref_ptr<Material> >& materials();
    /**
     * @return a node that animates the light position.
     */
    ref_ptr<LightNode> loadLightNode(const ref_ptr<Light> &light);

    vector< ref_ptr<Mesh> > loadAllMeshes(
        const Mat4f &transform, VBO::Usage usage);
    vector< ref_ptr<Mesh> > loadMeshes(
        const Mat4f &transform, VBO::Usage usage, vector<GLuint> meshIndices);

    /**
     * @return the material associated to a previously loaded meshes.
     */
    ref_ptr<Material> getMeshMaterial(Mesh *state);
    /**
     * @return list of bone animation nodes associated to given mesh.
     */
    list< ref_ptr<AnimationNode> > loadMeshBones(Mesh *meshState, NodeAnimation *anim);
    /**
     * @return number of weights used for bone animation.
     */
    GLuint numBoneWeights(Mesh *meshState);
    /**
     * @return asset animations.
     */
    const vector< ref_ptr<NodeAnimation> >& getNodeAnimations();

  protected:
    const struct aiScene *scene_;

    vector< ref_ptr<NodeAnimation> > nodeAnimations_;
    // name to node map
    map<string, struct aiNode*> nodes_;

    // user specified texture path
    string texturePath_;

    // loaded lights
    vector< ref_ptr<Light> > lights_;

    // loaded materials
    vector< ref_ptr<Material> > materials_;
    // mesh to material mapping
    map< Mesh*, ref_ptr<Material> > meshMaterials_;
    map< Mesh*, const struct aiMesh* > meshToAiMesh_;

    map< Light*, struct aiLight* > lightToAiLight_;

    // root node of skeleton
    ref_ptr<AnimationNode> rootNode_;
    // maps assimp bone nodes to Bone implementation
    map< struct aiNode*, ref_ptr<AnimationNode> > aiNodeToNode_;

    //////

    vector< ref_ptr<Light> > loadLights();

    vector< ref_ptr<Material> > loadMaterials();

    void loadMeshes(
        const struct aiNode &node,
        const Mat4f &transform,
        VBO::Usage usage,
        vector<GLuint> meshIndices,
        GLuint &currentIndex,
        vector< ref_ptr<Mesh> > &out);
    ref_ptr<Mesh> loadMesh(
        const struct aiMesh &mesh,
        const Mat4f &transform,
        VBO::Usage usage);

    void loadNodeAnimation(const AssimpAnimationConfig &animConfig);
    ref_ptr<AnimationNode> loadNodeTree();
    ref_ptr<AnimationNode> loadNodeTree(
        struct aiNode* assimpNode, ref_ptr<AnimationNode> parent);
  };
} // namespace

#endif /* ASSIMP_MODEL_H_ */
