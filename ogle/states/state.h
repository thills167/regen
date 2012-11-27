/*
 * state.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef STATE_H_
#define STATE_H_

#include <set>

#include <ogle/utility/callable.h>
#include <ogle/utility/event-object.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/gl-types/shader-config.h>
#include <ogle/gl-types/shader-input.h>

class RenderState;

/**
 * Base class for states.
 * States can be enabled,disabled and updated.
 * Also you can join states together,
 * then the joined state will be enabled after
 * the original state.
 */
class State : public EventObject
{
public:
  State();

  GLboolean isHidden() const;
  void set_isHidden(GLboolean);

  list< ref_ptr<State> >& joined();

  void joinShaderInput(ref_ptr<ShaderInput> in);
  void disjoinShaderInput(ref_ptr<ShaderInput> in);

  void joinStates(ref_ptr<State> state);
  void disjoinStates(ref_ptr<State> state);

  // TODO: remove this iface
  void addEnabler(ref_ptr<Callable> enabler);
  void addDisabler(ref_ptr<Callable> disabler);
  void removeEnabler(ref_ptr<Callable> enabler);
  void removeDisabler(ref_ptr<Callable> disabler);

  void shaderDefine(const string &name, const string &value);

  /**
   * For all joined states and this state collect all
   * uniform states and set the constant.
   */
  void setConstantUniforms(GLboolean isConstant=GL_TRUE);

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);
  virtual void configureShader(ShaderConfig*);

protected:
  map<string,string> shaderDefines_;
  list< ref_ptr<State> > joined_;
  list< ref_ptr<Callable> > enabler_;
  list< ref_ptr<Callable> > disabler_;
  GLboolean isHidden_;
};

class StateSequence : public State
{
public:
  StateSequence();
  virtual void enable(RenderState*);
  virtual void disable(RenderState*);
};

#endif /* STATE_H_ */
