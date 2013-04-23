
#include <regen/utility/filesystem.h>
#include "factory.h"
using namespace regen;

#define USE_SPOT_LIGHT
#define USE_POINT_LIGHT
#define USE_SKY
//#define USE_LIGHT_SHAFTS
#define USE_VOLUME_FOG
#define USE_HUD
#define USE_PICKING
#define USE_FLOOR
#define USE_DWARF
#define USE_FXAA

#ifdef USE_SKY
#define USE_BACKGROUND_NODE
#endif

int main(int argc, char** argv)
{
#ifdef USE_DWARF
  static const string assimpMeshFile = filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/models/psionic/dwarf/x/dwarf.x");
  static const string assimpMeshTexturesPath = filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/models/psionic/dwarf/x");
  static const BoneAnimRange animRanges[] = {
      BoneAnimRange("none",        Vec2d(  -1.0,  -1.0 )),
      BoneAnimRange("complete",    Vec2d(   0.0, 361.0 )),
      BoneAnimRange("run",         Vec2d(  16.0,  26.0 )),
      BoneAnimRange("jump",        Vec2d(  28.0,  40.0 )),
      BoneAnimRange("jumpSpot",    Vec2d(  42.0,  54.0 )),
      BoneAnimRange("crouch",      Vec2d(  56.0,  59.0 )),
      BoneAnimRange("crouchLoop",  Vec2d(  60.0,  69.0 )),
      BoneAnimRange("getUp",       Vec2d(  70.0,  74.0 )),
      BoneAnimRange("battleIdle1", Vec2d(  75.0,  88.0 )),
      BoneAnimRange("battleIdle2", Vec2d(  90.0, 110.0 )),
      BoneAnimRange("attack1",     Vec2d( 112.0, 126.0 )),
      BoneAnimRange("attack2",     Vec2d( 128.0, 142.0 )),
      BoneAnimRange("attack3",     Vec2d( 144.0, 160.0 )),
      BoneAnimRange("attack4",     Vec2d( 162.0, 180.0 )),
      BoneAnimRange("attack5",     Vec2d( 182.0, 192.0 )),
      BoneAnimRange("block",       Vec2d( 194.0, 210.0 )),
      BoneAnimRange("dieFwd",      Vec2d( 212.0, 227.0 )),
      BoneAnimRange("dieBack",     Vec2d( 230.0, 251.0 )),
      BoneAnimRange("yes",         Vec2d( 253.0, 272.0 )),
      BoneAnimRange("no",          Vec2d( 274.0, 290.0 )),
      BoneAnimRange("idle1",       Vec2d( 292.0, 325.0 )),
      BoneAnimRange("idle2",       Vec2d( 327.0, 360.0 ))
  };
#endif

  ref_ptr<QtApplication> app = initApplication(argc,argv);

  // create a root node for everything that needs camera as input
  ref_ptr<Camera> cam = createPerspectiveCamera(app.get()); {
    cam->position()->setVertex3f(0, Vec3f(16.0f,0.5f,7.0f));

    Vec3f dir(-4.0f,0.0f,-1.0f);
    dir.normalize();
    cam->direction()->setVertex3f(0, dir);
  }
  ref_ptr<EgoCameraManipulator> manipulator = createEgoCameraManipulator(app.get(), cam);

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->addChild(sceneRoot);

  ref_ptr<FBOState> gTargetState = createGBuffer(app.get());
  ref_ptr<StateNode> gTargetNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(gTargetState)));
  sceneRoot->addChild(gTargetNode);
  ref_ptr<Texture> gDiffuseTexture = gTargetState->fbo()->colorBuffer()[0];
  ref_ptr<Texture> gDepthTexture = gTargetState->fbo()->depthTexture();

  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage(new StateNode);
  gTargetNode->addChild(gBufferNode);
