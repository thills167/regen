
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/textures/video-texture.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/animations/animation-manager.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("Volume Renderer");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.7f, 0.6f, 0.5f, 0.0f)
  );

  ref_ptr<Light> &light = renderTree->setLight();
  light->setConstantUniforms(GL_TRUE);

  ref_ptr<Material> material;
  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.isNormalRequired = GL_TRUE;
    cubeConfig.posScale = Vec3f(2.0f, 2.0f, 2.0f);

    material = ref_ptr<Material>::manage(new Material);
    material->set_shading( Material::NO_SHADING );
    material->set_useAlpha(GL_TRUE);

    ref_ptr<RAWTexture3D> tex = ref_ptr<RAWTexture3D>::manage(new RAWTexture3D());
    RAWTextureFile rawFile;
    rawFile.path = "res/textures/teapot.raw";
    rawFile.bytesPerComponent = 8;
    rawFile.numComponents = 1;
    rawFile.width = 256;
    rawFile.height = 256;
    rawFile.depth = 256;
    tex->loadRAWFile(rawFile);

    ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(
        new TextureState(ref_ptr<Texture>::cast(tex)));
    texState->set_name("volumeTexture");
    material->joinStates(ref_ptr<State>::cast(texState));

    material->setConstantUniforms(GL_TRUE);

    ref_ptr<StateNode> shaderNode = renderTree->addMesh(
        ref_ptr<MeshState>::manage(new UnitCube(cubeConfig)),
        ref_ptr<ModelTransformationState>(),
        material,
        "volume");

    // find shader parent
    ShaderState *shaderState = (ShaderState*) shaderNode->state().get();
    Shader *shader = shaderState->shader().get();
    shader->setTexture(texState->texture(), texState->name());

    ref_ptr<State> alphaBlending =
        ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA));
    shaderNode->state()->joinStates(alphaBlending);
  }

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
