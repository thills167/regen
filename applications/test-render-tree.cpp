/*
 * glut-render-tree.cpp
 *
 *  Created on: 10.08.2012
 *      Author: daniel
 */

#ifndef GLUT_RENDER_TREE_CPP_
#define GLUT_RENDER_TREE_CPP_

#include "test-render-tree.h"

#include <ogle/states/blit-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/render-tree/shading-forward.h>
#include <ogle/render-tree/shading-deferred.h>
#include <ogle/render-tree/picker.h>
#include <ogle/states/shader-state.h>
#include <ogle/font/font-manager.h>
#include <ogle/models/quad.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>
#include <ogle/textures/cube-image-texture.h>

#include "ogle-application.h"

static GLdouble randomNorm() {
  return (GLdouble)rand()/(GLdouble)RAND_MAX;
}

/**
 * State that sorts node children when enabled.
 */
class SortNodeChildrenState : public State
{
public:
  SortNodeChildrenState(
      ref_ptr<StateNode> &alphaNode,
      ref_ptr<PerspectiveCamera> &camera,
      GLboolean frontToBack)
  : State(), alphaNode_(alphaNode), comparator_(camera,frontToBack)
  {
  }
  virtual void enable(RenderState *state) {
    alphaNode_->childs().sort(comparator_);
  }
  ref_ptr<StateNode> alphaNode_;
  NodeEyeDepthComparator comparator_;
};


class PlaceholderMesh : public State
{
public:
  PlaceholderMesh(ref_ptr<MeshState> &mesh)
  : State()
  {
    joinStates(ref_ptr<State>::cast(mesh));
  }
  virtual void enable(RenderState *state) { }
  virtual void disable(RenderState *state) { }
  virtual void configureShader(ShaderConfig *shaderCfg) { }
};

class UpdateFPS : public Animation
{
public:
  UpdateFPS(ref_ptr<Text> fpsText)
  : Animation(),
    fpsText_(fpsText),
    numFrames_(0),
    sumDtMiliseconds_(0.0f)
  {

  }
  virtual void animate(GLdouble milliSeconds)
  {
  }
  virtual void updateGraphics(GLdouble dt)
  {
    numFrames_ += 1;
    sumDtMiliseconds_ += dt;

    if (sumDtMiliseconds_ > 1000.0) {
      fps_ = (int) (numFrames_*1000.0/sumDtMiliseconds_);
      sumDtMiliseconds_ = 0;
      numFrames_ = 0;

      wstringstream ss;
      ss << fps_ << " FPS";
      fpsText_->set_value(ss.str());
    }
  }
private:
  ref_ptr<Text> fpsText_;
  unsigned int numFrames_;
  int fps_;
  double sumDtMiliseconds_;
};
class ResizeFramebufferEvent : public EventCallable
{
public:
  ResizeFramebufferEvent(
      GLfloat windowWidthScale,
      GLfloat windowHeightScale)
  : EventCallable(),
    widthScale_(windowWidthScale),
    heightScale_(windowHeightScale)
  {
  }
  virtual void call(EventObject *evObject, void*)
  {
    TestRenderTree *tree = (TestRenderTree*)evObject;
    if(fboState_.get()!=NULL) {
      fboState_->resize(
          tree->windowWidth()*widthScale_,
          tree->windowHeight()*heightScale_);
    }
    if(shading_.get()!=NULL) {
      shading_->resize(
          tree->windowWidth()*widthScale_,
          tree->windowHeight()*heightScale_);
    }
  }
  ref_ptr<FBOState> fboState_;
  ref_ptr<ShadingInterface> shading_;
  GLfloat widthScale_;
  GLfloat heightScale_;
};

GLuint TestRenderTree::RESIZE_EVENT =
    EventObject::registerEvent("testRenderTreeResize");

TestRenderTree::TestRenderTree(
    GLuint width, GLuint height)
