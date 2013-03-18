/*
 * fltk-application.cpp
 *
 *  Created on: 09.08.2012
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include <ogle/utility/font-manager.h>
#include <ogle/utility/logging.h>
#include <ogle/utility/gl-util.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/external/glsw/glsw.h>

#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>

#include "fltk-application.h"
using namespace ogle;

//////

static int shortcutEater(int event) {
  if(event == FL_SHORTCUT) return 1;
  return 0;
}
static void changeValueCallbackf_(Fl_Widget *widget, void *data) {
  Fl_Valuator *valueWidget = (Fl_Valuator*)widget;
  InputCallbackData *cbData = (InputCallbackData*) data;

  GLfloat *v = (GLfloat*) cbData->in->dataPtr();
  v[cbData->index] = (GLfloat) valueWidget->value();
  cbData->in->setUniformDataUntyped((byte*)v);

  cbData->app->valueChanged(cbData->name);
}
static void changeValueCallbacki_(Fl_Widget *widget, void *data) {
  Fl_Valuator *valueWidget = (Fl_Valuator*)widget;
  InputCallbackData *cbData = (InputCallbackData*) data;

  GLint *v = (GLint*) cbData->in->dataPtr();
  v[cbData->index] = (GLint) valueWidget->value();
  cbData->in->setUniformDataUntyped((byte*)v);

  cbData->app->valueChanged(cbData->name);
}
static void closeApplicationCallback_(Fl_Widget *widget, void *data)
{
  FltkApplication *app = (FltkApplication*) data;
  app->exitMainLoop(0);
}

FltkApplication::FltkApplication(
    const ref_ptr<RootNode> &tree,
    int &argc, char** argv,
    GLuint width, GLuint height)
: OGLEApplication(tree,argc,argv,width,height),
  windowTitle_("OpenGL Engine"),
  fltkHeight_(height),
  fltkWidth_(width),
  mainWindow_(width,height),
  mainWindowPackH_(NULL),
  isApplicationRunning_(GL_TRUE)
{
  lastButtonTime_ = lastMotionTime_;

  Fl::scheme("GTK+");
  // clearlook background
  Fl::background(0xed, 0xec, 0xeb);
  Fl::add_handler(shortcutEater);
}
FltkApplication::~FltkApplication()
{

}

void FltkApplication::toggleFullscreen()
{
  if(mainWindow_.fullscreen_active()) {
    mainWindow_.fullscreen_off();
  }
  else {
    mainWindow_.fullscreen();
  }
}

void FltkApplication::createShaderInputWidget()
{
}

void FltkApplication::addValueChangedHandler(
    const string &value, void (*function)(void*), void *data)
{
  valueChangedHandler_[value].push_back(ValueChangedHandler(function,data));
}
void FltkApplication::valueChanged(const string &value)
{
  list<ValueChangedHandler> &handler = valueChangedHandler_[value];
  for(list<ValueChangedHandler>::iterator
      it=handler.begin(); it!=handler.end(); ++it)
  {
    it->function(it->data);
  }
}

void FltkApplication::addShaderInput(const string &name)
{
  static const int windowWidth = 340;
  static const int windowHeight = 600;
  static const int labelHeight = 24;

  if(uniformWindow_==NULL) {
    uniformWindow_ = new Fl_Window(windowWidth,windowHeight);
    uniformWindow_->callback(closeApplicationCallback_, this);
    uniformWindow_->end();
    uniformWindow_->show();

    uniformScroll_ = new Fl_Scroll(0,0,windowWidth,windowHeight);
    uniformScrollY_ = 0;
    uniformWindow_->add(uniformScroll_);
  }

  Fl_Box *nameWidget = new Fl_Box(0,uniformScrollY_,uniformScroll_->w()-20,labelHeight);
  nameWidget->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
  nameWidget->label(name.c_str());
  uniformScroll_->add(nameWidget);
  uniformScrollY_ += labelHeight;
}

void FltkApplication::addShaderInputf(
    const string &name,
    ShaderInputf *in,
    GLfloat min, GLfloat max,
    GLint precision)
{
  static const int valuatorHeight = 24;

  addShaderInput(name);

  GLfloat *v = (GLfloat*) in->data();
  for(GLuint i=0; i<in->valsPerElement(); ++i)
  {
    InputCallbackData *data = new InputCallbackData;
    data->in = in;
    data->index = i;
    data->app = this;
    data->name = name;
    Fl_Hor_Value_Slider *valueWidget =
        new Fl_Hor_Value_Slider(0,uniformScrollY_,uniformScroll_->w()-20,valuatorHeight);
    valueWidget->bounds(min, max);
    valueWidget->precision(precision);
    valueWidget->value(v[i]);
    valueWidget->callback(changeValueCallbackf_, data);
    uniformScroll_->add(valueWidget);
    uniformScrollY_ += valuatorHeight;
  }
}

void FltkApplication::addShaderInputi(
    const string &name,
    ShaderInputi *in,
    GLint min, GLint max)
{
  static const int valuatorHeight = 24;

  addShaderInput(name);

  GLint *v = (GLint*) in->data();
  for(GLuint i=0; i<in->valsPerElement(); ++i)
  {
    InputCallbackData *data = new InputCallbackData;
    data->in = in;
    data->index = i;
    data->app = this;
    data->name = name;
    Fl_Hor_Value_Slider *valueWidget =
        new Fl_Hor_Value_Slider(0,uniformScrollY_,uniformScroll_->w()-20,valuatorHeight);
    valueWidget->bounds(min, max);
    valueWidget->precision(0);
    valueWidget->value(v[i]);
    valueWidget->callback(changeValueCallbacki_, data);
    uniformScroll_->add(valueWidget);
    uniformScrollY_ += valuatorHeight;
  }
}

void FltkApplication::addShaderInput(
    const ref_ptr<ShaderInput1f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  addShaderInputf(in->name(), in.get(), min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void FltkApplication::addShaderInput(
    const ref_ptr<ShaderInput2f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  addShaderInputf(in->name(), in.get(), min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void FltkApplication::addShaderInput(
    const ref_ptr<ShaderInput3f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  addShaderInputf(in->name(), in.get(), min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void FltkApplication::addShaderInput(
    const ref_ptr<ShaderInput4f> &in,
    GLfloat min, GLfloat max, GLint precision)
{
  addShaderInputf(in->name(), in.get(), min, max, precision);
  in->set_isConstant(GL_FALSE);
}
void FltkApplication::addShaderInput(const ref_ptr<ShaderInput1i> &in, GLint min, GLint max)
{
  addShaderInputi(in->name(), in.get(), min, max);
  in->set_isConstant(GL_FALSE);
}
void FltkApplication::addShaderInput(const ref_ptr<ShaderInput2i> &in, GLint min, GLint max)
{
  addShaderInputi(in->name(), in.get(), min, max);
  in->set_isConstant(GL_FALSE);
}
void FltkApplication::addShaderInput(const ref_ptr<ShaderInput3i> &in, GLint min, GLint max)
{
  addShaderInputi(in->name(), in.get(), min, max);
  in->set_isConstant(GL_FALSE);
}
void FltkApplication::addShaderInput(const ref_ptr<ShaderInput4i> &in, GLint min, GLint max)
{
  addShaderInputi(in->name(), in.get(), min, max);
  in->set_isConstant(GL_FALSE);
}

void FltkApplication::createWidgets(Fl_Pack *parent)
{
}

void FltkApplication::set_windowTitle(const string &windowTitle)
{
  windowTitle_ = windowTitle;
  mainWindow_.label(windowTitle_.c_str());
}

static void _postRedisplay(void *data)
{
  FltkApplication *app = (FltkApplication*) data;
  app->postRedisplay();
}

void FltkApplication::setKeepAspect()
{
  mainWindow_.size_range(
      fltkWidth_/10, fltkHeight_/10,
      fltkWidth_*10, fltkHeight_*10,
      0, 0,
      1);
}

void FltkApplication::setFixedSize()
{
  mainWindow_.size_range(
      fltkWidth_, fltkHeight_,
      fltkWidth_, fltkHeight_,
      0, 0, 0);
}

void FltkApplication::postRedisplay()
{
  fltkWindow_->redraw();
}

void FltkApplication::initGL()
{
  mainWindow_.begin();
  {
    mainWindow_.callback(closeApplicationCallback_, this);

    mainWindowPackH_ = new Fl_Pack(0,0,fltkWidth_,fltkHeight_);
    mainWindowPackH_->type(Fl_Pack::HORIZONTAL);
    mainWindowPackH_->begin();
    {
      mainWindowPackV_ = new Fl_Pack(0,0,fltkWidth_,fltkHeight_);
      mainWindowPackV_->type(Fl_Pack::VERTICAL);
      mainWindowPackV_->begin(); {
        fltkWindow_ = new GLWindow(this,0,0,fltkWidth_,fltkHeight_);
      } mainWindowPackV_->end();
      mainWindowPackV_->resizable(fltkWindow_);
    }
    mainWindowPackH_->end();
    mainWindowPackH_->resizable(mainWindowPackV_);
  }
  mainWindow_.end();
  mainWindow_.resizable(mainWindowPackH_);

  fltkWindow_->show();
  mainWindow_.show();
  fltkWindow_->make_current();
  OGLEApplication::initGL();

  createWidgets(mainWindowPackV_);

  Fl::add_idle(_postRedisplay, this);
}

void FltkApplication::resize(GLuint width, GLuint height)
{
  mainWindow_.size((GLint)width, (GLint)height);
  fltkHeight_ = height;
  fltkWidth_ =  width;
}

void FltkApplication::exitMainLoop(int errorCode)
{
  isApplicationRunning_ = GL_FALSE;
}

void FltkApplication::show()
{
  initGL();
}

void FltkApplication::swapGL()
{
  fltkWindow_->swap_buffers();
}

int FltkApplication::mainLoop()
{
  AnimationManager::get().resume();

  while(Fl::wait() && isApplicationRunning_) {}
  return 0;
}

////////////

FltkApplication::GLWindow::GLWindow(
    FltkApplication *app,
    GLint x, GLint y,
    GLint w, GLint h)
: Fl_Gl_Window(x,y,w,h),
  app_(app)
{
  mode(FL_SINGLE);
  //mode(FL_RGB8);
  //mode(FL_RGB);
}

void FltkApplication::GLWindow::flush()
{
  draw();
}

void FltkApplication::GLWindow::draw()
{
  app_->drawGL();
}

void FltkApplication::GLWindow::resize(int x, int y, int w, int h)
{
  Fl_Gl_Window::resize(x,y,w,h);
  app_->resizeGL(w, h);
}

static int fltkButtonToOgleButton(int button)
{
  switch(button) {
  case FL_MIDDLE_MOUSE:
    return 2;
  case FL_RIGHT_MOUSE:
    return 1;
  case FL_LEFT_MOUSE:
  default:
    return 0;
  }
}
int FltkApplication::GLWindow::handle(int ev)
{
  switch(ev) {
  case FL_KEYDOWN: {
    app_->keyDown(Fl::event_key(), Fl::event_x(),  Fl::event_y());
    return 1;
  }
  case FL_SHORTCUT:
    switch(Fl::event_key()) {
    case FL_Escape:
      app_->exitMainLoop(0);
      return 1;
    default:
      return Fl_Gl_Window::handle(ev);
    }
  case FL_KEYUP: {
    unsigned char key = Fl::event_key();
    if(key == 'f') {
      app_->toggleFullscreen();
    }
    else {
      app_->keyUp(Fl::event_key(), Fl::event_x(), Fl::event_y());
    }
    return 1;
  }
  case FL_PUSH:
  case FL_RELEASE:
    app_->mouseButton(
        fltkButtonToOgleButton(Fl::event_button()),
        (ev==FL_PUSH ? GL_TRUE : GL_FALSE),
        Fl::event_x(),
        Fl::event_y());
    return 1;
  case FL_MOUSEWHEEL:
    app_->mouseButton(
        Fl::event_dy()<0 ? 3 : 4,
        GL_FALSE,
        Fl::event_x(),
        Fl::event_y());
    return 1;
  case FL_MOVE:
  case FL_DRAG:
    app_->mouseMove(
        Fl::event_x(),
        Fl::event_y());
    return 1;
  case FL_NO_EVENT:
  case FL_HIDE:
  case FL_ENTER:
  case FL_LEAVE:
    return Fl_Gl_Window::handle(ev);
  default:
    return Fl_Gl_Window::handle(ev);
  }
}
