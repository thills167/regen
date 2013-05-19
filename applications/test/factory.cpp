/*
 * factory.cpp
 *
 *  Created on: 18.02.2013
 *      Author: daniel
 */

#include <string>
#include <GL/glew.h>
#include <regen/config.h>
#include <regen/utility/filesystem.h>
#include "factory.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

// Defeat evil windows defines...
#ifdef KEY_EVENT
#undef KEY_EVENT
#endif
#ifdef BUTTON_EVENT
#undef BUTTON_EVENT
#endif
#ifdef MOUSE_MOTION_EVENT
#undef MOUSE_MOTION_EVENT
#endif
#ifdef RESIZE_EVENT
#undef RESIZE_EVENT
#endif

namespace regen {

// Resizes Framebuffer texture when the window size changed
class FBOResizer : public EventHandler
{
public:
  FBOResizer(const ref_ptr<FBOState> &fbo, GLfloat wScale, GLfloat hScale)
  : EventHandler(), fboState_(fbo), wScale_(wScale), hScale_(hScale) { }

  void call(EventObject *evObject, EventData*) {
    Application *app = (Application*)evObject;
    const Vec2i& winSize = app->windowViewport()->getVertex(0);
    fboState_->resize(winSize.x*wScale_, winSize.y*hScale_);
  }

protected:
  ref_ptr<FBOState> fboState_;
  GLfloat wScale_, hScale_;
};
// Resizes FilterSequence textures when the window size changed
class ResizableResizer : public EventHandler
{
public:
  ResizableResizer(const ref_ptr<Resizable> &f)
  : EventHandler(), f_(f) { }

  void call(EventObject *evObject, EventData*) { f_->resize(); }

protected:
  ref_ptr<Resizable> f_;
};

// Updates Camera Projection when window size changes
class ProjectionUpdater : public EventHandler
{
public:
  ProjectionUpdater(const ref_ptr<Camera> &cam,
      GLfloat fov, GLfloat near, GLfloat far)
  : EventHandler(), cam_(cam), fov_(fov), near_(near), far_(far) { }

  void call(EventObject *evObject, EventData*) {
    Application *app = (Application*)evObject;
    const Vec2i& winSize = app->windowViewport()->getVertex(0);
    GLfloat aspect = winSize.x/(GLfloat)winSize.y;

    Mat4f &view = *(Mat4f*)cam_->view()->dataPtr();
    Mat4f &viewInv = *(Mat4f*)cam_->viewInverse()->dataPtr();
    Mat4f &proj = *(Mat4f*)cam_->projection()->dataPtr();
    Mat4f &projInv = *(Mat4f*)cam_->projectionInverse()->dataPtr();
    Mat4f &viewproj = *(Mat4f*)cam_->viewProjection()->dataPtr();
    Mat4f &viewprojInv = *(Mat4f*)cam_->viewProjectionInverse()->dataPtr();

    proj = Mat4f::projectionMatrix(fov_, aspect, near_, far_);
    projInv = proj.projectionInverse();
    viewproj = view * proj;
    viewprojInv = projInv * viewInv;
    cam_->projection()->nextStamp();
    cam_->projectionInverse()->nextStamp();
    cam_->viewProjection()->nextStamp();
    cam_->viewProjectionInverse()->nextStamp();
  }

protected:
  ref_ptr<Camera> cam_;
  GLfloat fov_, near_, far_;
};

class FramebufferClear : public State
{
public:
  FramebufferClear() : State() {}
  virtual void enable(RenderState *rs) {
    rs->clearColor().push(ClearColor(0.0));
    glClear(GL_COLOR_BUFFER_BIT);
    rs->clearColor().pop();
  }
};


// create application window and set up OpenGL
ref_ptr<QtApplication> initApplication(int argc, char** argv, const string &windowTitle)
{
#ifdef Q_WS_X11
#ifndef SINGLE_THREAD_GUI_AND_GRAPHICS
  // needed because gui and render thread use xlib calls
  XInitThreads();
#endif
#endif
  QGLFormat glFormat(
    QGL::SingleBuffer
   |QGL::NoAlphaChannel
   |QGL::NoAccumBuffer
   |QGL::NoDepthBuffer
   |QGL::NoStencilBuffer
   |QGL::NoStereoBuffers
   |QGL::NoSampleBuffers);
  glFormat.setSwapInterval(0);
  glFormat.setDirectRendering(true);
  glFormat.setRgba(true);
  glFormat.setOverlay(false);
  glFormat.setVersion(3,3);
  glFormat.setProfile(QGLFormat::CoreProfile);

  // create and show application window
  ref_ptr<QtApplication> app = ref_ptr<QtApplication>::alloc(argc,(const char**)argv,glFormat);
  app->setupLogging();
  app->toplevelWidget()->setWindowTitle(windowTitle.c_str());
  app->show();
  return app;
}

// Blits fbo attachment to screen
void setBlitToScreen(
    QtApplication *app,
    const ref_ptr<FBO> &fbo,
    const ref_ptr<Texture> &texture,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<BlitTexToScreen>::alloc(
      fbo, texture, app->windowViewport(), attachment);
  app->renderTree()->addChild(ref_ptr<StateNode>::alloc(blitState));
}
void setBlitToScreen(
    QtApplication *app,
    const ref_ptr<FBO> &fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<BlitToScreen>::alloc(fbo, app->windowViewport(), attachment);
  app->renderTree()->addChild(ref_ptr<StateNode>::alloc(blitState));
}

ref_ptr<TextureCube> createStaticReflectionMap(
    QtApplication *app,
    const string &file,
    const GLboolean flipBackFace,
    const GLenum textureFormat,
    const GLfloat aniso)
{
  ref_ptr<TextureCube> reflectionMap = textures::loadCube(
      file,flipBackFace,GL_FALSE,textureFormat);

  reflectionMap->begin(RenderState::get());
  reflectionMap->aniso().push(aniso);
  reflectionMap->filter().push(TextureFilter(GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR));
  reflectionMap->wrapping().push(GL_CLAMP_TO_EDGE);
  reflectionMap->setupMipmaps(GL_DONT_CARE);
  reflectionMap->end(RenderState::get());

  return reflectionMap;
}

class SelectionChangedHandler : public EventHandler
{
public:
  SelectionChangedHandler(QtApplication *app)
  : EventHandler(), app_(app)
  {
    pickedMesh_=NULL;
    pickedInstance_=0;
    pickedObject_=0;
  }
  void call(EventObject *evObject, EventData *_ev)
  {
    PickingGeom::PickEvent *ev = (PickingGeom::PickEvent*)_ev;
    if(app_->isMouseEntered()->getVertex(0)) {
      pickedMesh_ = ev->state;
      pickedInstance_ = ev->instanceId;
      pickedObject_ = ev->objectId;

      REGEN_INFO("Selection changed. id=" << pickedObject_ <<
          " instance=" << pickedInstance_);
    }
  }

protected:
  QtApplication *app_;

  Mesh *pickedMesh_;
  GLint pickedInstance_;
  GLint pickedObject_;
};

PickingGeom* createPicker(QtApplication *app,
    const ref_ptr<Camera> &camera,
    GLdouble interval)
{
  PickingGeom *picker = new PickingGeom(
      app->mouseTexco(), camera->projectionInverse());
  picker->set_pickInterval(interval);
  picker->State::connect(PickingGeom::PICK_EVENT,
      ref_ptr<SelectionChangedHandler>::alloc(app));

  return picker;
}

/////////////////////////////////////
//// Camera
/////////////////////////////////////

class EgoCamMotion : public EventHandler
{
public:
  EgoCamMotion(const ref_ptr<EgoCameraManipulator> &m, const GLboolean &buttonPressed)
  : EventHandler(), m_(m), buttonPressed_(buttonPressed)
  {
    sensitivity_= 0.0002f;
  }