: OGLERenderTree(),
  fov_(45.0f),
  near_(0.1f),
  far_(200.0f)
{
  timeDelta_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("deltaT"));
  timeDelta_->setUniformData(0.0f);
  mousePosition_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("mousePosition"));
  mousePosition_->setUniformData(Vec2f(0.0f));
  viewport_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("viewport"));
  viewport_->setUniformData(Vec2f(width,height));

  perspectiveCamera_ = ref_ptr<PerspectiveCamera>::manage(new PerspectiveCamera);
  guiCamera_ = ref_ptr<OrthoCamera>::manage(new OrthoCamera);
  updateProjection();

  globalStates_ = rootNode();

  perspectivePass_ = ref_ptr<StateNode>::manage(new StateNode);
  backgroundPass_ = ref_ptr<StateNode>::manage(new StateNode);
  transparencyPass_ = ref_ptr<StateNode>::manage(new StateNode);
  lightNode_ = ref_ptr<StateNode>::manage(new StateNode);
  lightNode_->state()->joinStates(ref_ptr<State>::cast(perspectiveCamera_));

  defaultLight_ = ref_ptr<DirectionalLight>::manage(new DirectionalLight);
}

void TestRenderTree::setRenderState(ref_ptr<RenderState> &renderState)
{
  renderState_ = renderState;
}

void TestRenderTree::setMousePosition(GLuint x, GLuint y)
{
  mousePosition_->setUniformData(Vec2f(x,y));
}

void TestRenderTree::initTree()
{
  if(renderState_.get()==NULL) {
    renderState_ = ref_ptr<RenderState>::manage(new RenderState);
  }

  UnitQuad::Config quadCfg;
  quadCfg.isNormalRequired = GL_FALSE;
  quadCfg.isTangentRequired = GL_FALSE;
  quadCfg.isTexcoRequired = GL_FALSE;
  quadCfg.levelOfDetail = 0;
  quadCfg.rotation = Vec3f(0.5*M_PI, 0.0f, 0.0f);
  quadCfg.posScale = Vec3f(2.0f);
  quadCfg.translation = Vec3f(-1.0f,-1.0f,0.0f);
  orthoQuad_ = ref_ptr<MeshState>::manage(new UnitQuad(quadCfg));

  globalStates_->state()->joinShaderInput(ref_ptr<ShaderInput>::cast(viewport_));
  globalStates_->state()->joinShaderInput(ref_ptr<ShaderInput>::cast(timeDelta_));
  globalStates_->state()->joinShaderInput(ref_ptr<ShaderInput>::cast(mousePosition_));
  globalStates_->state()->joinStates(
      ref_ptr<State>::manage(new PlaceholderMesh(orthoQuad_)));

  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);

  guiPass_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(guiCamera_)));
  guiPass_->state()->joinStates(ref_ptr<State>::manage(new BlendState));
  guiPass_->state()->joinStates(ref_ptr<State>::cast(depthState));

  globalStates_->addChild(lightNode_);
  lightNode_->addChild(perspectivePass_);
  lightNode_->addChild(backgroundPass_);
  lightNode_->addChild(transparencyPass_);
}

ref_ptr<DirectionalLight>& TestRenderTree::defaultLight()
{
  return defaultLight_;
}
ref_ptr<StateNode>& TestRenderTree::globalStates()
{
  return globalStates_;
}
ref_ptr<StateNode>& TestRenderTree::perspectivePass()
{
  return perspectivePass_;
}
ref_ptr<StateNode>& TestRenderTree::transparencyPass()
{
  return transparencyPass_;
}
ref_ptr<StateNode>& TestRenderTree::guiPass()
{
  return guiPass_;
}
ref_ptr<PerspectiveCamera>& TestRenderTree::perspectiveCamera()
{
  return perspectiveCamera_;
}
ref_ptr<OrthoCamera>& TestRenderTree::guiCamera()
{
  return guiCamera_;
}
ref_ptr<RenderState>& TestRenderTree::renderState()
{
  return renderState_;
}
ref_ptr<MeshState>& TestRenderTree::orthoQuad()
{
  return orthoQuad_;
}

