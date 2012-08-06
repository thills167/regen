/*
 * image-texture.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _IMAGE_TEXTURE_H_
#define _IMAGE_TEXTURE_H_

#include <stdexcept>

#include <ogle/exceptions/io-exceptions.h>
#include <ogle/gl-types/texture.h>

class ImageError : public runtime_error
{
public:
  ImageError(const string &message)
  : runtime_error(message)
  {
  }
};

/**
 * A texture that can load data from common image file formats.
 * The formats supported depend on the used library DevIL.
 * @see http://openil.sourceforge.net/features.php
 */
class ImageTexture : public Texture {
public:
  /**
   * Default constructor, does not load image data.
   */
  ImageTexture();
  /**
   * Constructor that loads image data.
   */
  ImageTexture(const string &file,
      int width, int height, int depth,
      bool useMipmap=true)
  throw (ImageError, FileNotFoundException);
  ImageTexture(const string &file, bool useMipmap=true)
  throw (ImageError, FileNotFoundException);
  ~ImageTexture();

  /**
   * Loads image data from file and uploads it to GL.
   * If mipmaps were generated before then mipmaps are generated
   * for the new image data too.
   */
  void set_file(const string &file,
      int width=0, int height=0, int depth=0,
      bool useMipmap=true,
      GLenum mimpmapFlag=GL_DONT_CARE) throw (ImageError, FileNotFoundException);

  virtual void texSubImage(GLubyte *subData, int layer=0) const;
  virtual void texImage() const;

  virtual string samplerType() const {
    return samplerType_;
  }

protected:
  static bool devilInitialized_;
  string samplerType_;
  int depth_;
private:
  ImageTexture(const ImageTexture&);
  void init();
};

#endif /* _IMAGE_TEXTURE_H_ */