#ifdef USE_DWARF
  list<MeshData> dwarf = createAssimpMesh(
        app.get(), gBufferNode
      , assimpMeshFile
      , assimpMeshTexturesPath
      , Mat4f::rotationMatrix(0.0f,M_PI,0.0f)
      , Vec3f(0.0f,-2.0f,0.0f)
      , Mat4f::identity()
      , animRanges, sizeof(animRanges)/sizeof(BoneAnimRange)
  );
#endif
#ifdef USE_FLOOR
  MeshData floor = createFloorMesh(app.get(), gBufferNode,
      -2.0, Vec3f(20.0f), Vec2f(4.0f), TextureState::TRANSFER_TEXCO_RELIEF);
#endif

  const GLboolean useAmbientLight = GL_TRUE;
  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gTargetState->fbo(), sceneRoot, ShadowMap::FILTERING_NONE, useAmbientLight);
  deferredShading->ambientLight()->setVertex3f(0,Vec3f(0.2f));
  deferredShading->dirShadowState()->setShadowLayer(3);

#ifdef USE_POINT_LIGHT
  ref_ptr<Light> pointLight = createPointLight(app.get());
  pointLight->position()->setVertex3f(0,Vec3f(-4.45f,7.0f,5.7f));
  pointLight->diffuse()->setVertex3f(0,Vec3f(0.52f,0.45f,0.34f));
  pointLight->radius()->setVertex2f(0,Vec2f(10.0,20.0));
  ShadowMap::Config pointShadowCfg; {
    pointShadowCfg.size = 512;
    pointShadowCfg.depthFormat = GL_DEPTH_COMPONENT24;
    pointShadowCfg.depthType = GL_UNSIGNED_BYTE;
  }
  ref_ptr<ShadowMap> pointShadow = createShadow(
      app.get(), pointLight, cam, pointShadowCfg);
  pointShadow->addCaster(gBufferNode);
  deferredShading->addLight(pointLight, pointShadow);
#endif
#ifdef USE_SPOT_LIGHT
  ref_ptr<Light> spotLight = createSpotLight(app.get());
  spotLight->position()->setVertex3f(0,Vec3f(3.0f,8.0f,4.0f));
  spotLight->direction()->setVertex3f(0,Vec3f(-0.37f,-0.95f,-0.46f));
  spotLight->diffuse()->setVertex3f(0,Vec3f(0.2f,0.45f,0.435f));
  spotLight->radius()->setVertex2f(0,Vec2f(10.0,21.0));
  spotLight->coneAngle()->setVertex2f(0, Vec2f(0.98, 0.9));
  ShadowMap::Config spotShadowCfg; {
    spotShadowCfg.size = 512;
    spotShadowCfg.depthFormat = GL_DEPTH_COMPONENT24;
    spotShadowCfg.depthType = GL_FLOAT;
  }
  ref_ptr<ShadowMap> spotShadow = createShadow(
      app.get(), spotLight, cam, spotShadowCfg);
  spotShadow->addCaster(gBufferNode);
  deferredShading->addLight(spotLight, spotShadow);
#endif

