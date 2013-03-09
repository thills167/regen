/*
 * atomic-states.h
 *
 *  Created on: 28.02.2013
 *      Author: daniel
 */

#ifndef ATOMIC_STATES_H_
#define ATOMIC_STATES_H_

#include <ogle/states/state.h>

namespace ogle {
/**
 * \brief Base class for 'atomic' states.
 *
 * Atomic states just push and pop to the RenderState.
 */
class ServerSideState : public State {};

/**
 * \brief Toggles server side GL state.
 */
class ToggleState : public ServerSideState
{
public:
  ToggleState(RenderState::Toggle key, GLboolean toggle)
  : ServerSideState(), key_(key), toggle_(toggle){}

  /**
   * @return the toggle key.
   */
  RenderState::Toggle key() const
  { return key_; }
  /**
   * @return toggle on ?
   */
  GLboolean toggle() const
  { return toggle_; }

  void enable(RenderState *rs)
  { rs->toggles().push(key_, toggle_); }
  void disable(RenderState *rs)
  { rs->toggles().pop(key_); }

protected:
  RenderState::Toggle key_;
  GLboolean toggle_;
};

/**
 * \brief Specifies the depth comparison function.
 *
 * Symbolic constants GL_NEVER,GL_LESS,GL_EQUAL,GL_LEQUAL,GL_GREATER,
 * GL_NOTEQUAL,GL_GEQUAL,GL_ALWAYS are accepted.
 * The initial value is GL_LESS.
 */
class DepthFuncState : public ServerSideState
{
public:
  DepthFuncState(GLenum depthFunc)
  : ServerSideState(), depthFunc_(depthFunc) {}

  void enable(RenderState *rs)
  { rs->depthFunc().push(depthFunc_); }
  void disable(RenderState *rs)
  { rs->depthFunc().pop(); }

protected:
  GLenum depthFunc_;
};

/**
 * \brief Specify mapping of depth values from normalized device coordinates
 * to window coordinates.
 *
 * 'nearVal' specifies the mapping of the near clipping plane to window coordinates.
 * The initial value is 0.
 * 'farVal' specifies the mapping of the far clipping plane to window coordinates.
 * The initial value is 1.
 */
class DepthRangeState : public ServerSideState
{
public:
  DepthRangeState(GLdouble nearVal, GLdouble farVal)
  : ServerSideState(), nearVal_(nearVal), farVal_(farVal) {}

  void enable(RenderState *rs)
  { rs->depthRange().push(DepthRange(nearVal_, farVal_)); }
  void disable(RenderState *rs)
  { rs->depthRange().pop(); }

protected:
  GLdouble nearVal_, farVal_;
};

/**
 * \brief Specifies whether the depth buffer is enabled for writing.
 *
 * If flag is GL_FALSE, depth buffer writing is disabled.
 * Otherwise, it is enabled. Initially, depth buffer writing is enabled.
 */
class ToggleDepthWriteState : public ServerSideState
{
public:
  ToggleDepthWriteState(GLboolean toggle)
  : ServerSideState(), toggle_(toggle) { }

  void enable(RenderState *rs)
  { rs->depthMask().push(toggle_); }
  void disable(RenderState *rs)
  { rs->depthMask().pop(); }

protected:
  GLboolean toggle_;
};

/**
 * \brief Set the blend color.
 *
 * Initially the GL_BLEND_COLOR is set to (0,0,0,0).
 */
class BlendColorState : public ServerSideState
{
public:
  BlendColorState(const Vec4f &col) : ServerSideState(), col_(col) {}

  void enable(RenderState *state)
  { state->blendColor().push(col_); }
  void disable(RenderState *state)
  { state->blendColor().pop(); }

protected:
  Vec4f col_;
};

/**
 * \brief Specify the equation used for both the RGB blend equation and the
 * Alpha blend equation.
 *
 * 'buf' specifies the index of the draw buffer for which to set the blend equation.
 * 'mode' specifies how source and destination colors are combined.
 * It must be GL_FUNC_ADD,GL_FUNC_SUBTRACT,GL_FUNC_REVERSE_SUBTRACT,GL_MIN,GL_MAX.
 * Initially, both the RGB blend equation and the alpha blend equation
 * are set to GL_FUNC_ADD.
 */
class BlendEquationState : public ServerSideState
{
public:
  BlendEquationState(GLenum equation)
  : ServerSideState(), equation_(BlendEquation(equation,equation)) {}

