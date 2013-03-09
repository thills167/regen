/*
 * feedback-state.h
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#ifndef FEEDBACK_STATE_H_
#define FEEDBACK_STATE_H_

#include <ogle/states/state.h>

namespace ogle {
/**
 * \brief Transform feedback base class.
 */
class FeedbackState : public State
{
public:
  FeedbackState(
      const GLenum &feedbackPrimitive,
      const ref_ptr<VertexBufferObject> &feedbackBuffer);

protected:
  const GLenum &feedbackPrimitive_;
  const ref_ptr<VertexBufferObject> &feedbackBuffer_;
};

/**
 * \brief Transform feedback - streaming data interleaved to the buffer.
 */
class FeedbackStateInterleaved : public FeedbackState
{
public:
  FeedbackStateInterleaved(
      const GLenum &feedbackPrimitive,
      const ref_ptr<VertexBufferObject> &feedbackBuffer);
  // override
  void enable(RenderState *state);
  void disable(RenderState *state);
};

/**
 * \brief Transform feedback - streaming data separate to the buffer.
 */
class FeedbackStateSeparate : public FeedbackState
{
public:
  FeedbackStateSeparate(
      const GLenum &feedbackPrimitive,
      const ref_ptr<VertexBufferObject> &feedbackBuffer,
      const list< ref_ptr<VertexAttribute> > &attributes);
  // override
  void enable(RenderState *state);
  void disable(RenderState *state);
protected:
  const list< ref_ptr<VertexAttribute> > &attributes_;
};
} // end ogle namespace

#endif /* FEEDBACK_STATE_H_ */
