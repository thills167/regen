/*
 * state.cpp
 *
 *  Created on: 01.08.2012
 *      Author: daniel
 */

#include "state.h"
#include "uniform-state.h"

State::State()
: EventObject()
{
}

void State::configureShader(ShaderConfiguration *cfg)
{
  for(list< ref_ptr<State> >::iterator
      it=joinned_.begin(); it!=joinned_.end(); ++it)
  {
    (*it)->configureShader(cfg);
  }
}

void State::enable(RenderState *state)
{
  for(list< ref_ptr<Callable> >::iterator
      it=enabler_.begin(); it!=enabler_.end(); ++it)
  {
    (*it)->call();
  }
  for(list< ref_ptr<State> >::iterator
      it=joinned_.begin(); it!=joinned_.end(); ++it)
  {
    (*it)->enable(state);
  }
}
void State::disable(RenderState *state)
{
  for(list< ref_ptr<State> >::iterator
      it=joinned_.begin(); it!=joinned_.end(); ++it)
  {
    (*it)->disable(state);
  }
  for(list< ref_ptr<Callable> >::iterator
      it=disabler_.begin(); it!=disabler_.end(); ++it)
  {
    (*it)->call();
  }
}

void State::joinStates(ref_ptr<State> &state)
{
  joinned_.push_back(state);
}
void State::disjoinStates(ref_ptr<State> &state)
{
  for(list< ref_ptr<State> >::iterator
      it=joinned_.begin(); it!=joinned_.end(); ++it)
  {
    if(it->get() == state.get())
    {
      joinned_.erase(it);
      return;
    }
  }
}

void State::joinStates(ref_ptr<Uniform> uniform)
{
  ref_ptr<State> s = ref_ptr<State>::manage(new UniformState(uniform));
  joinStates(s);
}
void State::disjoinStates(ref_ptr<Uniform> uniform)
{
  for(list< ref_ptr<State> >::iterator
      it=joinned_.begin(); it!=joinned_.end(); ++it)
  {
    State *state = it->get();
    UniformState *uniformState = dynamic_cast<UniformState*>(state);
    if(uniformState!=NULL &&
        uniformState->uniform().get()==uniform.get())
    {
      joinned_.erase(it);
      return;
    }
  }
}

void State::addEnabler(ref_ptr<Callable> &enabler)
{
  enabler_.push_back(enabler);
}
void State::addDisabler(ref_ptr<Callable> &disabler)
{
  disabler_.push_back(disabler);
}

void State::removeEnabler(ref_ptr<Callable> &enabler)
{
  for(list< ref_ptr<Callable> >::iterator
      it=enabler_.begin(); it!=enabler_.end(); ++it)
  {
    if(it->get() == enabler.get())
    {
      enabler_.erase(it);
      return;
    }
  }
}
void State::removeDisabler(ref_ptr<Callable> &disabler)
{
  for(list< ref_ptr<Callable> >::iterator
      it=disabler_.begin(); it!=disabler_.end(); ++it)
  {
    if(it->get() == disabler.get())
    {
      disabler_.erase(it);
      return;
    }
  }
  disabler_.push_back(disabler);
}