ref_ptr<Picker> TestRenderTree::usePicking()
{
  ref_ptr<Picker> picker = ref_ptr<Picker>::manage(new Picker(perspectivePass_));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(picker));
  return picker;
}
void TestRenderTree::useGUIPass()
{
  globalStates_->addChild(guiPass_);
}
void TestRenderTree::setBlitToScreen(
    ref_ptr<FrameBufferObject> fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, viewport_, attachment));
  ref_ptr<StateNode> blitNode = ref_ptr<StateNode>::manage(
      new StateNode(blitState));
  globalStates_->addChild(blitNode);
}
void TestRenderTree::setClearScreenColor(const Vec4f &clearColor)
{
  ref_ptr<ClearColorState> clearScreenColor =
      ref_ptr<ClearColorState>::manage(new ClearColorState);
  ClearColorData clearData;
  clearData.clearColor = clearColor;
  clearData.colorBuffers.push_back( GL_FRONT );
  clearScreenColor->data.push_back(clearData);
  globalStates_->state()->joinStates(ref_ptr<State>::cast(clearScreenColor));
}
void TestRenderTree::setClearScreenDepth()
{
  ref_ptr<State> clearScreenDepth =
      ref_ptr<State>::manage(new ClearDepthState);
  globalStates_->state()->joinStates(clearScreenDepth);
}

ref_ptr<DirectionalLight>& TestRenderTree::setLight()
{
  setLight(ref_ptr<Light>::cast(defaultLight_));
  return defaultLight_;
}
void TestRenderTree::setLight(ref_ptr<Light> light)
{
  lightNode_->state()->joinStates(ref_ptr<State>::cast(light));
}

ref_ptr<FBOState> TestRenderTree::createRenderTarget(
    GLfloat windowWidthScale,
    GLfloat windowHeightScale,
    GLenum depthAttachmentFormat,
    GLboolean clearDepthBuffer,
    GLboolean clearColorBuffer,
    const Vec4f &clearColor)
{
  Vec2f &viewport = viewport_->getVertex2f(0);
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(
          windowWidthScale*viewport.x,
          windowHeightScale*viewport.y,
          depthAttachmentFormat));

  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  GLenum colorAttachment = GL_COLOR_ATTACHMENT0;
  if(clearDepthBuffer) {
    fboState->setClearDepth();
  }
  if(clearColorBuffer) {
    ClearColorData colorData;
    colorData.clearColor = clearColor;
    colorData.colorBuffers.push_back( colorAttachment );
    fboState->setClearColor(colorData);
  }

  // call glDrawBuffer for GL_COLOR_ATTACHMENT0
  fboState->addDrawBuffer(colorAttachment);

  ref_ptr<ResizeFramebufferEvent> resizeFramebuffer = ref_ptr<ResizeFramebufferEvent>::manage(
      new ResizeFramebufferEvent(windowWidthScale, windowHeightScale));
  resizeFramebuffer->fboState_ = fboState;
  connect(RESIZE_EVENT, ref_ptr<EventCallable>::cast(resizeFramebuffer));

  return fboState;
}

void TestRenderTree::setTransparencyMode(
    TransparencyMode transparencyMode)
{
  const GLboolean useDoublePrecision = GL_FALSE;
  if(transparencyState_.get()) {
    transparencyPass_->state()->disjoinStates(
        ref_ptr<State>::cast(transparencyState_));
  }
  transparencyState_ = ref_ptr<TransparencyState>::manage(
      new TransparencyState(transparencyMode,
          sceneTexture_->width(),
          sceneTexture_->height(),
          sceneDepthTexture_,
          useDoublePrecision));
  transparencyPass_->state()->joinStates(
      ref_ptr<State>::cast(transparencyState_));
  // order dependent transparency modes require sorting the meshes by
  // model view matrix.
  if(transparencyMode == TRANSPARENCY_MODE_BACK_TO_FRONT) {
    transparencyPass_->state()->joinStates(ref_ptr<State>::manage(
        new SortNodeChildrenState(transparencyPass_, perspectiveCamera_, GL_FALSE)));
  }
  else if(transparencyMode == TRANSPARENCY_MODE_FRONT_TO_BACK) {
    transparencyPass_->state()->joinStates(ref_ptr<State>::manage(
        new SortNodeChildrenState(transparencyPass_, perspectiveCamera_, GL_TRUE)));
  }
  // resize with scene
  ref_ptr<ResizeFramebufferEvent> resizeFramebuffer = ref_ptr<ResizeFramebufferEvent>::manage(
      new ResizeFramebufferEvent(1.0, 1.0));
  resizeFramebuffer->fboState_ = transparencyState_->fboState();
  connect(RESIZE_EVENT, ref_ptr<EventCallable>::cast(resizeFramebuffer));

  ref_ptr<AccumulateTransparency> accum = ref_ptr<AccumulateTransparency>::manage(
      new AccumulateTransparency(transparencyMode, orthoQuad_, sceneFBO_, sceneTexture_)) ;
  accum->setTransparencyTextures(
      transparencyState_->colorTexture(),
      transparencyState_->counterTexture()
      );
  if(transparencyAccumulation_.get()) {
    lightNode_->removeChild(transparencyAccumulation_);
  }
  transparencyAccumulation_ = ref_ptr<StateNode>::cast(accum);
  lightNode_->addChild(transparencyAccumulation_);
}

