
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/textures/video-texture.h>
#include <ogle/animations/animation-manager.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "libav Video Texture + OpenAL sound");

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      800,600,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  ref_ptr<Light> &light = application->setLight();
  light->setConstantUniforms(GL_TRUE);

  application->perspectiveCamera()->set_isAudioListener(true);
  application->camManipulator()->setStepLength(0.0f,0.0f);
  application->camManipulator()->set_degree(0.0f,0.0f);
  application->camManipulator()->set_height(0.0f,0.0f);
  application->camManipulator()->set_radius(5.0f, 0.0f);

  ref_ptr<ModelTransformationState> modelMat;

  ref_ptr<VideoTexture> v = ref_ptr<VideoTexture>::manage(new VideoTexture);
  v->set_file("res/textures/video.avi");
  v->set_repeat( true );
  v->addMapTo(MAP_TO_DIFFUSE);
  ref_ptr<AudioSource> audio = v->audioSource();

  {
    UnitQuad::Config quadConfig;
    quadConfig.levelOfDetail = 0;
    quadConfig.isTexcoRequired = GL_TRUE;
    quadConfig.isNormalRequired = GL_TRUE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(0.5*M_PI, 0.0*M_PI, 1.0*M_PI);
    quadConfig.posScale = Vec3f(2.0f, 2.0f, 2.0f);
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.5f, 0.0f), 0.0f);
    modelMat->set_audioSource( audio );
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_shading( Material::PHONG_SHADING );
    material->set_twoSided(true);
    material->addTexture(ref_ptr<Texture>::cast(v));
    material->setConstantUniforms(GL_TRUE);

    //quad->set_isSprite(true);
    application->addMesh(quad, modelMat, material);
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  application->addSkyBox("res/textures/cube-clouds");
  application->setShowFPS();

  // TODO: screen blit must know screen width/height
  application->setBlitToScreen(
      fboState->fbo(), GL_COLOR_ATTACHMENT0);

  v->play();

  application->mainLoop();
  return 0;
}
