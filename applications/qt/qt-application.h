/*
 * qt-application.h
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#ifndef QT_APPLICATION_H_
#define QT_APPLICATION_H_

#include <GL/glew.h>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QApplication>
#include <regen/application.h>
#include <applications/qt/qt-gl-widget.h>
#include <applications/qt/shader-input-widget.h>

#include <string>
using namespace std;

namespace regen {
class QTGLWidget;
class QtApplication : public Application
{
public:
  QtApplication(
      const int &argc, const char** argv,
      const QGLFormat &glFormat,
      GLuint width=800, GLuint height=600,
      QWidget *parent=NULL);

  /**
   * @return topmost parent of GL widget.
   */
  QWidget* toplevelWidget();

  /**
   * @return the rendering widget.
   */
  QTGLWidget* glWidget();
  QWidget* glWidgetContainer();

  /**
   * Add generic data to editor, allowing the user to manipulate the data.
   * @param treePath path in tree widget.
   * @param in the data
   * @param minBound per component minimum
   * @param maxBound per component maximum
   * @param precision per component precision
   * @param description brief description
   */
  void addShaderInput(
      const string &treePath,
      const ref_ptr<ShaderInput> &in,
      const Vec4f &minBound,
      const Vec4f &maxBound,
      const Vec4i &precision,
      const string &description);

  void toggleFullscreen();

  void show();

  int mainLoop();
  void exitMainLoop(int errorCode);

protected:
  QApplication *app_;
  QWidget *glContainer_;
  QTGLWidget *glWidget_;
  ShaderInputWidget *shaderInputWidget_;
  GLboolean isMainloopRunning_;
  GLint exitCode_;

  friend class QTGLWidget;
};
}

#endif /* QT_APPLICATION_H_ */
