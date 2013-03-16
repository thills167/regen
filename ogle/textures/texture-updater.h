
#ifndef TEXTURE_UPDATER_H_
#define TEXTURE_UPDATER_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <map>
using namespace std;

#include <ogle/animations/animation.h>
#include <ogle/textures/texture-update-operation.h>
#include <ogle/gl-types/fbo.h>

#include <ogle/algebra/vector.h>
#include <ogle/meshes/mesh-state.h>

namespace ogle {
/**
 * \brief Executes a sequence of operations for updating a
 * texture.
 */
class TextureUpdater : public Animation
{
public:
  TextureUpdater();
  ~TextureUpdater();

  /**
   * Name identifier.
   */
  const string& name() const;

  /**
   * The desired animation framerate.
   */
  GLint framerate() const;
  /**
   * The desired animation framerate.
   */
  void set_framerate(GLint framerate);

  /**
   * Serializing.
   */
  friend void operator>>(istream &in, TextureUpdater &v);
  /**
   * Serializing.
   */
  void parseConfig(const map<string,string> &cfg);

  //////////

  /**
   * Add a named buffer to the list of known buffers.
   */
  void addBuffer(SimpleRenderTarget *buffer);
  /**
   * Get buffer by name.
   */
  SimpleRenderTarget* getBuffer(const string &name);

  //////////

  /**
   * Adds an operation to the sequence of operations
   * to be executed.
   */
  void addOperation(
      TextureUpdateOperation *operation,
      GLboolean isInitial=GL_FALSE);
  /**
   * Removes an previously added operation.
   */
  void removeOperation(TextureUpdateOperation *operation);

  /**
   * @return sequence of initial operations.
   */
  list<TextureUpdateOperation*>& initialOperations();
  /**
   * @return sequence of operations.
   */
  list<TextureUpdateOperation*>& operations();
  /**
   * @return map of buffers used by this updater.
   */
  map<string,SimpleRenderTarget*>& buffers();

  /**
   * Execute sequence of operations.
   */
  void executeOperations(RenderState *rs,
      const list<TextureUpdateOperation*>&);

  // override
  void glAnimate(RenderState *rs, GLdouble dt);

protected:
  string name_;
  GLdouble dt_;

  GLint framerate_;

  list<TextureUpdateOperation*> operations_;
  list<TextureUpdateOperation*> initialOperations_;
  map<string,SimpleRenderTarget*> buffers_;

private:
  TextureUpdater(const TextureUpdater&);
};
ostream& operator<<(ostream& os, const TextureUpdater& v);

} // end ogle namespace

#endif /* TEXTURE_UPDATER_H_ */
