
#ifndef SHADER_INPUT_WIDGET_H
#define SHADER_INPUT_WIDGET_H

#include <QtGui/QMainWindow>

#include <regen/gl-types/shader-input.h>
#include <regen/animations/animation.h>
#include <regen/states/state-node.h>
#include <applications/qt/shader-input-editor.h>

namespace regen {
/**
 * \brief Allows editing ShaderInput values.
 */
class ShaderInputWidget : public QWidget
{
Q_OBJECT

public:
  ShaderInputWidget(QWidget *parent = 0);
  ~ShaderInputWidget();

  void setNode(const ref_ptr<StateNode> &node);

public slots:
  void resetValue();
  void valueUpdated();
  void activateValue(QTreeWidgetItem*,QTreeWidgetItem*);

protected:
  Ui_shaderInputEditor ui_;
  QTreeWidgetItem *selectedItem_;
  ShaderInput *selectedInput_;
  ref_ptr<Animation> setValueCallback_;
  GLboolean ignoreValueChanges_;

  map<ShaderInput*,byte*> initialValue_;
  map<ShaderInput*,GLuint> initialValueStamp_;
  map<ShaderInput*,GLuint> valueStamp_;

  map<QTreeWidgetItem*, ref_ptr<ShaderInput> > inputs_;

  bool handleState(
      const ref_ptr<State> &state,
      QTreeWidgetItem *parent);
  bool handleNode(
      const ref_ptr<StateNode> &node,
      QTreeWidgetItem *parent);
  bool handleInput(
      const NamedShaderInput &input,
      QTreeWidgetItem *parent);

  void updateInitialValue(ShaderInput *x);
};
}

#endif /* SHADER_INPUT_WIDGET_H */
