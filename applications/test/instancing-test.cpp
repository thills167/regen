
#include "factory.h"

#define USE_SPOT_LIGHT
#define USE_POINT_LIGHT
#define USE_SKY
#define USE_HUD

// Loads Meshes from File using Assimp. Optionally Bone animations are loaded.
void createAssimpMeshInstanced(
    OGLEApplication *app,
    const ref_ptr<StateNode> &root,
    const string &modelFile,
    const string &texturePath,
    const Mat4f &meshRotation,
    const Vec3f &meshTranslation,
    const BoneAnimRange *animRanges=NULL,
    GLuint numAnimationRanges=0,
    GLdouble ticksPerSecond=20.0)
{
#define RANDOM (rand()%100)/100.0f

  const GLuint numInstancesX = 10;
  const GLuint numInstancesY = 10;
  const GLuint numInstances = numInstancesX*numInstancesY;
  // two instances play the same animation
  const GLint numInstancedAnimations = numInstances/2;

  // import file
  AssimpImporter importer(modelFile, texturePath);

  ref_ptr<ModelTransformation> modelMat =
      createInstancedModelMat(numInstancesX, numInstancesY, 8.0);
  // defines offset to matrix tbo for each instance
  GLint *boneOffset = new int[numInstances];
  for(GLuint i=0; i<numInstances; ++i) boneOffset[i] = numInstancedAnimations*RANDOM;

  // load meshes
  list< ref_ptr<MeshState> > meshes = importer.loadMeshes();
  // load node animations, copy the animation for each different animation that
  // should be played by different instances
  list< ref_ptr<NodeAnimation> > instanceAnimations;
  ref_ptr<NodeAnimation> boneAnim = importer.loadNodeAnimation(
      GL_TRUE, ANIM_BEHAVIOR_LINEAR, ANIM_BEHAVIOR_LINEAR, ticksPerSecond);
  instanceAnimations.push_back(boneAnim);
  for(GLint i=1; i<numInstancedAnimations; ++i) instanceAnimations.push_back(boneAnim->copy());

  for(list< ref_ptr<MeshState> >::iterator
      it=meshes.begin(); it!=meshes.end(); ++it)
  {
    ref_ptr<MeshState> &mesh = *it;

    mesh->joinStates(
        ref_ptr<State>::cast(importer.getMeshMaterial(mesh.get())));
    mesh->joinStates(ref_ptr<State>::cast(modelMat));

    if(boneAnim.get()) {
      list< ref_ptr<AnimationNode> > meshBones;
      GLuint boneCount = 0;
      for(list< ref_ptr<NodeAnimation> >::iterator
          it=instanceAnimations.begin(); it!=instanceAnimations.end(); ++it)
      {
        list< ref_ptr<AnimationNode> > ibonNodes = importer.loadMeshBones(mesh.get(), it->get());
        boneCount = ibonNodes.size();
        meshBones.insert(meshBones.end(), ibonNodes.begin(), ibonNodes.end() );
      }
      GLuint numBoneWeights = importer.numBoneWeights(mesh.get());

      ref_ptr<BonesState> bonesState = ref_ptr<BonesState>::manage(
          new BonesState(meshBones, numBoneWeights));
      mesh->joinStates(ref_ptr<State>::cast(bonesState));
      AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(bonesState));

      // defines offset to matrix tbo for each instance
      ref_ptr<ShaderInput1f> u_boneOffset =
          ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("boneOffset"));
      u_boneOffset->setInstanceData(numInstances, 1, NULL);
      GLfloat *boneOffset_ = (GLfloat*)u_boneOffset->dataPtr();
      for(GLuint i=0; i<numInstances; ++i) boneOffset_[i] = boneCount*boneOffset[i];
      mesh->setInput(ref_ptr<ShaderInput>::cast(u_boneOffset));
    }

    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
    mesh->joinStates(ref_ptr<State>::cast(shaderState));

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(mesh)));
    root->addChild(meshNode);

    ShaderConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
    shaderState->createShader(shaderConfigurer.cfg(), "mesh");
  }

  for(list< ref_ptr<NodeAnimation> >::iterator
      it=instanceAnimations.begin(); it!=instanceAnimations.end(); ++it)
  {
    ref_ptr<NodeAnimation> &anim = *it;
    ref_ptr<EventCallable> animStopped = ref_ptr<EventCallable>::manage(
        new AnimationRangeUpdater(animRanges,numAnimationRanges));
    anim->connect( NodeAnimation::ANIMATION_STOPPED, animStopped );
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(anim));
    animStopped->call(anim.get(), NULL);
  }

#undef RANDOM
}

