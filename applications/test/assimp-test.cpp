
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/states/assimp-importer.h>
#include <ogle/states/mesh-state.h>
#include <ogle/animations/animation-manager.h>

#include <applications/glut-render-tree.h>

class AnimStoppedHandler : public EventCallable
{
public:
  map< string, Vec2d > animationRanges_;
  AnimStoppedHandler(const map< string, Vec2d > &animationRanges)
  : EventCallable(),
    animationRanges_(animationRanges)
  {
  }
  void call(EventObject *ev, void *data)
  {
    NodeAnimation *anim = (NodeAnimation*)ev;

    GLint i = rand()%animationRanges_.size();
    GLint index = 0;
    for(map< string, Vec2d >::iterator
        it=animationRanges_.begin(); it!=animationRanges_.end(); ++it)
    {
      if(index==i) {
        anim->setAnimationIndexActive(0,
            it->second + Vec2d(-1.0, -1.0) );
        break;
      }
      ++index;
    }
  }
};

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Assimp Model + Bones");

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      800,600,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  application->setLight();
  application->camManipulator()->setStepLength(0.0f,0.0f);
  application->camManipulator()->set_radius(9.0f, 0.0f);

  {
    string modelPath = "res/models/psionic/dwarf/x";
    string modelName = "dwarf2.x";

    AssimpImporter importer(
        modelPath + "/" + modelName,
        modelPath);

    list< ref_ptr<MeshState> > meshes = importer.loadMeshes(Vec3f( 0.0f ));
    Vec3f translation( 0.0, -0.5f*15.0f*0.3f, 0.0f );

    ref_ptr<ModelTransformationState> modelMat;
    ref_ptr<Material> material;

    for(list< ref_ptr<MeshState> >::iterator
        it=meshes.begin(); it!=meshes.end(); ++it)
    {
      ref_ptr<MeshState> mesh = *it;

      material = importer.getMeshMaterial(mesh.get());
      material->set_shading(Material::PHONG_SHADING);
      material->set_specular( Vec4f(0.0f) );

      modelMat = ref_ptr<ModelTransformationState>::manage(
          new ModelTransformationState);
      modelMat->translate(translation, 0.0f);

      ref_ptr<BonesState> bonesState =
          importer.loadMeshBones(mesh.get());
      if(bonesState.get()==NULL) {
        WARN_LOG("No bones state!");
      } else {
        modelMat->joinStates(ref_ptr<State>::cast(bonesState));
      }

      ref_ptr<StateNode> meshNode = application->addMesh(mesh, modelMat, material);
    }

    // mapping from different types of animations
    // to matching ticks
    map< string, Vec2d > animationRanges;
    animationRanges["none"] = Vec2d( -1.0, -1.0 );
    animationRanges["complete"] = Vec2d( 0.0, 361.0 );
    animationRanges["run"] = Vec2d( 16.0, 26.0 );
    animationRanges["jump"] = Vec2d( 28.0, 40.0 );
    animationRanges["jumpSpot"] = Vec2d( 42.0, 54.0 );
    animationRanges["crouch"] = Vec2d( 56.0, 59.0 );
    animationRanges["crouchLoop"] = Vec2d( 60.0, 69.0 );
    animationRanges["getUp"] = Vec2d( 70.0, 74.0 );
    animationRanges["battleIdle1"] = Vec2d( 75.0, 88.0 );
    animationRanges["battleIdle2"] = Vec2d( 90.0, 110.0 );
    animationRanges["attack1"] = Vec2d( 112.0, 126.0 );
    animationRanges["attack2"] = Vec2d( 128.0, 142.0 );
    animationRanges["attack3"] = Vec2d( 144.0, 160.0 );
    animationRanges["attack4"] = Vec2d( 162.0, 180.0 );
    animationRanges["attack5"] = Vec2d( 182.0, 192.0 );
    animationRanges["block"] = Vec2d( 194.0, 210.0 );
    animationRanges["dieFwd"] = Vec2d( 212.0, 227.0 );
    animationRanges["dieBack"] = Vec2d( 230.0, 251.0 );
    animationRanges["yes"] = Vec2d( 253.0, 272.0 );
    animationRanges["no"] = Vec2d( 274.0, 290.0 );
    animationRanges["idle1"] = Vec2d( 292.0, 325.0 );
    animationRanges["idle2"] = Vec2d( 327.0, 360.0 );

    bool forceChannelStates=true;
    AnimationBehaviour forcedPostState = ANIM_BEHAVIOR_LINEAR;
    AnimationBehaviour forcedPreState = ANIM_BEHAVIOR_LINEAR;
    double defaultTicksPerSecond=20.0;
    ref_ptr<NodeAnimation> boneAnim = importer.loadNodeAnimation(
        forceChannelStates,
        forcedPostState,
        forcedPreState,
        defaultTicksPerSecond);
    boneAnim->set_timeFactor(1.0);

    ref_ptr<EventCallable> animStopped = ref_ptr<EventCallable>::manage(
        new AnimStoppedHandler(animationRanges) );
    boneAnim->connect( NodeAnimation::ANIMATION_STOPPED, animStopped );

    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(boneAnim));

    animStopped->call(boneAnim.get(), NULL);
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  application->addSkyBox("res/textures/cube-clouds");
  application->setShowFPS();

  // TODO: screen blit must know screen width/height
  application->setBlitToScreen(
      fboState->fbo(), GL_COLOR_ATTACHMENT0);

  application->mainLoop();
  return 0;
}
