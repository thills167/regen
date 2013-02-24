/*
 * glsl-io-processor.h
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#ifndef GLSL_IO_PROCESSOR_H_
#define GLSL_IO_PROCESSOR_H_

#include <boost/regex.hpp>
#include <ogle/gl-types/shader-input.h>

#include <iostream>
#include <list>
#include <string>
using namespace std;

/**
 * IO Varying used in Shader code.
 */
struct GLSLInputOutput {
  // the layout qualifier
  string layout;
  string interpolation;
  // the IO type (in/out/const/uniform)
  string ioType;
  // the data type as used in the Shader
  string dataType;
  // the name as used in the Shader
  string name;
  // number of array elements (name[#N])
  string numElements;
  // for constants this defines the value
  string value;
  GLSLInputOutput();
  GLSLInputOutput(const GLSLInputOutput&);
  string declaration(GLenum stage);
};

/**
 * A GLSL processor that modifies the IO behavior of the Shader code.
 * Specified ShaderInput can change the IO Type of declaration in the Shader.
 * This can be used to change declarations to constants, uniforms, attributes
 * or instanced attributes.
 * The declarations in the Shader supposed to use 'in_' and 'out_' prefix for
 * varyings in all stages, this processor ensures name matching by inserting
 * defines with a matching prefix above the declarations.
 * This processor also can automatically generate Shader code to transfer a
 * varying between stages. For each input of the following stage output
 * is generated if it was missing. Shaders must declare '#define HANDLE_IO'
 * somewhere above the main function and call 'HANDLE_IO(0)' in the main function
 * for this wo work.
 */
class GLSLInputOutputProcessor {
public:
  /**
   * Truncate the one of the known prefixes from string
   * if string matches any prefix.
   */
  static string getNameWithoutPrefix(const string &name);

  /**
   * @param in The input stream providing GLSL code
   * @param stage The shader stage to pre process
   * @param nextStage The following Shader stage
   * @param nextStageInputs used to automatically genrate IO varyings
   * @param specifiedInput used to modify declarations
   */
  GLSLInputOutputProcessor(
      istream &in,
      GLenum stage,
      GLenum nextStage,
      const map<string,GLSLInputOutput> &nextStageInputs,
      const map<string, ref_ptr<ShaderInput> > &specifiedInput);

  /**
   * Outputs collected while processing the input stream.
   */
  map<string,GLSLInputOutput>& outputs();
  /**
   * Inputs collected while processing the input stream.
   */
  map<string,GLSLInputOutput>& inputs();

  /**
   * Read a single line from input stream.
   */
  bool getline(string &line);

  /**
   * Read input stream until EOF reached.
   */
  void preProcess(ostream &out);

protected:
  istream &in_;
  list<string> lineQueue_;
  const map<string,GLSLInputOutput> &nextStageInputs_;
  map<string,GLSLInputOutput> outputs_;
  map<string,GLSLInputOutput> inputs_;

  GLboolean wasEmpty_;
  GLenum stage_;
  GLenum nextStage_;
  const map<string, ref_ptr<ShaderInput> > &specifiedInput_;

  void defineHandleIO();
  void parseValue(string &v, string &val);
  void parseArray(string &v, string &numElements);
};

#endif /* GLSL_IO_PROCESSOR_H_ */
