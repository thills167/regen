/*
 * video-player-widget.cpp
 *
 *  Created on: 01.01.2013
 *      Author: daniel
 */

#include <iostream>
using namespace std;

#include "video-player-widget.h"

#include <QtGui/QFileDialog>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QFont>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtCore/QList>
#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtCore/QDirIterator>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>

extern "C" {
  #include <libavformat/avformat.h>
}

#include <ogle/utility/string-util.h>

static QString formatTime(GLfloat elapsedSeconds)
{
  GLuint seconds = (GLuint)elapsedSeconds;
  GLuint minutes = seconds/60;
  seconds = seconds%60;
  string label = FORMAT_STRING(
      (minutes<10 ? "0" : "") << minutes <<
      ":" <<
      (seconds<10 ? "0" : "") << seconds);
  return QString(label.c_str());
}
static void hideLayout(QLayout *layout)
{
  for(GLint i=0; i<layout->count(); ++i) {
    QLayoutItem *item = layout->itemAt(i);
    if(item->widget()) { item->widget()->hide(); }
    if(item->layout()) { hideLayout(item->layout()); }
  }
}
static void showLayout(QLayout *layout)
{
  for(GLint i=0; i<layout->count(); ++i) {
    QLayoutItem *item = layout->itemAt(i);
    if(item->widget()) { item->widget()->show(); }
    if(item->layout()) { showLayout(item->layout()); }
  }
}
static GLboolean isRegularFile(const string &f)
{
  boost::filesystem::path p(f);
  return boost::filesystem::is_regular_file(p);
}
static GLboolean isDirectory(const string &f)
{
  boost::filesystem::path p(f);
  return boost::filesystem::is_directory(p);
}

////////////
////////////

VideoPlayerWidget::VideoPlayerWidget(QtOGLEApplication *app)
: QMainWindow(),
  EventCallable(),
  app_(app),
  gain_(1.0f),
  elapsedTimer_(this),
  activePlaylistRow_(NULL)
{
  setMouseTracking(true);
  setAcceptDrops(true);

  vid_ = ref_ptr<VideoTexture>::manage(new VideoTexture);

  ui_.setupUi(this);
  ui_.glWidgetLayout->addWidget(&app_->glWidget(), 0,0,1,1);
  ui_.repeatButton->click();

  QList<int> initialSizes;
  initialSizes.append(1);
  initialSizes.append(0);
  ui_.splitter->setSizes(initialSizes);

  // update elapsed time label
  elapsedTimer_.setInterval(1000);
  connect(&elapsedTimer_, SIGNAL(timeout()), this, SLOT(updateElapsedTime()));
  elapsedTimer_.start();

  srand(time(NULL));
}

ref_ptr<Texture> VideoPlayerWidget::texture() const
{
  return ref_ptr<Texture>::cast(vid_);
}
const ref_ptr<VideoTexture>& VideoPlayerWidget::video() const
{
  return vid_;
}

void VideoPlayerWidget::call(EventObject *ev, void *data)
{
  OGLEApplication::KeyEvent *keyEv = static_cast<OGLEApplication::KeyEvent*>(data);
  if(keyEv != NULL) {
    if(!keyEv->isUp) { return; }

    if(keyEv->keyValue == Qt::Key_Left) {
      vid_->seekBackward(10.0);
    }
    else if(keyEv->keyValue == Qt::Key_Right) {
      vid_->seekForward(10.0);
    }
    else if(keyEv->keyValue == Qt::Key_Up) {
      vid_->seekForward(60.0);
    }
    else if(keyEv->keyValue == Qt::Key_Down) {
      vid_->seekBackward(60.0);
    }
    else if(keyEv->key == 'f' || keyEv->key == 'F') {
      toggleFullscreen();
    }
    else if(keyEv->key == ' ') {
      togglePlayVideo();
    }
    else if(keyEv->key == 'o' || keyEv->key == 'O') {
      openVideoFile();
    }
  }

  OGLEApplication::ButtonEvent *mouseEv = static_cast<OGLEApplication::ButtonEvent*>(data);
  if(mouseEv != NULL) {
    if(mouseEv->isDoubleClick && mouseEv->button==1) {
      toggleFullscreen();
    }
  }
}

void VideoPlayerWidget::activatePlaylistRow(int row)
{
  QFont font;
  int activeRow = (activePlaylistRow_ != NULL ? activePlaylistRow_->row() : -1);
  if(activeRow!=-1) {
    font.setBold(false);
    ui_.playlistTable->item(activeRow,0)->setFont(font);
    ui_.playlistTable->item(activeRow,1)->setFont(font);
  }
  font.setBold(true);
  if(row != -1) {
    activePlaylistRow_ = ui_.playlistTable->item(row,0);
    ui_.playlistTable->item(row,0)->setFont(font);
    ui_.playlistTable->item(row,1)->setFont(font);
  } else {
    activePlaylistRow_ = NULL;
  }
}

