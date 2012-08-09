/*
 * render-tree.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef RENDER_TREE_H_
#define RENDER_TREE_H_

#include <ogle/render-tree/state-node.h>
#include <ogle/states/attribute-state.h>
#include <ogle/states/vbo-state.h>

typedef bool (*NodeHiddenFunc)(StateNode*,void*);

class RenderTree
{
public:
  RenderTree(ref_ptr<StateNode> &node);
  RenderTree();
  ~RenderTree();

  ref_ptr<StateNode>& rootNode();

  void addChild(
      ref_ptr<StateNode> &parent,
      ref_ptr<StateNode> &child,
      GLboolean generateVBONode=true);
  void remove(ref_ptr<StateNode> &node);

  void traverse(RenderState *state);
  void traverse(RenderState *state, ref_ptr<StateNode> &node);

  void updateStates(GLfloat dt);

  void optimize();

  ref_ptr<Shader> generateShader(
      StateNode &node);
  ref_ptr<Shader> generateShader(
      StateNode &node,
      ShaderConfiguration *cfg);

protected:
  ref_ptr<StateNode> rootNode_;

  RenderTree(const RenderTree&);

  void removeFromParentVBO(
      ref_ptr<StateNode> &parent,
      ref_ptr<StateNode> &child,
      const set< AttributeState* > &geomNodes);
  bool addToParentVBO(
      ref_ptr<StateNode> &parent,
      ref_ptr<StateNode> &child,
      set< AttributeState* > &geomNodes);
  void findUnhandledGeomNodes(
      ref_ptr<StateNode> &node,
      set< AttributeState* > &ret);
  GLboolean hasUnhandledGeometry(State *s);
};

#endif /* RENDER_TREE_H_ */
