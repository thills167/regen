
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Render To Texture Test");

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  application->setLight();

  ref_ptr<ModelTransformationState> modelMat;

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

    application->addMesh(
        ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig)),
        modelMat,
        ref_ptr<Material>::manage(new Material));
  }

  FXAA::Config aaCfg;
  aaCfg.spanMax = 8.0;
  aaCfg.reduceMin = 1.0/128.0;
  aaCfg.reduceMul = 1.0/8.0;
  aaCfg.edgeThreshold = 1.0/8.0;
  aaCfg.edgeThresholdMin = 1.0/16.0;
  application->addAntiAliasingPass(aaCfg);

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  application->addSkyBox("res/textures/cube-violentdays.jpg");
  application->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  application->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  application->mainLoop();
  return 0;
}