void VideoPlayerWidget::changeVolume(int val)
{
  gain_ = ((float)val)/100.0f;
  if(vid_->audioSource().get()) {
    vid_->audioSource()->set_gain(gain_);
  }
  string label = FORMAT_STRING(val << "%");
  ui_.volumeLabel->setText(QString(label.c_str()));
}

void VideoPlayerWidget::updateElapsedTime()
{
  GLfloat elapsed = vid_->elapsedSeconds();
  ui_.progressLabel->setText(formatTime(elapsed));
  ui_.progressSlider->blockSignals(true);
  ui_.progressSlider->setValue((int) (100000.0f*elapsed/vid_->totalSeconds()));
  ui_.progressSlider->blockSignals(false);
  if(vid_->isCompleted()) {
    nextVideo();
    vid_->play();
  }
}

void VideoPlayerWidget::setVideoFile(const string &filePath)
{
  boost::filesystem::path bdir(filePath.c_str());
  app_->set_windowTitle(bdir.filename().c_str());

  vid_->set_file(filePath);
  vid_->play();
  if(vid_->audioSource().get()) {
    vid_->audioSource()->set_gain(gain_);
  }
  ui_.playButton->setIcon(QIcon::fromTheme("media-playback-pause"));
  ui_.movieLengthLabel->setText(formatTime(vid_->totalSeconds()));
  updateElapsedTime();
  updateSize();
}

int VideoPlayerWidget::addPlaylistItem(const string &filePath)
{
  // load information about the video file.
  // if libav cannot open the file skip it
  AVFormatContext *formatCtx = NULL;
  if(avformat_open_input(&formatCtx, filePath.c_str(), NULL, NULL) != 0) {
    return -1;
  }
  if(avformat_find_stream_info(formatCtx, NULL)<0) {
    return -1;
  }
  GLdouble numSeconds = formatCtx->duration/(GLdouble)AV_TIME_BASE;
  avformat_close_input(&formatCtx);

  std::string filename = boost::filesystem::path(filePath).stem().string();
  int row = ui_.playlistTable->rowCount();
  ui_.playlistTable->insertRow(row);

  QTableWidgetItem *fileNameItem = new QTableWidgetItem;
  fileNameItem->setText(filename.c_str());
  fileNameItem->setTextAlignment(Qt::AlignLeft);
  fileNameItem->setData(1, QVariant(filePath.c_str()));
  ui_.playlistTable->setItem(row, 0, fileNameItem);

  QTableWidgetItem *lengthItem = new QTableWidgetItem;
  lengthItem->setText(formatTime(numSeconds));
  lengthItem->setTextAlignment(Qt::AlignLeft);
  lengthItem->setData(1, QVariant(filePath.c_str()));
  ui_.playlistTable->setItem(row, 1, lengthItem);

  return row;
}

void VideoPlayerWidget::addLocalPath(const string &filePath)
{
  if(isRegularFile(filePath))
  {
    addPlaylistItem(filePath);
  }
  else if(isDirectory(filePath))
  {
    QDirIterator it(filePath.c_str(), QDirIterator::Subdirectories);
    QStringList files;
    while (it.hasNext()) {
      string childPath = it.next().toStdString();
      if(isRegularFile(childPath)) {
        files.append(childPath.c_str());
      }
    }
    files.sort();
    for(QStringList::iterator it=files.begin(); it!=files.end(); ++it)
    {
      addPlaylistItem(it->toStdString());
    }
  }
}

//////////////////////////////
//////// Slots
//////////////////////////////

void VideoPlayerWidget::updateSize()
{
  GLfloat widgetRatio = ui_.blackBackground->width()/(GLfloat)ui_.blackBackground->height();
  GLfloat videoRatio = vid_->width()/(GLfloat)vid_->height();
  GLint w,h;
  if(widgetRatio>videoRatio) {
    w = (GLint)(ui_.blackBackground->height()*videoRatio);
    h = ui_.blackBackground->height();
  }
  else {
    w = ui_.blackBackground->width();
    h = (GLint)(ui_.blackBackground->width()/videoRatio);
  }
  if(w%2 != 0) { w-=1; }
  if(h%2 != 0) { h-=1; }
  ui_.glWidget->setMinimumSize(QSize(max(2,w),max(2,h)));
}

void VideoPlayerWidget::toggleRepeat(bool v)
{
  // nothing to do here for now...
}
void VideoPlayerWidget::toggleShuffle(bool v)
{
  // nothing to do here for now...
}