ref_ptr<FBOState> TestRenderTree::setRenderToTexture(
    GLfloat windowWidthScale,
    GLfloat windowHeightScale,
    GLenum colorAttachmentFormat,
    GLenum depthAttachmentFormat,
    GLboolean clearDepthBuffer,
    GLboolean clearColorBuffer,
    const Vec4f &clearColor)
{
  Vec2f &viewport = viewport_->getVertex2f(0);
#if 0
  ref_ptr<ShadingInterface> shading = ref_ptr<ShadingInterface>::manage(new ForwardShading(
      windowWidthScale*viewport.x,
      windowHeightScale*viewport.y,
      colorAttachmentFormat,
      depthAttachmentFormat));
#else
  list<GBufferTarget> outputTargets;
  outputTargets.push_back(GBufferTarget("color", GL_RGBA, colorAttachmentFormat));
  outputTargets.push_back(GBufferTarget("specular", GL_RGBA, GL_RGBA));
  outputTargets.push_back(GBufferTarget("norWorld", GL_RGBA, GL_RGBA));
  outputTargets.push_back(GBufferTarget("posWorld", GL_RGB, GL_RGB16F));
  ref_ptr<ShadingInterface> shading = ref_ptr<ShadingInterface>::manage(new DeferredShading(
      windowWidthScale*viewport.x,
      windowHeightScale*viewport.y,
      depthAttachmentFormat,
      outputTargets));
#endif
  sceneFBO_ = shading->framebuffer()->fbo();
  sceneTexture_ = shading->colorTexture();
  sceneDepthTexture_ = shading->depthTexture();
  if(clearDepthBuffer) {
    shading->framebuffer()->setClearDepth();
  }
  if(clearColorBuffer) {
    ClearColorData clearData;
    clearData.clearColor = clearColor;
#if 0
    clearData.colorBuffers.push_back(GL_COLOR_ATTACHMENT0);
#else
    clearData.colorBuffers.push_back(GL_COLOR_ATTACHMENT1);
#endif
    shading->framebuffer()->setClearColor(clearData);
  }
  shading->state()->joinStates(ref_ptr<State>::cast(perspectiveCamera_));

  lightNode_->removeChild(perspectivePass_);
  perspectivePass_ = shading->geometryStage();
  ref_ptr<StateNode> shadingPass_ = ref_ptr<StateNode>::cast(shading);
  lightNode_->addFirstChild(shadingPass_);

  ref_ptr<ResizeFramebufferEvent> resizeFramebuffer = ref_ptr<ResizeFramebufferEvent>::manage(
      new ResizeFramebufferEvent(windowWidthScale, windowHeightScale)
      );
  resizeFramebuffer->shading_ = shading;
  connect(RESIZE_EVENT, ref_ptr<EventCallable>::cast(resizeFramebuffer));

  return shading->framebuffer();
}
ref_ptr<FBOState> TestRenderTree::setRenderToTexture(
    ref_ptr<FrameBufferObject> fbo,
    GLboolean clearDepthBuffer,
    GLboolean clearColorBuffer,
    const Vec4f &clearColor)
{
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  GLenum colorAttachment = GL_COLOR_ATTACHMENT0;
  if(clearDepthBuffer) {
    fboState->setClearDepth();
  }
  if(clearColorBuffer) {
    ClearColorData colorData;
    colorData.clearColor = clearColor;
    colorData.colorBuffers.push_back( colorAttachment );
    fboState->setClearColor(colorData);
  }

  // call glDrawBuffer for GL_COLOR_ATTACHMENT0
  fboState->addDrawBuffer(colorAttachment);

  globalStates_->state()->joinStates(ref_ptr<State>::cast(fboState));
  sceneFBO_ = fboState->fbo();

  return fboState;
}

