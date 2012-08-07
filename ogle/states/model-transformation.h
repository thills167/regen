/*
 * model-transformation-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef MODEL_TRANSFORMATION_STATE_H_
#define MODEL_TRANSFORMATION_STATE_H_

#include <ogle/states/state.h>
#include <ogle/av/audio-source.h>
#include <ogle/algebra/quaternion.h>

/**
 * Provides a model transformation matrix that
 * is applied to the vertex coordinates before the view
 * matrix is applied.
 */
class ModelTransformationState : public State
{
public:
  ModelTransformationState();
  /**
   * Translate model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void translate(const Vec3f &translation, float dt);
  /**
   * Set translation components of model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void setTranslation(const Vec3f &translation, float dt);

  /**
   * Scale model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void scale(const Vec3f &scaling, float dt);

  /**
   * Rotate model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void rotate(const Quaternion &rotation, float dt);

  /**
   * Sets the model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void set_modelMat(const Mat4f &m, float dt);

  /**
   * Sets the model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void set_modelMat(
      const Vec3f &translation,
      const Quaternion &rotation, float dt);

  /**
   * Sets the model matrix.
   * dt in milliseconds for velocity calculation.
   */
  void set_modelMat(
      const Vec3f &translation,
      const Quaternion &rotation,
      const Vec3f &scaling, float dt);

  /**
   * The model matrix used to transform from object space to orld space.
   */
  const Mat4f& modelMat() const {
    return modelMat_->value();
  }
  /**
   * The audio source associated to the world position
   * of the transformation.
   */
  void set_audioSource(ref_ptr<AudioSource> &audioSource);
  /**
   * The audio source associated to the world position
   * of the transformation.
   */
  bool isAudioSource() const;

protected:
  // model matrix
  ref_ptr<UniformMat4> modelMat_;
  ref_ptr<AudioSource> audioSource_;

  ref_ptr<UniformVec3> velocity_;
  Vec3f lastPosition_;

  void updateVelocity(float dt);
  void updateAudioSource();
};

#endif /* MODEL_TRANSFORMATION_STATE_H_ */
