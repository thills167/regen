
#include "factory.h"

#define USE_HDR
#define USE_HUD

int main(int argc, char** argv)
{
  ref_ptr<OGLEFltkApplication> app = initApplication(argc,argv,"HDR Reflection Map");

#ifdef USE_HDR
  ref_ptr<TextureCube> reflectionMap = createStaticReflectionMap(app.get(),
      "res/textures/cube-grace.hdr", GL_TRUE, GL_R11F_G11F_B10F);
#else
  ref_ptr<TextureCube> reflectionMap = createStaticReflectionMap(app.get(),
      "res/textures/cube-stormydays.jpg", GL_FALSE, GL_RGBA);
#endif
  reflectionMap->set_wrapping(GL_CLAMP_TO_EDGE);

  // create a root node for everything that needs camera as input
  ref_ptr<PerspectiveCamera> cam = createPerspectiveCamera(app.get());
  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->rootNode()->addChild(sceneRoot);

  // create a GBuffer node. All opaque meshes should be added to
  // this node. Shading is done deferred.
#ifdef USE_HDR
  ref_ptr<FBOState> gBufferState = createGBuffer(app.get(),1.0,1.0,GL_RGB16F);
#else
  ref_ptr<FBOState> gBufferState = createGBuffer(app.get(),1.0,1.0,GL_RGBA);
#endif
  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(gBufferState)));
  ref_ptr<Texture> gDiffuseTexture = gBufferState->fbo()->colorBuffer()[0];
  sceneRoot->addChild(gBufferNode);
  createReflectionSphere(app.get(), reflectionMap, gBufferNode);

  // create root node for background rendering, draw ontop gDiffuseTexture
  ref_ptr<StateNode> backgroundNode = createBackground(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(backgroundNode);
  createSkyCube(app.get(), reflectionMap, backgroundNode);

  ref_ptr<BlurState> blur = createBlurState(
      app.get(), gDiffuseTexture, backgroundNode);
  // switch gDiffuseTexture buffer (last rendering was ontop)
  blur->joinStatesFront(ref_ptr<State>::manage(new NextTextureBuffer(gDiffuseTexture)));
  blur->joinStates(ref_ptr<State>::manage(new NextTextureBuffer(gDiffuseTexture)));
  ref_ptr<Texture> blurTexture = blur->blurTexture();

  ref_ptr<Tonemap> toenmap =
      createTonemapState(app.get(), gDiffuseTexture, blurTexture, backgroundNode);
  toenmap->joinStatesFront(ref_ptr<State>::manage(
      new DrawBufferTex(gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_FALSE)));

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->rootNode()->addChild(guiNode);
  createFPSWidget(app.get(), guiNode);
#endif

  setBlitToScreen(app.get(), gBufferState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}