int main(int argc, char** argv)
{
  static const string assimpMeshFile = "res/models/psionic/dwarf/x/dwarf2.x";
  static const string assimpMeshTexturesPath = "res/models/psionic/dwarf/x";
  static const BoneAnimRange animRanges[] = {
      (BoneAnimRange) {"none",        Vec2d(  -1.0,  -1.0 )},
      (BoneAnimRange) {"complete",    Vec2d(   0.0, 361.0 )},
      (BoneAnimRange) {"run",         Vec2d(  16.0,  26.0 )},
      (BoneAnimRange) {"jump",        Vec2d(  28.0,  40.0 )},
      (BoneAnimRange) {"jumpSpot",    Vec2d(  42.0,  54.0 )},
      (BoneAnimRange) {"crouch",      Vec2d(  56.0,  59.0 )},
      (BoneAnimRange) {"crouchLoop",  Vec2d(  60.0,  69.0 )},
      (BoneAnimRange) {"getUp",       Vec2d(  70.0,  74.0 )},
      (BoneAnimRange) {"battleIdle1", Vec2d(  75.0,  88.0 )},
      (BoneAnimRange) {"battleIdle2", Vec2d(  90.0, 110.0 )},
      (BoneAnimRange) {"attack1",     Vec2d( 112.0, 126.0 )},
      (BoneAnimRange) {"attack2",     Vec2d( 128.0, 142.0 )},
      (BoneAnimRange) {"attack3",     Vec2d( 144.0, 160.0 )},
      (BoneAnimRange) {"attack4",     Vec2d( 162.0, 180.0 )},
      (BoneAnimRange) {"attack5",     Vec2d( 182.0, 192.0 )},
      (BoneAnimRange) {"block",       Vec2d( 194.0, 210.0 )},
      (BoneAnimRange) {"dieFwd",      Vec2d( 212.0, 227.0 )},
      (BoneAnimRange) {"dieBack",     Vec2d( 230.0, 251.0 )},
      (BoneAnimRange) {"yes",         Vec2d( 253.0, 272.0 )},
      (BoneAnimRange) {"no",          Vec2d( 274.0, 290.0 )},
      (BoneAnimRange) {"idle1",       Vec2d( 292.0, 325.0 )},
      (BoneAnimRange) {"idle2",       Vec2d( 327.0, 360.0 )}
  };

  ref_ptr<OGLEFltkApplication> app = initApplication(argc,argv,"Assimp Mesh | Instanced Bone Animation | Sky | Distance Fog");
  // global config
  DirectionalShadowMap::set_numSplits(3);

  // create a root node for everything that needs camera as input
  ref_ptr<PerspectiveCamera> cam = createPerspectiveCamera(app.get());
  ref_ptr<LookAtCameraManipulator> manipulator = createLookAtCameraManipulator(app.get(), cam);
  manipulator->set_height( 5.2f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 60.0f );
  manipulator->set_degree( M_PI*1.0 );
  manipulator->setStepLength( M_PI*0.0006 );

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->addChild(sceneRoot);

  ref_ptr<Frustum> frustum = ref_ptr<Frustum>::manage(new Frustum);
  frustum->setProjection(cam->fov(), cam->aspect(), cam->near(), cam->far());

  // create a GBuffer node. All opaque meshes should be added to
  // this node. Shading is done deferred.
  ref_ptr<FBOState> gBufferState = createGBuffer(app.get());
  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(gBufferState)));
  ref_ptr<Texture> gDiffuseTexture = gBufferState->fbo()->colorBuffer()[0];
  ref_ptr<Texture> gSpecularTexture = gBufferState->fbo()->colorBuffer()[1];
  ref_ptr<Texture> gNorWorldTexture = gBufferState->fbo()->colorBuffer()[2];
  ref_ptr<Texture> gDepthTexture = gBufferState->fbo()->depthTexture();
  sceneRoot->addChild(gBufferNode);
  createAssimpMeshInstanced(
        app.get(), gBufferNode
      , assimpMeshFile
      , assimpMeshTexturesPath
      , Mat4f::rotationMatrix(0.0f,M_PI,0.0f)
      , Vec3f(0.0f,-2.0f,0.0f)
      , animRanges, sizeof(animRanges)/sizeof(BoneAnimRange)
  );
  createFloorMesh(app.get(), gBufferNode, 0.0f, Vec3f(100.0f), Vec2f(20.0f));
  // XXX: invalid operation bug
  //createPicker(gBufferNode);

  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gBufferState->fbo(), sceneRoot, ShadowMap::FILTERING_NONE);

  // create root node for background rendering, draw ontop gDiffuseTexture
  ref_ptr<StateNode> backgroundNode = createBackground(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(backgroundNode);

  // add a sky box
  ref_ptr<DynamicSky> sky = createSky(app.get(), backgroundNode);
  //sky->setMars();
  sky->setEarth();
  ref_ptr<DirectionalShadowMap> sunShadow = createSunShadow(sky, cam, frustum, 1024);
  sunShadow->addCaster(gBufferNode);
  deferredShading->addLight(sky->sun(), sunShadow);

  ref_ptr<DistanceFog> dfog = createDistanceFog(app.get(), Vec3f(1.0f),
      sky->cubeMap(), gDepthTexture, backgroundNode);
  dfog->fogEnd()->setVertex1f(0,150.0f);

  // XXX:
  //ref_ptr<SSAO> ao = createSSAOState(
  //    app.get(), gDepthTexture, gNorWorldTexture, backgroundNode);

  ref_ptr<StateNode> postPassNode = createPostPassNode(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(postPassNode);

  //ref_ptr<SkyLightShaft> sunRay = createSkyLightShaft(
  //    app.get(), sky->sun(), gDiffuseTexture, gDepthTexture, postPassNode);
  //sunRay->joinStatesFront(ref_ptr<State>::manage(new DrawBufferTex(
  //    gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_FALSE)));

  ref_ptr<AntiAliasing> aa = createAAState(
      app.get(), gDiffuseTexture, postPassNode);
  aa->joinStatesFront(ref_ptr<State>::manage(new DrawBufferTex(
      gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_FALSE)));

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createFPSWidget(app.get(), guiNode);
#endif

  setBlitToScreen(app.get(), gBufferState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