void VideoPlayerWidget::toggleFullscreen()
{
  QLayoutItem *item;
  while ((item = ui_.mainLayout->takeAt(0)) != 0) {
    ui_.mainLayout->removeItem(item);
  }
  ui_.progressLayout->setParent(NULL);
  ui_.buttonBarLayout->setParent(NULL);
  ui_.blackBackground->setParent(NULL);
  ui_.playlistTable->setParent(NULL);

  if(isFullScreen()) {
    showNormal();

    ui_.mainLayout->addWidget(ui_.splitter);
    ui_.splitter->addWidget(ui_.blackBackground);
    ui_.splitter->addWidget(ui_.playlistTable);
    ui_.mainLayout->addLayout(ui_.progressLayout);
    ui_.mainLayout->addLayout(ui_.buttonBarLayout);
    ui_.menubar->show();
    ui_.statusbar->show();
    showLayout(ui_.progressLayout);
    showLayout(ui_.buttonBarLayout);
  }
  else {
    ui_.mainLayout->addWidget(ui_.blackBackground);
    ui_.menubar->hide();
    ui_.statusbar->hide();
    hideLayout(ui_.progressLayout);
    hideLayout(ui_.buttonBarLayout);

    showFullScreen();
  }
}

void VideoPlayerWidget::openVideoFile()
{
  QWidget *parent = NULL;
  QFileDialog dialog(parent);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setFilter("Videos (*.avi *.mpg);;All files (*.*)");
  dialog.setViewMode(QFileDialog::Detail);
  if(!dialog.exec()) { return; }

  QStringList fileNames = dialog.selectedFiles();
  string filePath = fileNames.first().toStdString();

  int row = addPlaylistItem(filePath);
  if(row==-1) {
    WARN_LOG("Failed to open video file at " << filePath);
  } else {
    setVideoFile(filePath);
    activatePlaylistRow(row);
  }
}

void VideoPlayerWidget::playlistActivated(QTableWidgetItem *item)
{
  setVideoFile(item->data(1).toString().toStdString());
  activatePlaylistRow(item->row());
}

void VideoPlayerWidget::nextVideo()
{
  int row;
  int activeRow = (activePlaylistRow_ != NULL ? activePlaylistRow_->row() : -1);

  if(ui_.shuffleButton->isChecked()) {
    row = rand()%ui_.playlistTable->rowCount();
  }
  else {
    row = activeRow+1;
    if(row >= ui_.playlistTable->rowCount()) {
      row = (ui_.repeatButton->isChecked() ? 0 : -1);
    }
  }

  // update video file
  if(row==-1) {
    vid_->stop();
  }
  else {
    QTableWidgetItem *item = ui_.playlistTable->item(row,0);
    setVideoFile(item->data(1).toString().toStdString());
  }
  // make playlist item bold
  activatePlaylistRow(row);
}

void VideoPlayerWidget::previousVideo()
{
  int row;
  int activeRow = (activePlaylistRow_ != NULL ? activePlaylistRow_->row() : -1);

  if(ui_.shuffleButton->isChecked()) {
    row = rand()%ui_.playlistTable->rowCount();
  }
  else {
    row = activeRow-1;
    if(row < 0) {
      row = (ui_.repeatButton->isChecked() ? ui_.playlistTable->rowCount()-1 : -1);
    }
  }

  // update video file
  if(row==-1) {
    vid_->stop();
  }
  else {
    QTableWidgetItem *item = ui_.playlistTable->item(row,0);
    setVideoFile(item->data(1).toString().toStdString());
  }
  // make playlist item bold
  activatePlaylistRow(row);
}

void VideoPlayerWidget::seekVideo(int val)
{
  vid_->seekTo(((float)val)/100000.0f);
}

void VideoPlayerWidget::stopVideo()
{
  if(vid_->isFileSet()) {
    vid_->stop();
    ui_.playButton->setIcon(QIcon::fromTheme("media-playback-start"));
  }
}

void VideoPlayerWidget::togglePlayVideo()
{
  if(vid_->isFileSet()) {
    vid_->togglePlay();
    ui_.playButton->setIcon(QIcon::fromTheme(vid_->isPlaying() ?
        "media-playback-pause" : "media-playback-start"));
  }
  else {
    nextVideo();
  }
}

//////////////////////////////
//////// Qt Events
//////////////////////////////

void VideoPlayerWidget::resizeEvent(QResizeEvent * event)
{
  updateSize();
}

void VideoPlayerWidget::keyPressEvent(QKeyEvent* event)
{
  app_->keyDown(event->key(),app_->mouseX(),app_->mouseY());
}

void VideoPlayerWidget::keyReleaseEvent(QKeyEvent *event)
{
  app_->keyUp(event->key(),app_->mouseX(),app_->mouseY());
}

void VideoPlayerWidget::dragEnterEvent(QDragEnterEvent *event)
{
  if(event->mimeData()->hasFormat("text/uri-list")) {
    event->acceptProposedAction();
  }
}

void VideoPlayerWidget::dropEvent(QDropEvent *event)
{
  QList<QUrl> uris = event->mimeData()->urls();
  for(QList<QUrl>::iterator it=uris.begin(); it!=uris.end(); ++it)
  {
    QUrl &url = *it;
    if(url.isLocalFile()) {
      addLocalPath(url.toLocalFile().toStdString());
    }
  }
}
