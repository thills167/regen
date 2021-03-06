/*
 * scene-node.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef SCENE_NODE_H_
#define SCENE_NODE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/scene/resource-manager.h>

#define REGEN_NODE_CATEGORY "node"

#include <regen/states/state-node.h>

namespace regen {
namespace scene {
  /**
   * Sorts children using Eye-Depth comparator.
   */
  class SortByModelMatrix : public State
  {
  public:
    /**
     * Default constructor.
     * @param n The scene node.
     * @param cam Camera reference.
     * @param frontToBack If true sorting is done front to back.
     */
    SortByModelMatrix(
        const ref_ptr<StateNode> &n,
        const ref_ptr<Camera> &cam,
        GLboolean frontToBack)
    : State(),
      n_(n),
      comparator_(cam,frontToBack)
    {}

    void enable(RenderState *state)
    { n_->childs().sort(comparator_); }

  protected:
    ref_ptr<StateNode> n_;
    NodeEyeDepthComparator comparator_;
  };

  /**
   * Processes SceneInput and creates StateNode's.
   */
  class SceneNodeProcessor : public NodeProcessor {
  public:
    SceneNodeProcessor()
    : NodeProcessor(REGEN_NODE_CATEGORY)
    {}

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &parent)
    {
      ref_ptr<StateNode> newNode;

      if(input.hasAttribute("import")) {
        // Handle node imports
        const ref_ptr<SceneInputNode> &root = parser->getRoot();
        const string importID = input.getValue("import");

        // TODO: Use previously loaded node. It's problematic because...
        //   - Shaders below the node are configured only for the first context.
        //   - Uniforms above node are joined into shader
        ref_ptr<SceneInputNode> imported =
            root->getFirstChild(REGEN_NODE_CATEGORY, importID);
        if(imported.get()==NULL) {
          REGEN_WARN("Unable to import node '" << importID << "'.");
        }
        else {
          newNode = createNode(parser,*imported.get(),parent);
          handleAttributes(parser,*imported.get(),newNode);
          handleAttributes(parser,input,newNode);
          handleChildren(parser,input,newNode);
          handleChildren(parser,*imported.get(),newNode);
        }
      }
      else {
        newNode = createNode(parser,input,parent);
        handleAttributes(parser,input,newNode);
        handleChildren(parser,input,newNode);
      }
    }

  protected:
    void handleAttributes(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &newNode)
    {
      if(input.hasAttribute("sort")) {
        // Sort node children by model view matrix.
        GLuint sortMode = input.getValue<GLuint>("sort",0);
        ref_ptr<Camera> sortCam =
            parser->getResources()->getCamera(parser,input.getValue<string>("sort-camera",""));
        if(sortCam.get()==NULL) {
          REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
        }
        else {
          newNode->state()->joinStatesFront(
              ref_ptr<SortByModelMatrix>::alloc(newNode,sortCam,(sortMode==1)));
        }
      }
    }
    void handleChildren(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &newNode)
    {
      // Process node children
      const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
      for(list< ref_ptr<SceneInputNode> >::const_iterator
          it=childs.begin(); it!=childs.end(); ++it)
      {
        const ref_ptr<SceneInputNode> &x = *it;
        // First try node processor
        ref_ptr<NodeProcessor> nodeProcessor = parser->getNodeProcessor(x->getCategory());
        if(nodeProcessor.get()!=NULL) {
          nodeProcessor->processInput(parser, *x.get(), newNode);
          continue;
        }
        // Second try state processor
        ref_ptr<StateProcessor> stateProcessor = parser->getStateProcessor(x->getCategory());
        if(stateProcessor.get()!=NULL) {
          stateProcessor->processInput(parser, *x.get(), newNode->state());
          continue;
        }
        REGEN_WARN("No processor registered for '" << x->getDescription() << "'.");
      }
    }
    ref_ptr<StateNode> createNode(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &parent)
    {
      GLuint numIterations = 1;
      if(input.hasAttribute("num-iterations")) {
        numIterations = input.getValue<GLuint>("num-iterations",1u);
      }

      ref_ptr<State> state = ref_ptr<State>::alloc();
      ref_ptr<StateNode> newNode;
      if(numIterations>1) {
        newNode = ref_ptr<LoopNode>::alloc(state,numIterations);
      }
      else {
        newNode = ref_ptr<StateNode>::alloc(state);
      }
      newNode->set_name(input.getName());
      parent->addChild(newNode);
      parser->putNode(input.getName(), newNode);

      return newNode;
    }
  };
}}

#endif /* SCENE_NODE_H_ */
