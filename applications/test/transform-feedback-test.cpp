
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/states/debug-normal.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Transform Feedback");

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

  ref_ptr<ModelTransformationState> modelMat;

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.5f, 0.0f, 0.0f), 0.0f);
    ref_ptr<MeshState> sphereState =
        ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));

    ref_ptr<VertexAttribute> posAtt_ = ref_ptr<VertexAttribute>::manage(
        new VertexAttributefv( "Position", 4 ));
    sphereState->setTransformFeedbackAttribute(posAtt_);

    ref_ptr<VertexAttribute> norAtt_ = ref_ptr<VertexAttribute>::manage(
        new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
    sphereState->setTransformFeedbackAttribute(norAtt_);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_shading(Material::PHONG_SHADING);
    material->set_chrome();

    ref_ptr<StateNode> meshNode = application->addMesh(sphereState, modelMat, material);

    ref_ptr<TFMeshState> tfState =
        ref_ptr<TFMeshState>::manage(new TFMeshState(sphereState));
    tfState->joinStates(ref_ptr<State>::manage(
        new DebugNormal(GS_INPUT_TRIANGLES, 0.1)));
    ref_ptr<StateNode> tfNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(tfState)));
    application->renderTree()->addChild(application->perspectivePass(), tfNode);
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
