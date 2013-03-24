/*
 * qt-application.h
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#ifndef QT_APPLICATION_H_
#define QT_APPLICATION_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QApplication>
#include <regen/application.h>
#include <applications/qt/qt-gl-widget.h>
#include <applications/qt/generic-data-window.h>

#include <string>
using namespace std;

namespace ogle {
class QTGLWidget;
class QtApplication : public Application
{
public:
  QtApplication(
      int &argc, char** argv,
      GLuint width=800, GLuint height=600,
      QWidget *parent=NULL);
  virtual ~QtApplication();

  /**
   * @return topmost parent of GL widget.
   */
  QWidget* toplevelWidget();

  /**
   * @return the rendering widget.
   */
  QTGLWidget& glWidget();

  /**
   * Add generic data to editor, allowing the user to manipulate the data.
   * @param treePath path in tree widget.
   * @param in the data
   * @param minBound per component minimum
   * @param maxBound per component maximum
   * @param precision per component precision
   * @param description brief description
   */
  void addGenericData(
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
  QApplication app_;
  QTGLWidget glWidget_;
  GenericDataWindow *genericDataWindow_;

  friend class QTGLWidget;
};
}

#endif /* QT_OGLE_APPLICATION_H_ */