  void call(EventObject *evObject, EventData *data)
  {
    if(buttonPressed_) {
      Application::MouseMotionEvent *ev = (Application::MouseMotionEvent*)data;
      Vec2f delta((float)ev->dx, (float)ev->dy);
      m_->lookLeft(delta.x*sensitivity_);
      m_->lookUp(delta.y*sensitivity_);
    }
  }
  ref_ptr<EgoCameraManipulator> m_;
  const GLboolean &buttonPressed_;
  GLfloat sensitivity_;
};
class EgoCamButton : public EventHandler
{
public:
  EgoCamButton(const ref_ptr<EgoCameraManipulator> &m)
  : EventHandler(), m_(m), buttonPressed_(GL_FALSE) {}

  void call(EventObject *evObject, EventData *data)
  {
    Application::ButtonEvent *ev = (Application::ButtonEvent*)data;
    if(ev->button == 0) buttonPressed_ = ev->pressed;
  }
  ref_ptr<EgoCameraManipulator> m_;
  GLboolean buttonPressed_;
};
class EgoCamKey : public EventHandler
{
public:
  EgoCamKey(const ref_ptr<EgoCameraManipulator> &m)
  : EventHandler(), m_(m) {}

  void call(EventObject *evObject, EventData *data)
  {
    Application::KeyEvent *ev = (Application::KeyEvent*)data;
    if(ev->key == Qt::Key_W || ev->key == Qt::Key_Up) {
      m_->moveForward(!ev->isUp);
    }
    else if(ev->key == Qt::Key_S || ev->key == Qt::Key_Down) {
      m_->moveBackward(!ev->isUp);
    }
    else if(ev->key == Qt::Key_A || ev->key == Qt::Key_Left) {
      m_->moveLeft(!ev->isUp);
    }
    else if(ev->key == Qt::Key_D || ev->key == Qt::Key_Right) {
      m_->moveRight(!ev->isUp);
    }
  }

  ref_ptr<EgoCameraManipulator> m_;
};

class LookAtMotion : public EventHandler
{
public:
  LookAtMotion(const ref_ptr<LookAtCameraManipulator> &m, const GLboolean &buttonPressed)
  : EventHandler(), m_(m), buttonPressed_(buttonPressed) {}

  void call(EventObject *evObject, EventData *data)
  {
    Application::MouseMotionEvent *ev = (Application::MouseMotionEvent*)data;
    if(buttonPressed_) {
      m_->set_height(m_->height() + ((float)ev->dy)*stepX_, ev->dt );
      m_->setStepLength( ((float)ev->dx)*stepY_, ev->dt );
    }
  }
  ref_ptr<LookAtCameraManipulator> m_;
  const GLboolean &buttonPressed_;
  GLfloat stepX_;
  GLfloat stepY_;
};
class LookAtButton : public EventHandler
{
public:
  LookAtButton(const ref_ptr<LookAtCameraManipulator> &m)
  : EventHandler(), m_(m), buttonPressed_(GL_FALSE) {}

