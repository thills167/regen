/*
 * parabolid-camera.h
 *
 *  Created on: Dec 16, 2013
 *      Author: daniel
 */

#ifndef PARABOLID_CAMERA_H_
#define PARABOLID_CAMERA_H_

#include <regen/camera/camera.h>
#include <regen/meshes/mesh-state.h>
#include <regen/gl-types/shader-input-container.h>

namespace regen {
  /**
   * A camera with a parabolid projection
   * computed in shaders.
   */
  class ParabolidCamera : public Camera
  {
  public:
    /**
     * @param mesh Defines the parabolid center.
     * @param userCamera The user camera.
     * @param hasBackFace If true use Dual Parabolid.
     */
    ParabolidCamera(
        const ref_ptr<Mesh> &mesh,
        const ref_ptr<Camera> &userCamera,
        GLboolean hasBackFace=GL_TRUE);

    // Override
    void enable(RenderState *rs);

  protected:
    ref_ptr<Camera> userCamera_;
    ref_ptr<ShaderInputMat4> modelMatrix_;
    ref_ptr<ShaderInput3f> pos_;
    GLboolean hasBackFace_;

    GLuint positionStamp_;
    GLuint matrixStamp_;

    void update();
  };
} // namespace

#endif /* PARABOLID_CAMERA_H_ */