  void enable(RenderState *state)
  { state->blendEquation().push(equation_); }
  void disable(RenderState *state)
  { state->blendEquation().pop(); }

protected:
  BlendEquation equation_;
};

/**
 * \brief Specify pixel arithmetic.
 *
 * 'buf' specifies the index of the draw buffer for which to set the blend function.
 * v.xz-'sfactor' specifies how the red, green, blue, and alpha source blending
 * factors are computed. The initial value is GL_ONE.
 * v.yw-'dfactor' specifies how the red, green, blue, and alpha destination
 * blending factors are computed.
 * The following symbolic constants are accepted:
 * GL_ZERO,GL_ONE,GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR,GL_DST_COLOR,
 * GL_ONE_MINUS_DST_COLOR,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,
 * GL_DST_ALPHA,GL_ONE_MINUS_DST_ALPHA.GL_CONSTANT_COLOR,
 * GL_ONE_MINUS_CONSTANT_COLOR,GL_CONSTANT_ALPHA,
 * GL_ONE_MINUS_CONSTANT_ALPHA. The initial value is GL_ZERO.
 */
class BlendFuncState : public ServerSideState
{
public:
  BlendFuncState(
      GLenum srcRGB, GLenum dstRGB,
      GLenum srcAlpha, GLenum dstAlpha)
  : ServerSideState(), func_(BlendFunction(srcRGB,dstRGB,srcAlpha,dstAlpha)) {}

  void enable(RenderState *state)
  { state->blendFunction().push(func_); }
  void disable(RenderState *state)
  { state->blendFunction().pop(); }

protected:
  BlendFunction func_;
};

/**
 * \brief Specifies whether front- or back-facing facets are candidates for culling.
 *
 * Symbolic constants GL_FRONT,GL_BACK, GL_FRONT_AND_BACK are accepted.
 * The initial value is GL_BACK.
 */
class CullFaceState : public ServerSideState
{
public:
  CullFaceState(GLenum face) : ServerSideState(), face_(face) {}

  void enable(RenderState *rs)
  { rs->cullFace().push(face_); }
  void disable(RenderState *rs)
  { rs->cullFace().pop(); }

protected:
  GLenum face_;
};

/**
 * \brief Set the scale and units used to calculate depth values.
 *
 * v.x-'factor' specifies a scale factor that is used to create a variable
 * depth offset for each polygon. The initial value is 0.
 * v.y-'units' is multiplied by an implementation-specific value to
 * create a constant depth offset. The initial value is 0.
 * This state also enables the polygon offset toggle.
 */
class PolygonOffsetState : public ServerSideState
{
public:
  PolygonOffsetState(GLfloat factor, GLfloat units)
  : ServerSideState(), factor_(factor), units_(units) {}

  void enable(RenderState *rs) {
    rs->toggles().push(RenderState::POLYGON_OFFSET_FILL, GL_TRUE);
    rs->polygonOffset().push(Vec2f(factor_, units_));
  }
  void disable(RenderState *rs) {
    rs->toggles().pop(RenderState::POLYGON_OFFSET_FILL);
    rs->polygonOffset().pop();
  }

protected:
  GLfloat factor_, units_;
};

/**
 * \brief Specifies how polygons will be rasterized.
 *
 * Accepted values are GL_POINT,GL_LINE,GL_FILL.
 * The initial value is GL_FILL for both front- and back-facing polygons.
 */
class FillModeState : public ServerSideState
{
public:
  FillModeState(GLenum mode) : ServerSideState(), mode_(mode) {}

  void enable(RenderState *rs)
  { rs->polygonMode().push(mode_); }
  void disable(RenderState *rs)
  { rs->polygonMode().pop(); }

protected:
  GLenum mode_;
};

/**
 * \brief Specifies the number of vertices that
 * will be used to make up a single patch primitive.
 */
class PatchVerticesState : public ServerSideState
{
public:
  PatchVerticesState(GLuint numPatchVertices)
  : ServerSideState(), numPatchVertices_(numPatchVertices) {}

  void enable(RenderState *rs)
  { rs->patchVertices().push(numPatchVertices_); }
  void disable(RenderState *rs)
  { rs->patchVertices().pop(); }

protected:
  GLuint numPatchVertices_;
};

/**
 * \brief Specifies the default outer or inner tessellation levels
 * to be used when no tessellation control shader is present.
 */
class PatchLevelState : public ServerSideState
{
public:
  PatchLevelState(const ref_ptr<ShaderInput4f> &inner, const ref_ptr<ShaderInput4f> &outer)
  : ServerSideState(), inner_(inner), outer_(outer) {}

  void enable(RenderState *rs)
  { rs->patchLevel().push(PatchLevels(inner(), outer())); }
  void disable(RenderState *rs)
  { rs->patchLevel().pop(); }

  /**
   * @return the inner patch level.
   */
  const Vec4f& inner() const
  { return inner_->getVertex4f(0); }
  /**
   * @return the outer patch level.
   */
  const Vec4f& outer() const
  { return outer_->getVertex4f(0); }

protected:
  ref_ptr<ShaderInput4f> inner_;
  ref_ptr<ShaderInput4f> outer_;
};

} // end ogle namespace

#endif /* ATOMIC_STATES_H_ */