ref_ptr<StateNode> TestRenderTree::addMesh(
    ref_ptr<MeshState> mesh,
    ref_ptr<ModelTransformationState> modelTransformation,
    ref_ptr<Material> material,
    const string &shaderKey,
    GLboolean useAlpha)
{
  return addMesh(
      (useAlpha ? transparencyPass_ : perspectivePass_),
      mesh,
      modelTransformation,
      material,
      shaderKey);
}

ref_ptr<StateNode> TestRenderTree::addGUIElement(
    ref_ptr<MeshState> mesh,
    ref_ptr<ModelTransformationState> modelTransformation,
    ref_ptr<Material> material,
    const string &shaderKey)
{
  if(guiPass_->parent() == NULL) {
    useGUIPass();
  }
  return addMesh(
      guiPass_,
      mesh,
      modelTransformation,
      material,
      shaderKey);
}

void TestRenderTree::removeMesh(ref_ptr<StateNode> &node)
{
  // XXX: not so nice---
  StateNode *n = node.get();
  while(n->parent()!=NULL) {
    StateNode *p = n->parent();
    if(p == perspectivePass_.get()) {
      p->removeChild(n);
      return;
    }
    if(p == guiPass_.get()) {
      p->removeChild(n);
      return;
    }
    if(p == transparencyPass_.get()) {
      p->removeChild(n);
      return;
    }
    n = p;
  }
}

ref_ptr<StateNode> TestRenderTree::addMesh(
    ref_ptr<StateNode> parent,
    ref_ptr<MeshState> mesh,
    ref_ptr<ModelTransformationState> modelTransformation,
    ref_ptr<Material> material,
    const string &shaderKey)
{
  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  ref_ptr<ShaderState> shaderState =
      ref_ptr<ShaderState>::manage(new ShaderState);
  ref_ptr<StateNode> materialNode, shaderNode, modeltransformNode;

  ref_ptr<StateNode> *root = &meshNode;
  if(!shaderKey.empty()) {
    shaderNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(shaderState)));
    shaderNode->addChild(*root);
    root = &shaderNode;
  }
  if(modelTransformation.get()!=NULL) {
    modeltransformNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(modelTransformation)));
    modeltransformNode->addChild(*root);
    root = &modeltransformNode;
  }
  if(material.get()!=NULL) {
    materialNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(material)));
    materialNode->addChild(*root);
    root = &materialNode;
  }

  parent->addChild(*root);

  if(!shaderKey.empty()) {
    ShaderConfig shaderCfg;
    meshNode->configureShader(&shaderCfg);
    shaderState->createShader(shaderCfg, shaderKey);
  }

  return (shaderNode.get()!=NULL ? shaderNode : meshNode);
}