  void call(EventObject *evObject, EventData *data)
  {
    Application::ButtonEvent *ev = (Application::ButtonEvent*)data;

    if(ev->button == 0) {
      buttonPressed_ = ev->pressed;
      if(ev->pressed) {
        m_->setStepLength( 0.0f );
      }
      } else if (ev->button == 4 && !ev->pressed) {
        m_->set_radius( m_->radius()+scrollStep_ );
      } else if (ev->button == 3 && !ev->pressed) {
        m_->set_radius( m_->radius()-scrollStep_ );
    }
  }
  ref_ptr<LookAtCameraManipulator> m_;
  GLboolean buttonPressed_;
  GLfloat scrollStep_;
};

ref_ptr<LookAtCameraManipulator> createLookAtCameraManipulator(
    QtApplication *app,
    const ref_ptr<Camera> &cam,
    const GLfloat &scrollStep,
    const GLfloat &stepX,
    const GLfloat &stepY,
    const GLuint &interval)
{
  ref_ptr<LookAtCameraManipulator> manipulator =
      ref_ptr<LookAtCameraManipulator>::alloc(cam,interval);
  manipulator->set_height( 0.0f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 5.0f );
  manipulator->set_degree( 0.0f );
  manipulator->setStepLength( M_PI*0.01f );

  ref_ptr<LookAtButton> buttonCallable = ref_ptr<LookAtButton>::alloc(manipulator);
  buttonCallable->scrollStep_ = scrollStep;
  app->connect(Application::BUTTON_EVENT, buttonCallable);

  ref_ptr<LookAtMotion> motionCallable =
      ref_ptr<LookAtMotion>::alloc(manipulator, buttonCallable->buttonPressed_);
  motionCallable->stepX_ = stepX;
  motionCallable->stepY_ = stepY;
  app->connect(Application::MOUSE_MOTION_EVENT, motionCallable);

  return manipulator;
}

ref_ptr<EgoCameraManipulator> createEgoCameraManipulator(
    QtApplication *app, const ref_ptr<Camera> &cam,
    GLfloat moveSpeed, GLfloat mouseSensitivity)
{
  ref_ptr<EgoCameraManipulator> manipulator =
      ref_ptr<EgoCameraManipulator>::alloc(cam);
  manipulator->set_moveAmount(moveSpeed);

  ref_ptr<EgoCamKey> keyCallable = ref_ptr<EgoCamKey>::alloc(manipulator);
  app->connect(Application::KEY_EVENT, keyCallable);

  ref_ptr<EgoCamButton> buttonCallable = ref_ptr<EgoCamButton>::alloc(manipulator);
  app->connect(Application::BUTTON_EVENT, buttonCallable);

  ref_ptr<EgoCamMotion> motionCallable =
      ref_ptr<EgoCamMotion>::alloc(manipulator, buttonCallable->buttonPressed_);
  motionCallable->sensitivity_ = mouseSensitivity;
  app->connect(Application::MOUSE_MOTION_EVENT, motionCallable);

  return manipulator;
}

ref_ptr<Camera> createPerspectiveCamera(
    QtApplication *app,
    GLfloat fov,
    GLfloat near,
    GLfloat far)
{
  ref_ptr<Camera> cam = ref_ptr<Camera>::alloc();

  ref_ptr<ProjectionUpdater> projUpdater =
      ref_ptr<ProjectionUpdater>::alloc(cam, fov, near, far);
  app->connect(Application::RESIZE_EVENT, projUpdater);
  EventData evData;
  evData.eventID = Application::RESIZE_EVENT;
  projUpdater->call(app, &evData);

  return cam;
}

/////////////////////////////////////
//// Instancing
/////////////////////////////////////

ref_ptr<ModelTransformation> createInstancedModelMat(
    GLuint numInstancesX, GLuint numInstancesY, GLfloat instanceDistance)
{
#define RANDOM (rand()%100)/100.0f

  const GLuint numInstances = numInstancesX*numInstancesY;
  const GLfloat startX = -0.5f*numInstancesX*instanceDistance -0.5f;
  const GLfloat startZ = -0.5f*numInstancesY*instanceDistance -0.5f;
  const Vec3f instanceScale(1.0f);
  const Vec3f instanceRotation(0.0f,M_PI,0.0f);
  Vec3f baseTranslation(startX, 0.0f, startZ);

  ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::alloc();
  modelMat->modelMat()->setInstanceData(numInstances, 1, NULL);

  Mat4f* instancedModelMats = (Mat4f*)modelMat->modelMat()->dataPtr();
  for(GLuint x=0; x<numInstancesX; ++x)
  {
    baseTranslation.x += instanceDistance;

    for(GLuint y=0; y<numInstancesY; ++y)
    {
      baseTranslation.z += instanceDistance;

      Vec3f instanceTranslation = baseTranslation +
          Vec3f(1.5f*(0.5f-RANDOM),0.0f,1.25f*(0.5f-RANDOM));
      *instancedModelMats = Mat4f::transformationMatrix(
          instanceRotation, instanceTranslation, instanceScale).transpose();
      ++instancedModelMats;
    }

    baseTranslation.z = startZ;
  }

  // add data to vbo
  modelMat->setInput(modelMat->modelMat());

  return modelMat;

#undef RANDOM
}

/////////////////////////////////////
//// GBuffer / TBuffer
/////////////////////////////////////

// Creates render target for deferred shading.
ref_ptr<FBOState> createGBuffer(
    QtApplication *app,
    GLfloat gBufferScaleW,
    GLfloat gBufferScaleH,
    GLenum colorBufferFormat,
    GLenum depthFormat)
{
  // diffuse, specular, norWorld
  static const GLenum count[] = { 2, 1, 1 };
  static const GLenum formats[] = { GL_RGBA, GL_RGBA, GL_RGBA };
  static const GLenum internalFormats[] = { colorBufferFormat, GL_RGBA, GL_RGBA };
  static const GLenum clearBuffers[] = {
      GL_COLOR_ATTACHMENT2, // spec
      GL_COLOR_ATTACHMENT3  // norWorld
  };

  const Vec2i& winSize = app->windowViewport()->getVertex(0);
  ref_ptr<FBO> fbo = ref_ptr<FBO>::alloc(
      winSize.x*gBufferScaleW, winSize.y*gBufferScaleH);
  ref_ptr<FBOState> gBufferState = ref_ptr<FBOState>::alloc(fbo);
  fbo->createDepthTexture(GL_TEXTURE_2D, depthFormat, GL_UNSIGNED_BYTE);

  for(GLuint i=0; i<sizeof(count)/sizeof(GLenum); ++i) {
    fbo->addTexture(count[i], GL_TEXTURE_2D,
        formats[i], internalFormats[i], GL_UNSIGNED_BYTE);
    // call glDrawBuffer
    gBufferState->addDrawBuffer(GL_COLOR_ATTACHMENT0+i+1);
  }
  // make sure buffer index of diffuse texture is set to 0
  gBufferState->joinStates(ref_ptr<SetTextureIndex>::alloc(fbo->colorTextures()[0], 0));

  ClearColorState::Data clearData;
  clearData.clearColor = Vec4f(0.0f);
  clearData.colorBuffers = std::vector<GLenum>(
      clearBuffers, clearBuffers + sizeof(clearBuffers)/sizeof(GLenum));
  gBufferState->setClearColor(clearData);
  if(depthFormat!=GL_NONE) {
    gBufferState->setClearDepth();
  }

  app->connect(Application::RESIZE_EVENT,
      ref_ptr<FBOResizer>::alloc(gBufferState,gBufferScaleW,gBufferScaleH));

  return gBufferState;
}

ref_ptr<TBuffer> createTBuffer(
    QtApplication *app,
    const ref_ptr<Camera> &cam,
    const ref_ptr<Texture> &depthTexture,
    TBuffer::Mode mode,
    GLfloat tBufferScaleW,
    GLfloat tBufferScaleH)
{
  const Vec2i& winSize = app->windowViewport()->getVertex(0);
  Vec2ui bufferSize(winSize.x*tBufferScaleW, winSize.y*tBufferScaleH);
  ref_ptr<TBuffer> tBufferState = ref_ptr<TBuffer>::alloc(mode, bufferSize, depthTexture);

  app->addShaderInput("T-buffer",
      tBufferState->ambientLight(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(3),
      "the ambient light.");

  app->connect(Application::RESIZE_EVENT,
      ref_ptr<FBOResizer>::alloc(tBufferState->fboState(),tBufferScaleW,tBufferScaleH));

  return tBufferState;
}

/////////////////////////////////////
//// Post Passes
/////////////////////////////////////

// Creates root node for states rendering the background of the scene
ref_ptr<StateNode> createPostPassNode(
    QtApplication *app,
    const ref_ptr<FBO> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment)
{
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);
  fboState->setDrawBufferOntop(tex, baseAttachment);

  ref_ptr<StateNode> root = ref_ptr<StateNode>::alloc(fboState);

  // no depth writing
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::alloc();
  depthState->set_useDepthWrite(GL_FALSE);
  depthState->set_useDepthTest(GL_FALSE);
  fboState->joinStates(depthState);

  return root;
}

ref_ptr<FilterSequence> createBlurState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<StateNode> &root,
    GLint size, GLfloat sigma,
    GLboolean downsampleTwice,
    const string &treePath)
{
  ref_ptr<FilterSequence> filter = ref_ptr<FilterSequence>::alloc(input);

  ref_ptr<ShaderInput1i> blurSize = ref_ptr<ShaderInput1i>::alloc("numBlurPixels");
  blurSize->setUniformData(size);
  filter->joinShaderInput(blurSize);

  ref_ptr<ShaderInput1f> blurSigma = ref_ptr<ShaderInput1f>::alloc("blurSigma");
  blurSigma->setUniformData(sigma);
  filter->joinShaderInput(blurSigma);

  // first downsample the moments texture
  filter->addFilter(ref_ptr<Filter>::alloc("regen.utility.sampling.downsample", 0.5));
  if(downsampleTwice) {
    filter->addFilter(ref_ptr<Filter>::alloc("regen.utility.sampling.downsample", 0.5));
  }
  filter->addFilter(ref_ptr<Filter>::alloc("regen.post-passes.blur.horizontal"));
  filter->addFilter(ref_ptr<Filter>::alloc("regen.post-passes.blur.vertical"));

  ref_ptr<StateNode> blurNode = ref_ptr<StateNode>::alloc(filter);
  root->addChild(blurNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(blurNode.get());
  filter->createShader(shaderConfigurer.cfg());

  string treePath_ = (treePath.empty() ? "Blur" : treePath + ".Blur");
  app->addShaderInput(treePath_,
      blurSize,
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(0),
      "Width and height of blur kernel.");
  app->addShaderInput(treePath_,
      blurSigma,
      Vec4f(0.0f), Vec4f(50.0f), Vec4i(2),
      "Blur sigma.");

  app->connect(Application::RESIZE_EVENT, ref_ptr<ResizableResizer>::alloc(filter));

  return filter;
}

ref_ptr<DepthOfField> createDoFState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<Texture> &depthInput,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<DepthOfField> dof = ref_ptr<DepthOfField>::alloc(input,blurInput,depthInput);

  app->addShaderInput("DepthOfField",
      dof->focalDistance(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "distance to point with max sharpness in NDC space.");
  app->addShaderInput("DepthOfField",
      dof->focalWidth(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "Inner and outer focal width. Between the original and the blurred image are linear combined.");

  ref_ptr<StateNode> node = ref_ptr<StateNode>::alloc(dof);
  root->addChild(node);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  dof->createShader(shaderConfigurer.cfg());

  return dof;
}

ref_ptr<Tonemap> createTonemapState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<Tonemap> tonemap = ref_ptr<Tonemap>::alloc(input, blurInput);

  app->addShaderInput("Tonemap",
      tonemap->blurAmount(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "mix factor for input and blurred input.");
  app->addShaderInput("Tonemap",
      tonemap->exposure(),
      Vec4f(0.0f), Vec4f(50.0f), Vec4i(2),
      "overall exposure factor.");
  app->addShaderInput("Tonemap",
      tonemap->gamma(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "gamma correction factor.");
  app->addShaderInput("Tonemap.StreamRays",
      tonemap->effectAmount(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "streaming rays factor.");
  app->addShaderInput("Tonemap.StreamRays",
      tonemap->radialBlurSamples(),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(0),
      "number of radial blur samples for streaming rays.");
  app->addShaderInput("Tonemap.StreamRays",
      tonemap->radialBlurStartScale(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "initial scale of texture coordinates for streaming rays.");
  app->addShaderInput("Tonemap.StreamRays",
      tonemap->radialBlurScaleMul(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "scale factor of texture coordinates for streaming rays.");
  app->addShaderInput("Tonemap.StreamRays",
      tonemap->vignetteInner(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "inner distance for vignette effect.");
  app->addShaderInput("Tonemap.StreamRays",
      tonemap->vignetteOuter(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "outer distance for vignette effect.");

  ref_ptr<StateNode> node = ref_ptr<StateNode>::alloc(tonemap);
  root->addChild(node);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  tonemap->createShader(shaderConfigurer.cfg());

  return tonemap;
}

ref_ptr<FullscreenPass> createAAState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<FullscreenPass> aa = ref_ptr<FullscreenPass>::alloc("regen.post-passes.fxaa");

  ref_ptr<TextureState> texState;
  texState = ref_ptr<TextureState>::alloc(input, "inputTexture");
  aa->joinStatesFront(texState);

  ref_ptr<ShaderInput1f> spanMax = ref_ptr<ShaderInput1f>::alloc("spanMax");
  spanMax->setUniformData(8.0f);
  aa->joinShaderInput(spanMax);

  ref_ptr<ShaderInput1f> reduceMul = ref_ptr<ShaderInput1f>::alloc("reduceMul");
  reduceMul->setUniformData(1.0f/8.0f);
  aa->joinShaderInput(reduceMul);

  ref_ptr<ShaderInput1f> reduceMin = ref_ptr<ShaderInput1f>::alloc("reduceMin");
  reduceMin->setUniformData(1.0f/128.0f);
  aa->joinShaderInput(reduceMin);

  ref_ptr<ShaderInput3f> luma = ref_ptr<ShaderInput3f>::alloc("luma");
  luma->setUniformData(Vec3f(0.299f, 0.587f, 0.114f));
  aa->joinShaderInput(luma);

  app->addShaderInput("AntiAliasing",
      spanMax,
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2), "");
  app->addShaderInput("AntiAliasing",
      reduceMul,
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2), "");
  app->addShaderInput("AntiAliasing",
      reduceMin,
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2), "");
  app->addShaderInput("AntiAliasing",
      luma,
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2), "");

  ref_ptr<StateNode> node = ref_ptr<StateNode>::alloc(aa);
  root->addChild(node);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  aa->createShader(shaderConfigurer.cfg());

  return aa;
}

/////////////////////////////////////
//// Background/Foreground States
/////////////////////////////////////

// Creates root node for states rendering the background of the scene
ref_ptr<StateNode> createBackground(
    QtApplication *app,
    const ref_ptr<FBO> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment)
{
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);
  fboState->setDrawBufferOntop(tex, baseAttachment);

  ref_ptr<StateNode> root = ref_ptr<StateNode>::alloc(fboState);

  // no depth writing
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::alloc();
  depthState->set_useDepthWrite(GL_FALSE);
  fboState->joinStates(depthState);

  return root;
}

// Creates sky box mesh
ref_ptr<SkyScattering> createSky(QtApplication *app, const ref_ptr<StateNode> &root)
{
  ref_ptr<SkyScattering> sky = ref_ptr<SkyScattering>::alloc();
  sky->setSunElevation(0.8, 20.0, -20.0);
  //sky->set_updateInterval(1000.0);
  //sky->set_timeScale(0.0001);
  sky->set_dayTime(0.5); // middle of the day
  sky->setEarth();

  app->addShaderInput("Sky",
      sky->rayleigh(),
      Vec4f(0.0f), Vec4f(10.0f,2.0f,5.0f,1.0f), Vec4i(2),
      "rayleigh profile.");
  app->addShaderInput("Sky",
      sky->mie(),
      Vec4f(0.0f), Vec4f(0.5f,0.5f,1.0f,10.0f), Vec4i(2),
      "aerosol profile.");
  app->addShaderInput("Sky",
      sky->spotBrightness(),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "the spot brightness.");
  app->addShaderInput("Sky",
      sky->scatterStrength(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "scattering strength.");
  app->addShaderInput("Sky",
      sky->absorbtion(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "the absorbtion color.");

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(sky);
  root->addChild(meshNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  sky->createShader(shaderConfigurer.cfg());

  return sky;
}

ref_ptr<SkyBox> createSkyCube(
    QtApplication *app,
    const ref_ptr<TextureCube> &reflectionMap,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<SkyBox> mesh = ref_ptr<SkyBox>::alloc();
  mesh->setCubeMap(reflectionMap);

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
  root->addChild(meshNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  mesh->createShader(shaderConfigurer.cfg());

  return mesh;
}

ref_ptr<ParticleRain> createRain(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLuint numParticles)
{
  ref_ptr<ParticleRain> particles = ref_ptr<ParticleRain>::alloc(numParticles);
  particles->set_depthTexture(depthTexture);
  //particles->loadIntensityTextureArray(
  //    "applications/res/textures/rainTextures", "cv[0-9]+_vPositive_[0-9]+\\.dds");
  //particles->loadIntensityTexture(filesystemPath(
  //    REGEN_SOURCE_DIR, "applications/res/textures/rainTextures/cv0_vPositive_0000.dds"));
  particles->loadIntensityTexture(filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/textures/splats/flare.jpg"));
  particles->createBuffer();

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(particles);
  root->addChild(meshNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  particles->createShader(shaderConfigurer.cfg());

  app->addShaderInput("Particles.Rain.Update",
      particles->gravity(),
      Vec4f(-100.0f), Vec4f(100.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Rain.Update",
      particles->dampingFactor(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Rain.Update",
      particles->noiseFactor(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Rain.Update",
      particles->cloudPosition(),
      Vec4f(-10.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Rain.Update",
      particles->cloudRadius(),
      Vec4f(0.1f), Vec4f(100.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Rain.Update",
      particles->particleMass(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Rain.Draw",
      particles->particleSize(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Rain.Draw",
      particles->streakSize(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Rain.Draw",
      particles->brightness(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Rain.Draw",
      particles->softScale(),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "");

  return particles;
}

ref_ptr<ParticleSnow> createParticleFog(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLuint numSnowFlakes)
{
  ref_ptr<ParticleSnow> particles = ref_ptr<ParticleSnow>::alloc(numSnowFlakes);
  ref_ptr<Texture> tex = textures::load(filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/textures/splats/flare.jpg"));
  particles->set_particleTexture(tex);
  particles->set_depthTexture(depthTexture);
  particles->createBuffer();

  particles->set_cloudPositionMode(ParticleCloud::ABSOLUTE);
  particles->cloudPosition()->setVertex(0, Vec3f(2.7f,6.5f,0.0));
  particles->cloudRadius()->setVertex(0, 5.0f);
  particles->surfaceHeight()->setVertex(0, -3.0f);
  particles->particleSize()->setVertex(0, Vec2f(3.0f,0.15f));
  particles->brightness()->setVertex(0, 0.03f);
  particles->particleMass()->setVertex(0, Vec2f(0.8f, 0.1f));
  particles->dampingFactor()->setVertex(0, 2.0f);
  particles->gravity()->setVertex(0, Vec3f(-4.0f, -9.0f, 0.0f));
  particles->noiseFactor()->setVertex(0, 10.0f);
  particles->softScale()->setVertex(0,100.0f);

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(particles);
  root->addChild(meshNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  particles->createShader(shaderConfigurer.cfg());

  app->addShaderInput("Particles.Fog.Update",
      particles->gravity(),
      Vec4f(-100.0f), Vec4f(100.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Fog.Update",
      particles->dampingFactor(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Fog.Update",
      particles->noiseFactor(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Fog.Update",
      particles->cloudPosition(),
      Vec4f(-10.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Fog.Update",
      particles->surfaceHeight(),
      Vec4f(-10.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Fog.Update",
      particles->cloudRadius(),
      Vec4f(0.1f), Vec4f(100.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Fog.Update",
      particles->particleMass(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Fog.Draw",
      particles->particleSize(),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Fog.Draw",
      particles->brightness(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "");
  app->addShaderInput("Particles.Fog.Draw",
      particles->softScale(),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "");

  return particles;
}

ref_ptr<VolumetricFog> createVolumeFog(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<Texture> &tBufferColor,
    const ref_ptr<Texture> &tBufferDepth,
    const ref_ptr<StateNode> &root,
    GLboolean useShadowMapping)
{
  ref_ptr<VolumetricFog> fog = ref_ptr<VolumetricFog>::alloc();
  fog->set_gDepthTexture(depthTexture);
  fog->set_tBuffer(tBufferColor,tBufferDepth);
  if(useShadowMapping) {
    fog->setShadowFiltering(ShadowMap::FILTERING_NONE);
  }

  ref_ptr<StateNode> node = ref_ptr<StateNode>::alloc(fog);
  root->addChild(node);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  fog->createShader(shaderConfigurer.cfg());

  app->addShaderInput("Fog",
      fog->fogDistance(),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "Inner and outer fog distance to camera for volumetric Fog.");

  return fog;
}
ref_ptr<VolumetricFog> createVolumeFog(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLboolean useShadowMapping)
{
  return regen::createVolumeFog(app,depthTexture,
      ref_ptr<Texture>(), ref_ptr<Texture>(),
      root, useShadowMapping);
}

ref_ptr<DistanceFog> createDistanceFog(
    QtApplication *app,
    const Vec3f &fogColor,
    const ref_ptr<TextureCube> &skyColor,
    const ref_ptr<Texture> &gDepth,
    const ref_ptr<Texture> &tBufferColor,
    const ref_ptr<Texture> &tBufferDepth,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<DistanceFog> fog = ref_ptr<DistanceFog>::alloc();
  fog->set_gBuffer(gDepth);
  fog->set_tBuffer(tBufferColor,tBufferDepth);
  fog->set_skyColor(skyColor);
  fog->fogColor()->setVertex(0,fogColor);

  ref_ptr<StateNode> node = ref_ptr<StateNode>::alloc(fog);
  root->addChild(node);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  fog->createShader(shaderConfigurer.cfg());

  app->addShaderInput("Fog",
      fog->fogDistance(),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "Inner and outer fog distance to camera for distance Fog.");
  app->addShaderInput("Fog",
      fog->fogDensity(),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "Constant fog density for distance Fog.");

  return fog;
}
ref_ptr<DistanceFog> createDistanceFog(
    QtApplication *app,
    const Vec3f &fogColor,
    const ref_ptr<TextureCube> &skyColor,
    const ref_ptr<Texture> &gDepth,
    const ref_ptr<StateNode> &root)
{
  return regen::createDistanceFog(app,fogColor,skyColor,gDepth,
      ref_ptr<Texture>(), ref_ptr<Texture>(),
      root);
}

/////////////////////////////////////
//// Shading States
/////////////////////////////////////

// Creates deferred shading state and add to render tree
ref_ptr<DeferredShading> createShadingPass(
    QtApplication *app,
    const ref_ptr<FBO> &gBuffer,
    const ref_ptr<StateNode> &root,
    ShadowMap::FilterMode shadowFiltering,
    GLboolean useAmbientLight)
{
  ref_ptr<DeferredShading> shading = ref_ptr<DeferredShading>::alloc();

  if(useAmbientLight) {
    shading->setUseAmbientLight();
    app->addShaderInput("Light",
        shading->ambientLight(),
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(3),
        "the ambient light.");
  }
  shading->dirShadowState()->setShadowFiltering(shadowFiltering);
  shading->pointShadowState()->setShadowFiltering(shadowFiltering);
  shading->spotShadowState()->setShadowFiltering(shadowFiltering);

  ref_ptr<Texture> gDiffuseTexture = gBuffer->colorTextures()[0];
  ref_ptr<Texture> gSpecularTexture = gBuffer->colorTextures()[2];
  ref_ptr<Texture> gNorWorldTexture = gBuffer->colorTextures()[3];
  ref_ptr<Texture> gDepthTexture = gBuffer->depthTexture();
  shading->set_gBuffer(
      gDepthTexture, gNorWorldTexture,
      gDiffuseTexture, gSpecularTexture);

  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(gBuffer);
  fboState->setDrawBufferUpdate(gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  shading->joinStatesFront(ref_ptr<FramebufferClear>::alloc());
  shading->joinStatesFront(fboState);

  // no depth testing/writing
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::alloc();
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);
  shading->joinStatesFront(depthState);

  ref_ptr<StateNode> shadingNode = ref_ptr<StateNode>::alloc(shading);
  root->addChild(shadingNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(shadingNode.get());
  shading->createShader(shaderConfigurer.cfg());

  return shading;
}

static int lightCounter=0;

ref_ptr<Light> createPointLight(QtApplication *app,
    const Vec3f &pos,
    const Vec3f &diffuse,
    const Vec2f &radius)
{
  ref_ptr<Light> pointLight = ref_ptr<Light>::alloc(Light::POINT);
  pointLight->position()->setVertex(0,pos);
  pointLight->diffuse()->setVertex(0,diffuse);
  pointLight->radius()->setVertex(0,radius);

  app->addShaderInput(
      REGEN_STRING("Light.Light"<<lightCounter<<"[point]"),
      pointLight->position(),
      Vec4f(-100.0f), Vec4f(100.0f), Vec4i(2),
      "the world space light position.");
  app->addShaderInput(
      REGEN_STRING("Light.Light"<<lightCounter<<"[point]"),
      pointLight->radius(),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "inner and outer light radius.");
  app->addShaderInput(
      REGEN_STRING("Light.Light"<<lightCounter<<"[point]"),
      pointLight->diffuse(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "diffuse light color.");
  app->addShaderInput(
      REGEN_STRING("Light.Light"<<lightCounter<<"[point]"),
      pointLight->specular(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "specular light color.");
  ++lightCounter;

  return pointLight;
}

ref_ptr<Light> createSpotLight(QtApplication *app,
    const Vec3f &pos,
    const Vec3f &dir,
    const Vec3f &diffuse,
    const Vec2f &radius,
    const Vec2f &coneAngles)
{
  ref_ptr<Light> l = ref_ptr<Light>::alloc(Light::SPOT);
  l->position()->setVertex(0,pos);
  l->direction()->setVertex(0,dir);
  l->diffuse()->setVertex(0,diffuse);
  l->radius()->setVertex(0,radius);
  l->set_innerConeAngle(coneAngles.x);
  l->set_outerConeAngle(coneAngles.y);

  app->addShaderInput(
      REGEN_STRING("Light.Light"<<lightCounter<<"[spot]"),
      l->position(),
      Vec4f(-100.0f), Vec4f(100.0f), Vec4i(2),
      "the world space light position.");
  app->addShaderInput(
      REGEN_STRING("Light.Light"<<lightCounter<<"[spot]"),
      l->direction(),
      Vec4f(-1.0f), Vec4f(1.0f), Vec4i(2),
      "the light direction.");
  app->addShaderInput(
      REGEN_STRING("Light.Light"<<lightCounter<<"[spot]"),
      l->coneAngle(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "inner and outer cone angles.");
  app->addShaderInput(
      REGEN_STRING("Light.Light"<<lightCounter<<"[spot]"),
      l->radius(),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "inner and outer light radius.");
  app->addShaderInput(
      REGEN_STRING("Light.Light"<<lightCounter<<"[spot]"),
      l->diffuse(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "diffuse light color.");
  app->addShaderInput(
      REGEN_STRING("Light.Light"<<lightCounter<<"[spot]"),
      l->specular(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "specular light color.");
  ++lightCounter;

  return l;
}

ref_ptr<ShadowMap> createShadow(
    QtApplication *app,
    const ref_ptr<Light> &light,
    const ref_ptr<Camera> &cam,
    ShadowMap::Config cfg)
{
  ref_ptr<ShadowMap> sm = ref_ptr<ShadowMap>::alloc(light, cam, cfg);
  return sm;
}

/////////////////////////////////////
//// Mesh Factory
/////////////////////////////////////

static void __addMaterialInputs(
    QtApplication *app,
    Material *material,
    const string &prefix)
{
  app->addShaderInput(REGEN_STRING(prefix<<".Material"),
      material->ambient(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "Ambient material color.");
  app->addShaderInput(REGEN_STRING(prefix<<".Material"),
      material->diffuse(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "Diffuse material color.");
  app->addShaderInput(REGEN_STRING(prefix<<".Material"),
      material->specular(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "Specular material color.");
  app->addShaderInput(REGEN_STRING(prefix<<".Material"),
      material->shininess(),
      Vec4f(0.0f), Vec4f(128.0f), Vec4i(2),
      "The shininess exponent.");
  app->addShaderInput(REGEN_STRING(prefix<<".Material"),
      material->alpha(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "The material alpha.");
  app->addShaderInput(REGEN_STRING(prefix<<".Material"),
      material->refractionIndex(),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "Index of refraction of the material.");
}
GLuint sphereCounter=0;
GLuint coneCounter=0;
GLuint quadCounter=0;
GLuint modelCounter=0;

// Loads Meshes from File using Assimp. Optionally Bone animations are loaded.
list<MeshData> createAssimpMesh(
    QtApplication *app,
    const ref_ptr<StateNode> &root,
    const string &modelFile,
    const string &texturePath,
    const Mat4f &meshRotation,
    const Vec3f &meshTranslation,
    const Mat4f &meshScaling,
    const BoneAnimRange *animRanges,
    GLuint numAnimationRanges,
    GLdouble ticksPerSecond,
    const string &shaderKey)
{
  AssimpImporter importer(modelFile, texturePath);
  list< ref_ptr<Mesh> > meshes;
  ref_ptr<NodeAnimation> boneAnim;

  importer.loadMeshes(meshScaling,VBO::USAGE_DYNAMIC,meshes);

  if(animRanges && numAnimationRanges>0) {
    boneAnim = importer.loadNodeAnimation(
        GL_TRUE,
        NodeAnimation::BEHAVIOR_LINEAR,
        NodeAnimation::BEHAVIOR_LINEAR,
        ticksPerSecond);
    if(boneAnim.get()) boneAnim->stopAnimation();
  }
  ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::alloc();
  modelMat->set_modelMat(meshRotation, 0.0);
  modelMat->translate(meshTranslation, 0.0f);

  list<MeshData> ret;

  for(list< ref_ptr<Mesh> >::iterator
      it=meshes.begin(); it!=meshes.end(); ++it)
  {
    ref_ptr<Mesh> &mesh = *it;

    mesh->joinStates(modelMat);

    ref_ptr<Material> material = importer.getMeshMaterial(mesh.get());
    mesh->joinStates(material);
    __addMaterialInputs(app, material.get(),
        REGEN_STRING("Meshes.Model"<<(++modelCounter)));

    if(boneAnim.get()) {
      list< ref_ptr<AnimationNode> > meshBones =
          importer.loadMeshBones(mesh.get(), boneAnim.get());
      ref_ptr<Bones> bonesState = ref_ptr<Bones>::alloc(importer.numBoneWeights(mesh.get()));
      bonesState->setBones(meshBones);
      mesh->joinStates(bonesState);
    }

    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
    mesh->joinStates(shaderState);

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
    root->addChild(meshNode);

    StateConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
    shaderState->createShader(shaderConfigurer.cfg(), shaderKey);
    mesh->initializeResources(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());

    MeshData d;
    d.material_ = material;
    d.mesh_ = mesh;
    d.shader_ = shaderState;
    d.node_ = meshNode;
    ret.push_back(d);
  }

  if(boneAnim.get()) {
    ref_ptr<EventHandler> animStopped =
        ref_ptr<AnimationRangeUpdater>::alloc(boneAnim,animRanges,numAnimationRanges);
    boneAnim->connect(Animation::ANIMATION_STOPPED, animStopped);
    boneAnim->startAnimation();

    EventData evData;
    evData.eventID = Animation::ANIMATION_STOPPED;
    animStopped->call(boneAnim.get(), &evData);
  }

  return ret;
}

void createConeMesh(QtApplication *app, const ref_ptr<StateNode> &root)
{
  ConeClosed::Config cfg;
  cfg.levelOfDetail = 3;
  cfg.isBaseRequired = GL_TRUE;
  cfg.isNormalRequired = GL_TRUE;
  cfg.height = 3.0;
  cfg.radius = 1.0;
  ref_ptr<Mesh> mesh = ref_ptr<ConeClosed>::alloc(cfg);

  ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::alloc();
  modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);
  mesh->joinStates(modelMat);

  ref_ptr<Material> material = ref_ptr<Material>::alloc();
  material->ambient()->setUniformData(Vec3f(0.3f));
  material->diffuse()->setUniformData(Vec3f(0.7f));
  mesh->joinStates(material);
  __addMaterialInputs(app, material.get(),
      REGEN_STRING("Meshes.Cone"<<(++coneCounter)));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
  mesh->joinStates(shaderState);

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
  root->addChild(meshNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "regen.meshes.mesh");
  mesh->initializeResources(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());
}

// Creates simple floor mesh
MeshData createFloorMesh(
    QtApplication *app,
    const ref_ptr<StateNode> &root,
    const GLfloat &height,
    const Vec3f &posScale,
    const Vec2f &texcoScale,
    TextureState::TransferTexco transferMode,
    GLboolean useTess)
{
  Rectangle::Config meshCfg;
  meshCfg.levelOfDetail = 0;
  meshCfg.isTexcoRequired = GL_TRUE;
  meshCfg.isNormalRequired = GL_TRUE;
  meshCfg.isTangentRequired = GL_TRUE;
  meshCfg.centerAtOrigin = GL_TRUE;
  meshCfg.rotation = Vec3f(0.0f*M_PI, 0.0f*M_PI, 1.0f*M_PI);
  meshCfg.posScale = posScale;
  meshCfg.texcoScale = texcoScale;
  ref_ptr<Mesh> floor = ref_ptr<Rectangle>::alloc(meshCfg);

  ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::alloc();
  modelMat->translate(Vec3f(0.0f, height, 0.0f), 0.0f);
  modelMat->setConstantUniforms(GL_TRUE);
  floor->joinStates(modelMat);

  ref_ptr<Material> material = ref_ptr<Material>::alloc();
  material->ambient()->setUniformData(Vec3f(0.3f));
  material->diffuse()->setUniformData(Vec3f(0.7f));
  material->setConstantUniforms(GL_TRUE);
  floor->joinStates(material);
  __addMaterialInputs(app, material.get(), "Meshes.Floor");

  // setup texco transfer uniforms
  if(useTess) {
    ref_ptr<TesselationState> tess = ref_ptr<TesselationState>::alloc(3);
    tess->set_lodMetric(TesselationState::CAMERA_DISTANCE_INVERSE);
    tess->lodFactor()->setVertex(0,1.0f);
    floor->set_primitive(GL_PATCHES);
    floor->joinStates(tess);
    app->addShaderInput("Meshes.Floor.Bricks",
        tess->lodFactor(),
        Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
        "Tesselation has a range for its levels, maxLevel is currently 64.0.");
  }
  else if(transferMode==TextureState::TRANSFER_TEXCO_PARALLAX) {
    ref_ptr<ShaderInput1f> bias = ref_ptr<ShaderInput1f>::alloc("parallaxBias");
    bias->setUniformData(0.015f);
    material->joinShaderInput(bias);
    app->addShaderInput("Meshes.Floor.Bricks",
        bias,
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
        "Parallax-Mapping bias.");

    ref_ptr<ShaderInput1f> scale = ref_ptr<ShaderInput1f>::alloc("parallaxScale");
    scale->setUniformData(0.03f);
    material->joinShaderInput(scale);
    app->addShaderInput("Meshes.Floor.Bricks",
        scale,
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
        "Parallax-Mapping scale.");
  }
  else if(transferMode==TextureState::TRANSFER_TEXCO_PARALLAX_OCC) {
    ref_ptr<ShaderInput1f> scale = ref_ptr<ShaderInput1f>::alloc("parallaxScale");
    scale->setUniformData(0.03f);
    material->joinShaderInput(scale);
    app->addShaderInput("Meshes.Floor.Bricks",
        scale,
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
        "Parallax-Occlusion-Mapping scale.");

    ref_ptr<ShaderInput1i> steps = ref_ptr<ShaderInput1i>::alloc("parallaxSteps");
    steps->setUniformData(10);
    material->joinShaderInput(steps);
    app->addShaderInput("Meshes.Floor.Bricks",
        steps,
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(0),
        "Parallax-Occlusion-Mapping steps.");
  }
  else if(transferMode==TextureState::TRANSFER_TEXCO_RELIEF) {
    ref_ptr<ShaderInput1f> scale = ref_ptr<ShaderInput1f>::alloc("reliefScale");
    scale->setUniformData(0.03f);
    material->joinShaderInput(scale);
    app->addShaderInput("Meshes.Floor.Bricks",
        scale,
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
        "Relief-Mapping scale.");

    ref_ptr<ShaderInput1i> linearSteps = ref_ptr<ShaderInput1i>::alloc("reliefLinearSteps");
    linearSteps->setUniformData(10);
    material->joinShaderInput(linearSteps);
    app->addShaderInput("Meshes.Floor.Bricks",
        linearSteps,
        Vec4f(0.0f), Vec4f(100.0f), Vec4i(0),
        "Relief-Mapping linear steps.");

    ref_ptr<ShaderInput1i> binarySteps = ref_ptr<ShaderInput1i>::alloc("reliefBinarySteps");
    binarySteps->setUniformData(2);
    material->joinShaderInput(binarySteps);
    app->addShaderInput("Meshes.Floor.Bricks",
        binarySteps,
        Vec4f(0.0f), Vec4f(100.0f), Vec4i(0),
        "Relief-Mapping binary steps.");
  }
  //material->shaderDefine("DEPTH_CORRECT", "TRUE");

  ref_ptr<Texture> colMap_ = textures::load(filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/textures/brick/color.jpg"));
  ref_ptr<Texture> norMap_ = textures::load(filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/textures/brick/normal.jpg"));
  ref_ptr<Texture> heightMap_ = textures::load(filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/textures/brick/height.jpg"));

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(colMap_, "colorTexture");
  texState->set_mapTo(TextureState::MAP_TO_COLOR);
  texState->set_texcoTransfer(transferMode);
  texState->set_blendMode(BLEND_MODE_SRC);
  material->joinStates(texState);

  texState = ref_ptr<TextureState>::alloc(norMap_, "normalTexture");
  texState->set_mapTo(TextureState::MAP_TO_NORMAL);
  texState->set_texcoTransfer(transferMode);
  texState->set_texelTransferKey("regen.utility.textures.normalTBNTransfer");
  texState->set_blendMode(BLEND_MODE_SRC);
  material->joinStates(texState);

  texState = ref_ptr<TextureState>::alloc(heightMap_, "heightTexture");
  if(useTess) {
    texState->set_blendMode(BLEND_MODE_ADD);
    texState->set_mapTo(TextureState::MAP_TO_HEIGHT);
    texState->set_texelTransferFunction(
        "void brickHeight(inout vec4 t) { t.x = t.x*0.05 - 0.05; }",
        "brickHeight");
  }
  material->joinStates(texState);

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
  floor->joinStates(shaderState);

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(floor);
  root->addChild(meshNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "regen.meshes.mesh");
  floor->initializeResources(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());

  MeshData d;
  d.mesh_ = floor;
  d.shader_ = shaderState;
  d.node_ = meshNode;
  return d;
}

MeshData createBox(QtApplication *app, const ref_ptr<StateNode> &root)
{
    Box::Config cubeConfig;
    cubeConfig.texcoMode = Box::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 0.5f, 0.5f);
    ref_ptr<Mesh> mesh = ref_ptr<Box>::alloc(cubeConfig);

    ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::alloc();
    modelMat->translate(Vec3f(-2.0f, 0.75f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);
    mesh->joinStates(modelMat);

    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
    mesh->joinStates(shaderState);

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
    root->addChild(meshNode);

    StateConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
    shaderState->createShader(shaderConfigurer.cfg(), "regen.meshes.mesh");
    mesh->initializeResources(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());

    MeshData d;
    d.mesh_ = mesh;
    d.shader_ = shaderState;
    d.node_ = meshNode;
    return d;
}

ref_ptr<Mesh> createSphere(QtApplication *app, const ref_ptr<StateNode> &root)
{
    Sphere::Config sphereConfig;
    sphereConfig.texcoMode = Sphere::TEXCO_MODE_NONE;
    ref_ptr<Mesh> mesh = ref_ptr<Sphere>::alloc(sphereConfig);

    ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::alloc();
    modelMat->translate(Vec3f(0.0f, 0.5f, 0.0f), 0.0f);
    mesh->joinStates(modelMat);

    ref_ptr<Material> material = ref_ptr<Material>::alloc();
    material->set_ruby();
    mesh->joinStates(material);
    __addMaterialInputs(app, material.get(),
        REGEN_STRING("Meshes.Sphere"<<(++sphereCounter)));

    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
    mesh->joinStates(shaderState);

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
    root->addChild(meshNode);

    StateConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
    shaderState->createShader(shaderConfigurer.cfg(), "regen.meshes.mesh");
    mesh->initializeResources(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());

    return mesh;
}

ref_ptr<Mesh> createQuad(QtApplication *app, const ref_ptr<StateNode> &root)
{
  Rectangle::Config quadConfig;
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_TRUE;
  quadConfig.isTangentRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  quadConfig.rotation = Vec3f(0.0f*M_PI, 0.0f*M_PI, 1.0f*M_PI);
  quadConfig.posScale = Vec3f(10.0f);
  quadConfig.texcoScale = Vec2f(2.0f);
  ref_ptr<Mesh> mesh = ref_ptr<Rectangle>::alloc(quadConfig);

  ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::alloc();
  modelMat->translate(Vec3f(0.0f, -0.5f, 0.0f), 0.0f);
  mesh->joinStates(modelMat);

  ref_ptr<Material> material = ref_ptr<Material>::alloc();
  material->set_chrome();
  material->specular()->setUniformData(Vec3f(0.0f));
  mesh->joinStates(material);
  __addMaterialInputs(app, material.get(),
      REGEN_STRING("Meshes.Quad"<<(++quadCounter)));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
  mesh->joinStates(shaderState);

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
  root->addChild(meshNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "regen.meshes.mesh");
  mesh->initializeResources(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());

  return mesh;
}

ref_ptr<Mesh> createReflectionSphere(
    QtApplication *app,
    const ref_ptr<TextureCube> &reflectionMap,
    const ref_ptr<StateNode> &root)
{
  SphereSprite::Config cfg;
  GLfloat radi[] = { 1.0 };
  Vec3f pos[] = { Vec3f(0.0f) };
  cfg.radius = radi;
  cfg.position = pos;
  cfg.sphereCount = 1;
  ref_ptr<SphereSprite> mesh = ref_ptr<SphereSprite>::alloc(cfg);

  ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::alloc();
  modelMat->translate(Vec3f(0.0f), 0.0f);
  mesh->joinStatesFront(modelMat);

  ref_ptr<Material> material = ref_ptr<Material>::alloc();
  mesh->joinStatesFront(material);
  __addMaterialInputs(app, material.get(),
      REGEN_STRING("Meshes.ReflectionSphere"<<(++sphereCounter)));

  ref_ptr<TextureState> refractionTexture = ref_ptr<TextureState>::alloc(reflectionMap);
  refractionTexture->set_mapTo(TextureState::MAP_TO_COLOR);
  refractionTexture->set_blendMode(BLEND_MODE_SRC);
  refractionTexture->set_mapping(TextureState::MAPPING_REFRACTION);
  material->joinStates(refractionTexture);

  ref_ptr<TextureState> reflectionTexture = ref_ptr<TextureState>::alloc(reflectionMap);
  reflectionTexture->set_mapTo(TextureState::MAP_TO_COLOR);
  reflectionTexture->set_blendMode(BLEND_MODE_MIX);
  reflectionTexture->set_blendFactor(0.35f);
  reflectionTexture->set_mapping(TextureState::MAPPING_REFLECTION);
  material->joinStates(reflectionTexture);

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
  root->addChild(meshNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  mesh->createShader(shaderConfigurer.cfg());

  return mesh;
}

/////////////////////////////////////
//// GUI Factory
/////////////////////////////////////

class UpdateFPS : public Animation
{
public:
  UpdateFPS(const ref_ptr<TextureMappedText> &widget)
  : Animation(GL_TRUE,GL_FALSE),
    widget_(widget), frameCounter_(0), sumDtMiliseconds_(0.0f)
  {}

  void glAnimate(RenderState *rs, GLdouble dt) {
    frameCounter_ += 1;
    sumDtMiliseconds_ += dt;

    if (sumDtMiliseconds_ > 1000.0) {
      fps_ = (GLint) (frameCounter_*1000.0/sumDtMiliseconds_);
      sumDtMiliseconds_ = 0;
      frameCounter_ = 0;

      wstringstream ss;
      ss << fps_ << " FPS";
      widget_->set_value(ss.str());
    }
  }

private:
  ref_ptr<TextureMappedText> widget_;
  GLuint frameCounter_;
  GLint fps_;
  GLdouble sumDtMiliseconds_;
};

// Creates GUI widgets displaying the current FPS
Animation* createFPSWidget(QtApplication *app, const ref_ptr<StateNode> &root)
{
  ref_ptr<Font> font = Font::get(filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/fonts/obelix.ttf"), 16, 96);

  ref_ptr<TextureMappedText> widget = ref_ptr<TextureMappedText>::alloc(font, 16.0);
  widget->set_color(Vec4f(0.97f,0.86f,0.77f,0.95f));
  widget->set_value(L"0 FPS");

  ref_ptr<ModelTransformation> modelTransformation = ref_ptr<ModelTransformation>::alloc();
  modelTransformation->translate( Vec3f( 12.0, 8.0, 0.0 ), 0.0f );
  widget->joinStatesFront(modelTransformation);

  ref_ptr<StateNode> widgetNode = ref_ptr<StateNode>::alloc(widget);
  root->addChild(widgetNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(widgetNode.get());
  widget->createShader(shaderConfigurer.cfg());

  return new UpdateFPS(widget);
}

void createLogoWidget(QtApplication *app, const ref_ptr<StateNode> &root)
{
  ref_ptr<Texture> logoTex = textures::load(filesystemPath(
      REGEN_SOURCE_DIR, "regen/res/logo.png"));
  Vec2f size(logoTex->width()*0.1, logoTex->height()*0.1);

  Rectangle::Config cfg;
  cfg.levelOfDetail = 0;
  cfg.isTexcoRequired = GL_TRUE;
  cfg.isNormalRequired = GL_FALSE;
  cfg.isTangentRequired = GL_FALSE;
  cfg.centerAtOrigin = GL_FALSE;
  cfg.posScale = Vec3f(size.x, 1.0, size.y);
  cfg.rotation = Vec3f(0.5f*M_PI, 0.0f, 0.0f);
  cfg.texcoScale = Vec2f(-1.0,1.0);
  cfg.translation = Vec3f(0.0f,0.0f,0.0f);
  ref_ptr<Mesh> widget = ref_ptr<Rectangle>::alloc(cfg);

  ref_ptr<Material> material = ref_ptr<Material>::alloc();
  material->alpha()->setVertex(0,0.7f);
  widget->joinStates(material);

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(logoTex);
  texState->set_mapTo(TextureState::MAP_TO_COLOR);
  texState->set_blendMode(BLEND_MODE_SRC);
  material->joinStates(texState);

  ref_ptr<ModelTransformation> modelTransformation = ref_ptr<ModelTransformation>::alloc();
  modelTransformation->translate( Vec3f(8.0,-8.0,0.0 ), 0.0f );
  widget->joinStates(modelTransformation);

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
  widget->joinStates(shaderState);

  ref_ptr<StateNode> widgetNode = ref_ptr<StateNode>::alloc(widget);
  root->addChild(widgetNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.define("INVERT_Y", "TRUE");
  shaderConfigurer.addNode(widgetNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "regen.gui.widget");
  widget->initializeResources(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());
}

void createTextureWidget(
    QtApplication *app,
    const ref_ptr<StateNode> &root,
    const ref_ptr<Texture> &tex,
    const Vec2ui &pos,
    const GLfloat &size)
{
  Rectangle::Config cfg;
  cfg.levelOfDetail = 0;
  cfg.isTexcoRequired = GL_TRUE;
  cfg.isNormalRequired = GL_FALSE;
  cfg.isTangentRequired = GL_FALSE;
  cfg.centerAtOrigin = GL_FALSE;
  cfg.posScale = Vec3f(size);
  cfg.rotation = Vec3f(0.5f*M_PI, 0.0f, 0.0f);
  cfg.texcoScale = Vec2f(1.0);
  cfg.translation = Vec3f(0.0f,-size,0.0f);
  ref_ptr<Mesh> widget = ref_ptr<Rectangle>::alloc(cfg);

  ref_ptr<Material> material = ref_ptr<Material>::alloc();
  widget->joinStates(material);

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(tex);
  texState->set_mapTo(TextureState::MAP_TO_COLOR);
  texState->set_blendMode(BLEND_MODE_SRC);
  texState->set_texelTransferFunction(
      "void transferIgnoreAlpha(inout vec4 v) { v.a=1.0; }", "transferIgnoreAlpha");
  material->joinStates(texState);

  ref_ptr<ModelTransformation> modelTransformation = ref_ptr<ModelTransformation>::alloc();
  modelTransformation->translate( Vec3f( pos.x, pos.y, 0.0 ), 0.0f );
  widget->joinStates(modelTransformation);

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
  widget->joinStates(shaderState);

  ref_ptr<StateNode> widgetNode = ref_ptr<StateNode>::alloc(widget);
  root->addChild(widgetNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(widgetNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "regen.gui.widget");
  widget->initializeResources(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());
}

// Creates root node for states rendering the HUD
ref_ptr<StateNode> createHUD(QtApplication *app,
    const ref_ptr<FBO> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment)
{
  ref_ptr<StateNode> guiRoot = ref_ptr<StateNode>::alloc();
  ref_ptr<State> guiState = guiRoot->state();

  // enable fbo and call DrawBuffer()
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);
  fboState->setDrawBufferOntop(tex,baseAttachment);
  guiState->joinStates(fboState);
  // alpha blend GUI widgets with scene
  guiState->joinStates(ref_ptr<BlendState>::alloc(BLEND_MODE_ALPHA));
  // no depth testing for gui
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::alloc();
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);
  guiState->joinStates(depthState);

  return guiRoot;
}

// Creates root node for states rendering the HUD
ref_ptr<StateNode> createHUD(QtApplication *app,
    const ref_ptr<FBO> &fbo,
    GLenum baseAttachment)
{
  ref_ptr<StateNode> guiRoot = ref_ptr<StateNode>::alloc();

  // enable fbo and call DrawBuffer()
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);
  fboState->addDrawBuffer(baseAttachment);
  guiRoot->state()->joinStates(fboState);
  // alpha blend GUI widgets with scene
  guiRoot->state()->joinStates(ref_ptr<BlendState>::alloc(BLEND_MODE_ALPHA));
  // no depth testing for gui
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::alloc();
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);
  guiRoot->state()->joinStates(depthState);

  return guiRoot;
}

}
