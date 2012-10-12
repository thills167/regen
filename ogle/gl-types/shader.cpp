/*
 * shader.cpp
 *
 *  Created on: 26.03.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "shader.h"
#include "texture.h"
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>

Shader::Shader(const map<GLenum, string> &shaderCodes)
: numInstances_(1),
  shaderCodes_(shaderCodes),
  isLineShader_(GL_FALSE),
  isPointShader_(GL_FALSE)
{
  id_ = glCreateProgram();
}
Shader::~Shader()
{
  for(map<GLenum, GLuint>::const_iterator
      it = shaders_.begin(); it != shaders_.end(); ++it)
  {
    glDeleteShader(it->second);
  }
  glDeleteProgram(id_);
}

GLboolean Shader::isPointShader() const
{
  return isPointShader_;
}
void Shader::set_isPointShader(GLboolean isPointShader)
{
  isPointShader_ = isPointShader;
}

GLboolean Shader::isLineShader() const
{
  return isLineShader_;
}
void Shader::set_isLineShader(GLboolean isLineShader)
{
  isLineShader_ = isLineShader;
}

bool Shader::hasShader(GLenum stage) const
{
  return shaders_.count(stage)>0;
}
const string& Shader::shaderCode(GLenum stage) const
{
  map<GLenum, string>::const_iterator it = shaderCodes_.find(stage);
  if(it!=shaderCodes_.end()) {
    return it->second;
  } else {
    static const string empty = "";
    return empty;
  }
}

const GLuint& Shader::shader(GLenum stage) const
{
  map<GLenum, GLuint>::const_iterator it = shaders_.find(stage);
  if(it!=shaders_.end()) {
    return it->second;
  } else {
    static const GLuint empty = 0;
    return empty;
  }
}
void Shader::setShaders(const map<GLenum, GLuint> &shaders)
{
  for(map<GLenum, GLuint>::const_iterator
      it = shaders_.begin(); it != shaders_.end(); ++it)
  {
    glAttachShader(id_, 0);
    glDeleteShader(it->second);
  }
  shaders_.clear();
  for(map<GLenum, GLuint>::const_iterator
      it = shaders.begin(); it != shaders.end(); ++it)
  {
    glAttachShader(id_, it->second);
    shaders_[it->first] = it->second;
  }
}

const map<string, ref_ptr<ShaderInput> >& Shader::inputs() const
{
  return inputs_;
}

GLint Shader::id() const
{
  return id_;
}

bool Shader::compile()
{
  for(map<GLenum, string>::const_iterator
      it = shaderCodes_.begin(); it != shaderCodes_.end(); ++it)
  {
    const char* source = it->second.c_str();
    GLuint shaderStage = glCreateShader(it->first);
    GLint length = -1;
    GLint status;

    glShaderSource(shaderStage, 1, &source, &length);
    glCompileShader(shaderStage);

    glGetShaderiv(shaderStage, GL_COMPILE_STATUS, &status);
    if (!status) {
      printLog(shaderStage, it->first, source, false);
      glDeleteShader(shaderStage);
      return false;
    }
    //if(Logging::verbosity() > Logging::_) {
    //  printLog(shaderStage, it->first, source, true);
    //}

    glAttachShader(id_, shaderStage);
    shaders_[it->first] = shaderStage;
  }
  return true;
}

bool Shader::link()
{
  glLinkProgram(id_);
  GLint status;
  glGetProgramiv(id_, GL_LINK_STATUS,  &status);
  if(status == GL_FALSE) {
    printLog(id_, GL_VERTEX_SHADER, NULL, false);
    handleGLError("after glLinkProgram");
    return false;
  } else {
    return true;
  }
}

void Shader::printLog(
    GLuint shader,
    GLenum shaderType,
    const char *shaderCode,
    GLboolean success)
{
  Logging::LogLevel logLevel;
  if(shaderCode != NULL) {
    const char* shaderName;
    switch(shaderType) {
    case GL_VERTEX_SHADER:   shaderName = "Vertex"; break;
    case GL_GEOMETRY_SHADER: shaderName = "Geometry"; break;
    case GL_FRAGMENT_SHADER: shaderName = "Fragment"; break;
    case GL_TESS_CONTROL_SHADER: shaderName = "TessControl"; break;
    case GL_TESS_EVALUATION_SHADER: shaderName = "TessEval"; break;
    }

    if(success) {
      logLevel = Logging::INFO;
      LOG_MESSAGE(logLevel, shaderName << " Shader compiled successfully!");
    } else {
      logLevel = Logging::ERROR;
      LOG_MESSAGE(logLevel, shaderName << " Shader failed to compile!");
    }
  } else {
    logLevel = Logging::ERROR;
    LOG_MESSAGE(logLevel, "Shader failed to link!");
  }

  if(shaderCode != NULL) {
    vector<string> codeLines;
    boost::split(codeLines, shaderCode, boost::is_any_of("\n"));
    for(GLuint i=0; i<codeLines.size(); ++i) {
      LOG_MESSAGE(logLevel,
          setw(3) << i << setw(0) << " " << codeLines[i]);
    }
  }

  int length;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
  if(length>0) {
    char log[length];
    glGetShaderInfoLog(shader, length, NULL, log);
    LOG_MESSAGE(logLevel, "shader info: " << log);
  } else {
    LOG_MESSAGE(logLevel, "shader info: empty");
  }

  if(length > 0) {
  }
}

void Shader::setupOutputs(
    const list<ShaderOutput> &outputs)
{
  for(list<ShaderOutput>::const_iterator
      it=outputs.begin(); it!=outputs.end(); ++it)
  {
    glBindFragDataLocation(
        id(),
        it->colorAttachment-GL_COLOR_ATTACHMENT0,
        it->name.c_str());
  }
  if(outputs.empty()) {
    glBindFragDataLocation(
        id(), 0, "defaultColorOutput");
  }
}

void Shader::setupTransformFeedback(
    const list<string> &tfAtts,
    GLenum attributeLayout)
{
  if(tfAtts.size()>0) {
    // specify the transform feedback output names
    vector<const char*> names(tfAtts.size());
    GLuint i=0;
    for(list<string>::const_iterator
        it=tfAtts.begin(); it!=tfAtts.end(); ++it)
    {
      names[i++] = it->c_str();
    }
    glTransformFeedbackVaryings(
        id(),
        tfAtts.size(),
        names.data(),
        attributeLayout);
  }
}

void Shader::setupLocations(
    const set<string> &attributeNames,
    const set<string> &uniformNames)
{
  for(set<string>::const_iterator
      it=attributeNames.begin(); it!=attributeNames.end(); ++it)
  {
    string attName;
    string attNameInShader = *it;
    if (hasPrefix(attNameInShader, "vs_")) {
      attName = truncPrefix(attNameInShader, "vs_");
    } else if (hasPrefix(attNameInShader, "in_")) {
      attName = truncPrefix(attNameInShader, "in_");
      attNameInShader = FORMAT_STRING("vs_" << attName);
    } else {
      attName = attNameInShader;
      attNameInShader = FORMAT_STRING("vs_" << attName);
    }
    GLint loc = glGetAttribLocation(id(), attNameInShader.c_str());
    if(loc!=-1) {
      attributeLocations_[FORMAT_STRING("in_"<<attName)] = loc;
      attributeLocations_[attName] = loc;
      attributeLocations_[*it] = loc;
    }
  }

  for(set<string>::const_iterator
      it=uniformNames.begin(); it!=uniformNames.end(); ++it)
  {
    string uniName;
    string uniNameInShader = *it;
    if (hasPrefix(uniNameInShader, "u_"))
    {
      uniName = truncPrefix(uniNameInShader, "u_");
    }
    else if (hasPrefix(uniNameInShader, "in_"))
    {
      uniName = truncPrefix(uniNameInShader, "in_");
      uniNameInShader = FORMAT_STRING("u_" << uniName);
    }
    else
    {
      uniName = uniNameInShader;
      uniNameInShader = FORMAT_STRING("u_" << uniName);
    }

    GLint loc = glGetUniformLocation(id(), uniNameInShader.c_str());
    if(loc==-1) {
      loc = glGetUniformLocation(id(), uniName.c_str());
    }
    if(loc==-1) {
      loc = glGetUniformLocation(id(), (*it).c_str());
    }
    if(loc!=-1) {
      uniformLocations_[FORMAT_STRING("in_"<<uniName)] = loc;
      uniformLocations_[uniName] = loc;
      uniformLocations_[uniNameInShader] = loc;
      uniformLocations_[*it] = loc;
    }
  }
}

GLuint Shader::numInstances() const
{
  return numInstances_;
}

void Shader::setupInputs(
    const map<string, ref_ptr<ShaderInput> > &inputs)
{
  numInstances_ = 1;
  for(map<string, ref_ptr<ShaderInput> >::const_iterator
      it=inputs.begin(); it!=inputs.end(); ++it)
  {
    const ref_ptr<ShaderInput> &in = it->second;
    if(in->isVertexAttribute())
    {
      if(in->numInstances()>1)
      {
        if(numInstances_==1)
        {
          numInstances_ = in->numInstances();
        }
        else if(numInstances_ != in->numInstances())
        {
          WARN_LOG("incompatible number of instance for " << in->name() << "."
              << " Excpected is '" << numInstances_ << "' but actual value is '"
              << in->numInstances() << "'.")
        }
      }

      map<string,GLint>::iterator needle = attributeLocations_.find(in->name());
      if(needle!=attributeLocations_.end()) {
        attributes_.push_back(ShaderInputLocation(in,needle->second));
      }
    }
    else if (!in->isConstant()) {
      map<string,GLint>::iterator needle = uniformLocations_.find(in->name());
      if(needle!=uniformLocations_.end()) {
        uniforms_.push_back(ShaderInputLocation(in,needle->second));
      }
    }
  }
  inputs_ = inputs;
}

void Shader::applyInputs()
{
  for(list<ShaderInputLocation>::iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    it->input->enableAttribute( it->location );
  }
  for(list<ShaderInputLocation>::iterator
      it=uniforms_.begin(); it!=uniforms_.end(); ++it)
  {
    it->input->enableUniform( it->location );
  }
}

void Shader::applyTexture(const ShaderTexture &d)
{
  map<string,GLint>::iterator needle = uniformLocations_.find(d.tex->name());
  if(needle!=uniformLocations_.end()) {
    glUniform1i( needle->second, d.texUnit );
  }
}

void Shader::applyAttribute(const ShaderInput *input)
{
  map<string,GLint>::iterator needle = attributeLocations_.find(input->name());
  if(needle!=attributeLocations_.end()) {
    input->enableAttribute( needle->second );
  }
}

void Shader::applyUniform(const ShaderInput *input)
{
  map<string,GLint>::iterator needle = uniformLocations_.find(input->name());
  if(needle!=uniformLocations_.end()) {
    input->enableUniform( needle->second );
  }
}
