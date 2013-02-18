/*
 * video-player.cpp
 *
 *  Created on: 01.01.2013
 *      Author: daniel
 */

#include <applications/qt-ogle-application.h>
#include <ogle/config.h>

#include <ogle/meshes/rectangle.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/blit-state.h>
#include <ogle/av/audio.h>
#include <ogle/states/shader-configurer.h>

#include "video-player-widget.h"

// Resizes Framebuffer texture when the window size changed
class FramebufferResizer : public EventCallable
{
public:
  FramebufferResizer(const ref_ptr<FBOState> &fbo, GLfloat wScale, GLfloat hScale)
  : EventCallable(), fboState_(fbo), wScale_(wScale), hScale_(hScale) { }

  virtual void call(EventObject *evObject, void*) {
    OGLEApplication *app = (OGLEApplication*)evObject;
    fboState_->resize(app->glWidth()*wScale_, app->glHeight()*hScale_);
  }

protected:
  ref_ptr<FBOState> fboState_;
  GLfloat wScale_, hScale_;
};

void setBlitToScreen(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, app->glSizePtr(), attachment));
  app->renderTree()->rootNode()->addChild(
      ref_ptr<StateNode>::manage(new StateNode(blitState)));
}

ref_ptr<MeshState> createVideoWidget(
    OGLEApplication *app,
    const ref_ptr<Texture> &videoTexture,
    const ref_ptr<StateNode> &root)
{
  Rectangle::Config quadConfig;
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_FALSE;
  quadConfig.isTangentRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  quadConfig.rotation = Vec3f(0.5*M_PI, 0.0*M_PI, 0.0*M_PI);
  quadConfig.posScale = Vec3f(1.0f);
  quadConfig.texcoScale = Vec2f(-1.0f, 1.0f);
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  ref_ptr<MeshState> mesh = ref_ptr<MeshState>::manage(new Rectangle(quadConfig));

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(videoTexture));
  texState->setMapTo(MAP_TO_COLOR);
  mesh->joinStates(ref_ptr<State>::cast(texState));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  mesh->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderConfigurer.define("USE_NORMALIZED_COORDINATES", "TRUE");
  shaderState->createShader(shaderConfigurer.cfg(), "gui");

  return mesh;
}

int main(int argc, char** argv)
{
  // create and show application window
  ref_ptr<RenderTree> tree = ref_ptr<RenderTree>::manage(new RenderTree);
  ref_ptr<QtOGLEApplication> app = ref_ptr<QtOGLEApplication>::manage(
      new QtOGLEApplication(tree,argc,argv));
  app->setWaitForVSync(GL_TRUE);
  app->set_windowTitle("OpenGL player");

  // add a custom path for shader loading
  boost::filesystem::path shaderPath(PROJECT_SOURCE_DIR);
  shaderPath /= "applications";
  shaderPath /= "test";
  shaderPath /= "shader";
  OGLEApplication::setupGLSWPath(shaderPath);

  // create the main widget and connect it to applications key events
  ref_ptr<VideoPlayerWidget> widget =
      ref_ptr<VideoPlayerWidget>::manage(new VideoPlayerWidget(app.get()));
  app->connect(OGLEApplication::KEY_EVENT, ref_ptr<EventCallable>::cast(widget));
  app->connect(OGLEApplication::BUTTON_EVENT, ref_ptr<EventCallable>::cast(widget));

  widget->show();
  app->show();

  // set the render state that is used during tree traversal
  tree->set_renderState(ref_ptr<RenderState>::manage(new RenderState));

  // configure OpenAL for the video player
  AudioSystem &as = AudioSystem::get();
  as.set_listenerPosition( Vec3f(0.0) );
  as.set_listenerVelocity( Vec3f(0.0) );
  as.set_listenerOrientation( Vec3f(0.0,0.0,1.0), UP_VECTOR );

  // create render target
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(app->glWidth(), app->glHeight()));
  ref_ptr<Texture> target = fbo->addTexture(1, GL_RGB, GL_RGB);
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->addDrawBuffer(GL_COLOR_ATTACHMENT0);
  // resize fbo with window
  app->connect(OGLEApplication::RESIZE_EVENT, ref_ptr<EventCallable>::manage(
      new FramebufferResizer(fboState,1.0,1.0)));

  // create a root node (that binds the render target)
  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fboState)));
  app->renderTree()->rootNode()->addChild(sceneRoot);
  // add the video widget to the root node
  createVideoWidget(app.get(), widget->texture(), sceneRoot);

  setBlitToScreen(app.get(), fbo, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
