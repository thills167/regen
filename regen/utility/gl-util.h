/*
 * gl-util.h
 *
 *  Created on: 20.03.2011
 *      Author: daniel
 */

#ifndef __GL_UTIL__
#define __GL_UTIL__

#include <sstream>
using namespace std;

#include <GL/glew.h>
#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/config.h>

namespace regen {
/**
 * Log the GL error state.
 */
#ifdef REGEN_DEBUG_BUILD
#define GL_ERROR_LOG() ERROR_LOG( getGLError() )
#else
#define GL_ERROR_LOG()
#endif
/**
 * Log the FBO error state.
 */
#ifdef REGEN_DEBUG_BUILD
#define FBO_ERROR_LOG() ERROR_LOG( getFBOError() )
#else
#define FBO_ERROR_LOG()
#endif

/**
 * Query GL error state.
 */
#ifdef REGEN_DEBUG_BUILD
string getGLError();
#else
#define getGLError()
#endif
/**
 * Query FBO error state.
 */
#ifdef REGEN_DEBUG_BUILD
string getFBOError(GLenum target);
#else
#define getFBOError(t)
#endif

/**
 * Query a GL query result.
 */
GLuint getGLQueryResult(GLuint query);
/**
 * Query a GL integer attribute.
 */
GLint getGLInteger(GLenum e);
/**
 * Query a GL float attribute.
 */
GLfloat getGLFloat(GLenum e);

} // namespace

#endif /* __GL_UTIL__ */
