/*
 * camera.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <ogle/states/state.h>
#include <ogle/utility/callable.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/algebra/matrix.h>
#include <ogle/gl-types/uniform.h>

/**
 * Base class for camera's.
 * Just provides the projection matrix.
 */
class Camera : public State
{
public:
  Camera();
  UniformMat4* projectionUniform();
protected:
  ref_ptr<UniformMat4> projectionUniform_;
};

/**
 * Camera with orthogonal projection.
 */
class OrthoCamera : public Camera
{
public:
  OrthoCamera();
  void updateProjection(
      GLfloat right, GLfloat top);
};

/**
 * Provides view transformation related
 * uniforms (view matrix, camera position, ..).
 */
class PerspectiveCamera : public Camera
{
public:
  typedef enum {
    DIRECTION_UP,
    DIRECTION_RIGHT,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_FRONT,
    DIRECTION_BACK
  }Direction;

  PerspectiveCamera();

  /**
   * Update the uniform values.
   * Should be done in rendering thread.
   */
  virtual void update(GLfloat dt);
  /**
   * Update the matrices.
   * Could be called from an animation thread.
   */
  void updatePerspective(GLfloat dt);

  void updateProjection(
      GLfloat fov,
      GLfloat near,
      GLfloat far,
      GLfloat aspect);

  /**
   * Matrix projecting world space to view space.
   */
  void set_viewMatrix(const Mat4f &viewMatrix);
  /**
   * Matrix projecting world space to view space.
   */
  const Mat4f& viewMatrix() const;

  /**
   * Inverse of the matrix projecting world space to view space.
   */
  const Mat4f& inverseViewMatrix() const;

  /**
   * Position of the camera in world space.
   */
  void set_position(const Vec3f &position);
  /**
   * Position of the camera in world space.
   */
  const Vec3f& position() const;

  /**
   * Direction of the camera.
   */
  const Vec3f& direction() const;
  /**
   * Direction of the camera.
   */
  void set_direction(const Vec3f &direction);

  /**
   * Camera velocity.
   */
  const Vec3f& velocity() const;

  /**
   * Model view matrix of the camera.
   */
  UniformMat4* viewUniform();
  /**
   * VIEW^(-1) * PROJECTION^(-1)
   */
  UniformMat4* viewProjectionUniform();
  /**
   * PROJECTION^(-1) * VIEW^(-1)
   */
  UniformMat4* inverseViewProjectionUniform();
  /**
   * VIEW^(-1)
   */
  UniformMat4* inverseViewUniform();
  /**
   * PROJECTION^(-1)
   */
  UniformMat4* inverseProjectionUniform();

  /**
   * Rotates camera by specified amount.
   */
  void rotate(float xAmplitude, float yAmplitude, float deltaT);
  /**
   * Translates camera by specified amount.
   */
  void translate(Direction direction, float deltaT);

  /**
   * Sensitivity of movement.
   */
  float sensitivity() const;
  /**
   * Sensitivity of movement.
   */
  void set_sensitivity(float sensitivity);

  /**
   * Speed of movement.
   */
  float walkSpeed() const;
  /**
   * Speed of movement.
   */
  void set_walkSpeed(float walkSpeed);

  /**
   * Sets this camera to be the audio listener.
   * This is an exclusive state.
   */
  void set_isAudioListener(GLboolean useAudio);

protected:
  Vec3f position_;
  Vec3f direction_;

  Mat4f invView_;
  Mat4f view_;
  Mat4f viewProjection_;
  Mat4f invViewProjection_;
  ref_ptr<UniformMat4> viewUniform_;
  ref_ptr<UniformMat4> invViewUniform_;
  ref_ptr<UniformMat4> viewProjectionUniform_;
  ref_ptr<UniformMat4> invViewProjectionUniform_;
  ref_ptr<UniformMat4> invProjectionUniform_;

  Vec3f lastPosition_;
  ref_ptr<UniformVec3> cameraPositionUniform_;

  ref_ptr<UniformFloat> fovUniform_;
  ref_ptr<UniformFloat> nearUniform_;
  ref_ptr<UniformFloat> farUniform_;
  ref_ptr<UniformVec3> velocity_;

  GLfloat sensitivity_;
  GLfloat walkSpeed_;
  GLfloat aspect_;

  GLboolean isAudioListener_;
};

#endif /* _CAMERA_H_ */