ref_ptr<StateNode> TestRenderTree::addSkyBox(
    const string &imagePath,
    GLenum internalFormat,
    GLboolean flipBackFace)
{
  ref_ptr<TextureCube> cubeMap = ref_ptr<TextureCube>::manage(
      new CubeImageTexture(imagePath, internalFormat, flipBackFace));
  cubeMap->set_wrapping(GL_CLAMP_TO_EDGE);
  skyBox_ = ref_ptr<SkyBox>::manage(new SkyBox(far_));
  skyBox_->setCubeMap(cubeMap);

  return addMesh(ref_ptr<MeshState>::cast(skyBox_));
}
ref_ptr<StateNode> TestRenderTree::addSkyBox(ref_ptr<TextureCube> &cubeMap)
{
  cubeMap->set_wrapping(GL_CLAMP_TO_EDGE);
  skyBox_ = ref_ptr<SkyBox>::manage(new SkyBox(far_));
  skyBox_->setCubeMap(cubeMap);

  return addMesh(ref_ptr<MeshState>::cast(skyBox_));
}
ref_ptr<StateNode> TestRenderTree::addDynamicSky()
{
  ref_ptr<DynamicSky> skyAtmosphere = ref_ptr<DynamicSky>::manage(
      new DynamicSky(orthoQuad_, far_));
  skyBox_ = ref_ptr<SkyBox>::cast(skyAtmosphere);

  ref_ptr<TextureCube> milkyway = ref_ptr<TextureCube>::manage(
      new CubeImageTexture("res/textures/cube-milkyway.png", GL_RGB, GL_FALSE));
  milkyway->set_wrapping(GL_CLAMP_TO_EDGE);
  skyAtmosphere->setStarMap(milkyway);
  skyAtmosphere->setStarMapBrightness(0.1f);

  srand(time(0));
  GLfloat lat = 360.0 * randomNorm();
  GLfloat lon = 360.0 * randomNorm();
  skyAtmosphere->setEarth(lon, lat);

  ref_ptr<ModelTransformationState> modelTransformation;
  ref_ptr<Material> material;
  ref_ptr<StateNode> skyNode = addMesh(
      backgroundPass_, ref_ptr<MeshState>::cast(skyBox_),
      modelTransformation, material,
      "mesh");

  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(skyAtmosphere));
  setLight(ref_ptr<Light>::cast(skyAtmosphere->sun()));

  return skyNode;
}

void TestRenderTree::setShowFPS()
{
  const Vec4f fpsColor = Vec4f(1.0f);
  const Vec4f fpsBackgroundColor = Vec4f(0.0f, 0.0f, 0.0f, 0.5f);
  FreeTypeFont& font = FontManager::get().getFont(
                  "res/fonts/arial.ttf",
                  12, // font size in pixel
                  fpsColor, //
                  fpsBackgroundColor,
                  0, // glyph rotation in degree
                  GL_LINEAR, // filter mode
                  false, // use mipmaps?
                  96); // dpi

  fpsText_ = ref_ptr<Text>::manage(new Text(font, 12.0, true, true));
  fpsText_->set_value(L"0 FPS");

  ref_ptr<ModelTransformationState> modelTransformation =
      ref_ptr<ModelTransformationState>::manage(new ModelTransformationState);
  modelTransformation->translate( Vec3f( 4.0, 4.0, 0.0 ), 0.0f );
  addGUIElement(ref_ptr<MeshState>::cast(fpsText_), modelTransformation);

  updateFPS_ = ref_ptr<Animation>::manage(new UpdateFPS(fpsText_));
  AnimationManager::get().addAnimation(updateFPS_);
}

void TestRenderTree::set_nearDistance(GLfloat near)
{
  near_ = near;
  updateProjection();
}
void TestRenderTree::set_farDistance(GLfloat far)
{
  far_ = far;
  updateProjection();
  if(skyBox_.get()!=NULL) {
    skyBox_->resize(far);
  }
}
void TestRenderTree::set_fieldOfView(GLfloat fov)
{
  fov_ = fov;
  updateProjection();
}

void TestRenderTree::updateProjection()
{
  Vec2f &viewport = viewport_->getVertex2f(0);
  guiCamera_->updateProjection(viewport.x, viewport.y);
  GLfloat aspect = viewport.x/viewport.y;
  perspectiveCamera_->updateProjection(fov_, near_, far_, aspect);
}

void TestRenderTree::setWindowSize(GLuint w, GLuint h)
{
  viewport_->getVertex2f(0) = Vec2f(w,h);
  updateProjection();
  emitEvent(RESIZE_EVENT);
}
void TestRenderTree::render(GLdouble dt)
{
  timeDelta_->setUniformData(dt);
  RenderTree::traverse(renderState_.get(), dt);
}
void TestRenderTree::postRender(GLdouble dt)
{
  // some animations modify the vertex data,
  // updating the vbo needs a context so we do it here in the main thread..
  AnimationManager::get().updateGraphics(dt);
  // invoke event handler of queued events
  EventObject::emitQueued();
}

#endif /* GLUT_RENDER_TREE_CPP_ */