#ifdef USE_BACKGROUND_NODE
  // create root node for background rendering, draw ontop gDiffuseTexture
  ref_ptr<StateNode> backgroundNode = createBackground(
      app.get(), gTargetState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(backgroundNode);
#ifdef USE_SKY
  // add a sky box
  ref_ptr<SkyScattering> sky = createSky(app.get(), backgroundNode);
  ShadowMap::Config sunShadowCfg; {
    sunShadowCfg.size = 1024;
    sunShadowCfg.depthFormat = GL_DEPTH_COMPONENT24;
    sunShadowCfg.depthType = GL_FLOAT;
    sunShadowCfg.numLayer = 3;
    sunShadowCfg.splitWeight = 0.8;
  }
  ref_ptr<ShadowMap> sunShadow = createShadow(
      app.get(), sky->sun(), cam, sunShadowCfg);
  sunShadow->addCaster(gBufferNode);
  deferredShading->addLight(sky->sun(), sunShadow);
#endif
#endif // USE_BACKGROUND_NODE

  ref_ptr<StateNode> postPassNode = createPostPassNode(
      app.get(), gTargetState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(postPassNode);

#ifdef USE_VOLUME_FOG
#ifdef USE_SPOT_LIGHT
  ref_ptr<VolumetricFog> volumeFogShadow =
      createVolumeFog(app.get(), gDepthTexture, postPassNode, GL_TRUE);
  app->addShaderInput("Fog",
      ref_ptr<ShaderInput>::cast(volumeFogShadow->shadowSampleStep()),
      Vec4f(0.001f), Vec4f(0.4f), Vec4i(4),
      "");
  app->addShaderInput("Fog",
      ref_ptr<ShaderInput>::cast(volumeFogShadow->shadowSampleThreshold()),
      Vec4f(0.01f), Vec4f(0.6f), Vec4i(3),
      "");
  ref_ptr<ShaderInput1f> spotExposure =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogExposure"));
  ref_ptr<ShaderInput2f> spotRadiusScale =
      ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogRadiusScale"));
  ref_ptr<ShaderInput2f> spotConeScale =
      ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogConeScale"));
  spotExposure->setUniformData(1.0);
  spotRadiusScale->setUniformData(Vec2f(0.44,0.76));
  spotConeScale->setUniformData(Vec2f(1.0));
  app->addShaderInput("Fog.Fog0[spot]",
      ref_ptr<ShaderInput>::cast(spotExposure),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "overall exposure factor.");
  app->addShaderInput("Fog.Fog0[spot]",
      ref_ptr<ShaderInput>::cast(spotRadiusScale),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "light radius scale.");
  app->addShaderInput("Fog.Fog0[spot]",
      ref_ptr<ShaderInput>::cast(spotConeScale),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "light cone scale.");

  volumeFogShadow->addSpotLight(spotLight, spotShadow,
      spotExposure, spotRadiusScale, spotConeScale);
#endif
#ifdef USE_POINT_LIGHT
  ref_ptr<VolumetricFog> volumeFog =
      createVolumeFog(app.get(), gDepthTexture, postPassNode, GL_FALSE);
  ref_ptr<ShaderInput1f> pointExposure =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogExposure"));
  ref_ptr<ShaderInput2f> pointRadiusScale =
      ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogRadiusScale"));
  pointExposure->setUniformData(2.57);
  pointRadiusScale->setUniformData(Vec2f(0.0,0.2));
  app->addShaderInput("Fog.Fog1[point]",
      ref_ptr<ShaderInput>::cast(pointExposure),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "overall exposure factor.");
  app->addShaderInput("Fog.Fog1[point]",
      ref_ptr<ShaderInput>::cast(pointRadiusScale),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "light radius scale.");

  volumeFog->addPointLight(pointLight, pointExposure, pointRadiusScale);
#endif
#endif

#ifdef USE_LIGHT_SHAFTS
  ref_ptr<SkyLightShaft> sunRay = createSkyLightShaft(
      app.get(), sky->sun(), gDiffuseTexture, gDepthTexture, postPassNode);
  sunRay->joinStatesFront(ref_ptr<State>::manage(new DrawBufferTex(
      gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_FALSE)));
#endif

#ifdef USE_FXAA
  ref_ptr<FullscreenPass> aa = createAAState(
      app.get(), gDiffuseTexture, postPassNode);
  aa->joinStatesFront(ref_ptr<State>::manage(new DrawBufferUpdate(
      gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0)));
#endif

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gTargetState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createLogoWidget(app.get(), guiNode);
  createFPSWidget(app.get(), guiNode);
#endif

#ifdef USE_PICKING
  PickingGeom *picker = createPicker(app.get(), gDepthTexture);
#ifdef USE_FLOOR
  picker->add(floor.mesh_, floor.node_, floor.shader_->shader());
#endif
#ifdef USE_DWARF
  for(list<MeshData>::iterator it=dwarf.begin(); it!=dwarf.end(); ++it) {
    picker->add(it->mesh_, it->node_, it->shader_->shader());
  }
#endif
#endif

  setBlitToScreen(app.get(), gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
