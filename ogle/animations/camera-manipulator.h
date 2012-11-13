/*
 * camera-manipulator.h
 *
 *  Created on: 29.02.2012
 *      Author: daniel
 */

#ifndef CAMERA_MANIPULATOR_H_
#define CAMERA_MANIPULATOR_H_

#include <ogle/states/camera.h>
#include <ogle/animations/animation.h>

/**
 * Manipulates the view/projection matrix of a camera.
 */
class CameraManipulator : public Animation
{
public:
  /**
   * @param cam the camera to manipulate
   * @param intervalMiliseconds interval for camera manipulation
   */
  CameraManipulator(
      ref_ptr<PerspectiveCamera> cam,
      GLint intervalMiliseconds);

  /**
   * Next step in the animation.
   */
  virtual void manipulateCamera(
      const GLdouble &milliSeconds) = 0;

  // override
  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);

protected:
  ref_ptr<PerspectiveCamera> cam_;
  GLdouble intervalMiliseconds_;
};

/**
 * Linear interpolation of values.
 * Used to get continuous animations for mouse motion
 * events and similar.
 */
template<class T>
class ValueKeyFrame {
public:
  T src_;
  T dst_;
  GLdouble dt_;
  ValueKeyFrame(const T &initialValue)
  : src_(initialValue),
    dst_(initialValue),
    dt_(0.0)
  {
  }
  void setDestination(const T &dst, const GLdouble &dt) {
    dst_ = dst;
    dt_ = dt;
  }
  const T& value() const {
    return src_;
  }
  const T& value(const GLdouble &dt) {
    if(dt > dt_) {
      dt_ = 0.0;
      src_ = dst_;
    } else {
      GLdouble factor = (dt/dt_);
      dt_ -= dt;
      src_ += (dst_-src_)*factor;
    }
    return src_;
  }
};

/**
 * Camera manipulator that looks at a given position.
 */
class LookAtCameraManipulator : public CameraManipulator
{
public:
  LookAtCameraManipulator(
      ref_ptr<PerspectiveCamera> cam, GLint intervalMiliseconds);

  /**
   * the look at position.
   */
  void set_lookAt(const Vec3f &lookAt, const GLdouble &dt=0.0);
  /**
   * Degree of rotation around the position.
   */
  void set_degree(GLfloat degree, const GLdouble &dt=0.0);
  /**
   * Distance to look at point in xz plane.
   */
  void set_radius(GLfloat radius, const GLdouble &dt=0.0);

  /**
   * Distance to look at point y direction.
   */
  void set_height(GLfloat height, const GLdouble &dt=0.0);

  /**
   * camera will move length units each timestep
   */
  void setStepLength(GLfloat length, const GLdouble &dt=0.0);

  /**
   * The camera height.
   */
  GLfloat height() const;
  /**
   * The camera radius.
   */
  GLfloat radius() const;

  // override
  virtual void manipulateCamera(const GLdouble &milliSeconds);

protected:
  ValueKeyFrame<Vec3f> lookAt_;
  ValueKeyFrame<GLdouble> radius_;
  ValueKeyFrame<GLdouble> height_;
  ValueKeyFrame<GLdouble> deg_;
  ValueKeyFrame<GLdouble> stepLength_;
};

/**
 * Interpolate linear between current position and destination.
 */
class CameraLinearPositionManipulator : public CameraManipulator
{
public:
  CameraLinearPositionManipulator(
      ref_ptr<PerspectiveCamera> cam, GLint intervalMiliseconds);

  /**
   * camera will move to this position (not changing direction)
   */
  void setDestinationPosition(const Vec3f &destination);
  /**
   * camera will move length units each timestep
   */
  void setStepLength(GLdouble length);

  // override
  virtual void manipulateCamera(const GLdouble &milliSeconds);

protected:
  Vec3f destination_;
  GLdouble stepLength_;
  GLboolean arrived_;
};

#endif /* CAMERA_MANIPULATOR_H_ */
