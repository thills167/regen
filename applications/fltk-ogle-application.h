/*
 * fltk-application.h
 *
 *  Created on: 09.08.2012
 *      Author: daniel
 */

#ifndef FLTK_APPLICATION_H_
#define FLTK_APPLICATION_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <applications/ogle-application.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>

#include <string>
using namespace std;

class OGLEFltkApplication;

struct InputCallbackData {
  OGLEFltkApplication *app;
  ShaderInput *in;
  GLuint index;
  string name;
};
struct ValueChangedHandler {
  void (*function)(void*);
  void *data;
  ValueChangedHandler(void (*_function)(void*), void *_data)
  : function(_function),
    data(_data) {}
};

class OGLEFltkApplication : public OGLEApplication
{
public:
  OGLEFltkApplication(
      const ref_ptr<RenderTree> &tree,
      int &argc, char** argv,
      GLuint width=800, GLuint height=600);
  virtual ~OGLEFltkApplication();

  virtual void createWidgets(Fl_Pack *parent);
  void createShaderInputWidget();

  void addShaderInput(const ref_ptr<ShaderInput1f> &in,
      GLfloat min, GLfloat max, GLint precision=4);
  void addShaderInput(const ref_ptr<ShaderInput2f> &in,
      GLfloat min, GLfloat max, GLint precision=4);
  void addShaderInput(const ref_ptr<ShaderInput3f> &in,
      GLfloat min, GLfloat max, GLint precision=4);
  void addShaderInput(const ref_ptr<ShaderInput4f> &in,
      GLfloat min, GLfloat max, GLint precision=4);
  void addShaderInput(const ref_ptr<ShaderInput1i> &in, GLint min, GLint max);
  void addShaderInput(const ref_ptr<ShaderInput2i> &in, GLint min, GLint max);
  void addShaderInput(const ref_ptr<ShaderInput3i> &in, GLint min, GLint max);
  void addShaderInput(const ref_ptr<ShaderInput4i> &in, GLint min, GLint max);

  void set_windowTitle(const string &windowTitle);
  void set_height(GLuint height);
  void set_width(GLuint width);
  void set_displayMode(GLuint displayMode);

  virtual void show();
  virtual int mainLoop();
  virtual void exitMainLoop(int errorCode);
  void postRedisplay();

  void addValueChangedHandler(
      const string &value, void (*function)(void*), void *data);
  void valueChanged(const string &value);

  void resize(GLuint width, GLuint height);

  void setKeepAspect();
  void setFixedSize();
  void toggleFullscreen();

protected:
  string windowTitle_;
  GLuint fltkHeight_;
  GLuint fltkWidth_;

  Fl_Window mainWindow_;
  Fl_Pack *mainWindowPackH_;
  Fl_Pack *mainWindowPackV_;

  Fl_Window *uniformWindow_;
  Fl_Scroll *uniformScroll_;
  GLuint uniformScrollY_;

  map<string, list<ValueChangedHandler> > valueChangedHandler_;

  GLboolean isApplicationRunning_;

  class GLWindow : public Fl_Gl_Window
  {
  public:
    GLWindow(
        OGLEFltkApplication *app,
        GLint x=0, GLint y=0,
        GLint width=800, GLint height=600);
  protected:
    OGLEFltkApplication *app_;
    void resize(int x, int y, int w, int h);
    void draw();
    void flush();
    int handle(int);
  };
  GLWindow *fltkWindow_;

  boost::posix_time::ptime lastButtonTime_;

  virtual void initGL();
  virtual void swapGL();

  void addShaderInput(const string &name);
  void addShaderInputf(
      const string &name,
      ShaderInputf *in,
      GLfloat min, GLfloat max,
      GLint precision);
  void addShaderInputi(
      const string &name,
      ShaderInputi *in,
      GLint min, GLint max);
};

#endif /* GLUT_APPLICATION_H_ */
