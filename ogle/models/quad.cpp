/*
 * Quad.cpp
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#include "quad.h"

UnitQuad::UnitQuad(const Config &cfg)
: MeshState(GL_QUADS)
{
  updateAttributes(cfg);
}

UnitQuad::Config::Config()
: posScale(Vec3f(1.0f)),
  rotation(Vec3f(0.0f)),
  translation(Vec3f(0.0f)),
  texcoScale(Vec2f(1.0f)),
  isTexcoRequired(true),
  isNormalRequired(true),
  isTangentRequired(false),
  centerAtOrigin(false),
  levelOfDetail(0)
{
}

Vec4f getFaceTangent(
    const Vec3f *vertices,
    const Vec2f *texco,
    const Vec3f *normals,
    GLuint numFaceVertices)
{
  Vec3f tangent, binormal;

  // calculate vertex and uv edges
  Vec3f edge1 = vertices[1] - vertices[0];
  Vec3f edge2 = vertices[numFaceVertices-1] - vertices[0];
  Vec2f texEdge1 = texco[1] - texco[0];
  Vec2f texEdge2 = texco[numFaceVertices-1] - texco[0];
  GLfloat det = texEdge1.x * texEdge2.y - texEdge2.x * texEdge1.y;

  if(abs(det) < 0.00001) {
    tangent  = Vec3f( 1.0, 0.0, 0.0 );
    binormal  = Vec3f( 0.0, 1.0, 0.0 );
  } else {
    det = 1.0 / det;
    tangent = Vec3f(
      (texEdge2.y * edge1.x - texEdge1.y * edge2.x),
      (texEdge2.y * edge1.y - texEdge1.y * edge2.y),
      (texEdge2.y * edge1.z - texEdge1.y * edge2.z)
    ) * det;
    binormal = Vec3f(
      (-texEdge2.x * edge1.x + texEdge1.x * edge2.x),
      (-texEdge2.x * edge1.y + texEdge1.x * edge2.y),
      (-texEdge2.x * edge1.z + texEdge1.x * edge2.z)
    ) * det;
  }

  // Gram-Schmidt orthogonalize tangent with normal.
  const Vec3f &normal = normals[0];
  tangent -= normal * dot(normal, tangent);
  normalize(tangent);

  // Calculate the handedness of the local tangent space.
  if(dot( cross(normal, tangent),  binormal ) < 0.0) {
    return Vec4f(tangent.x, tangent.y, tangent.z, -1.0);
  } else {
    return Vec4f(tangent.x, tangent.y, tangent.z, 1.0);
  }
}

void UnitQuad::updateAttributes(Config cfg)
{
  Mat4f rotMat = xyzRotationMatrix(cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);
  GLuint numQuads = pow(4, cfg.levelOfDetail);
  GLuint numQuadsSide = sqrt(numQuads);
  GLfloat quadSize = 1.0/numQuadsSide;

  if(cfg.isTangentRequired) {
    cfg.isNormalRequired = true;
    cfg.isTexcoRequired = true;
  }

  // allocate attributes
  ref_ptr<PositionShaderInput> pos = ref_ptr<PositionShaderInput>::manage(new PositionShaderInput);
  ref_ptr<ShaderInput> nor, tan, texco;
  pos->setVertexData(numQuads*4);
  if(cfg.isNormalRequired) {
    nor = ref_ptr<ShaderInput>::manage(new NormalShaderInput);
    nor->setVertexData(numQuads*4);
  }
  if(cfg.isTexcoRequired) {
    texco = ref_ptr<ShaderInput>::manage(new TexcoShaderInput( 0, 2 ));
    texco->setVertexData(numQuads*4);
  }
  if(cfg.isTangentRequired) {
    tan = ref_ptr<ShaderInput>::manage(new TangentShaderInput);
    tan->setVertexData(numQuads*4);
  }

  Vec3f startPos;
  if(cfg.centerAtOrigin) {
    startPos = Vec3f(-cfg.posScale.x*0.5f, 0.0f, -cfg.posScale.z*0.5f);
  } else {
    startPos = Vec3f(0.0f, 0.0f, 0.0f);
  }
  Vec2f texcoPos(0.0f, 1.0f);
  Vec3f curPos(startPos.x, 0.0f, 0.0f);
  GLuint vertexIndex = 0;

  for(GLuint x=0; x<numQuadsSide; ++x)
  {
    texcoPos.y = 1.0f;
    curPos.z = startPos.z;

    for(GLuint z=0; z<numQuadsSide; ++z)
    {
#define TRANSFORM(x) (transformVec3(rotMat, cfg.posScale*x + curPos) + cfg.translation)
      pos->setVertex3f(vertexIndex + 0, TRANSFORM(Vec3f(0.0,0.0,0.0)));
      pos->setVertex3f(vertexIndex + 1, TRANSFORM(Vec3f(quadSize,0.0,0.0)));
      pos->setVertex3f(vertexIndex + 2, TRANSFORM(Vec3f(quadSize,0.0,quadSize)));
      pos->setVertex3f(vertexIndex + 3, TRANSFORM(Vec3f(0.0,0.0,quadSize)));
#undef TRANSFORM

      if(cfg.isNormalRequired)
      {
#define TRANSFORM(x) transformVec3(rotMat,x)
        nor->setVertex3f(vertexIndex + 0, TRANSFORM(Vec3f(0.0,-1.0,0.0)));
        nor->setVertex3f(vertexIndex + 1, TRANSFORM(Vec3f(0.0,-1.0,0.0)));
        nor->setVertex3f(vertexIndex + 2, TRANSFORM(Vec3f(0.0,-1.0,0.0)));
        nor->setVertex3f(vertexIndex + 3, TRANSFORM(Vec3f(0.0,-1.0,0.0)));
#undef TRANSFORM
      }

      if(cfg.isTexcoRequired)
      {
#define TRANSFORM(x) ( (cfg.texcoScale*x + texcoPos) )
        texco->setVertex2f(vertexIndex + 0, TRANSFORM(Vec2f(0, 0)));
        texco->setVertex2f(vertexIndex + 1, TRANSFORM(Vec2f(quadSize, 0)));
        texco->setVertex2f(vertexIndex + 2, TRANSFORM(Vec2f(quadSize, quadSize)));
        texco->setVertex2f(vertexIndex + 3, TRANSFORM(Vec2f(0, quadSize)));
#undef TRANSFORM
      }

      if(cfg.isTangentRequired)
      {
        Vec3f *vertices = ((Vec3f*)pos->dataPtr())+vertexIndex;
        Vec2f *texcos = ((Vec2f*)texco->dataPtr())+vertexIndex;
        Vec3f *normals = ((Vec3f*)nor->dataPtr())+vertexIndex;
        Vec4f tangent = getFaceTangent(vertices, texcos, normals, 4);
#define TRANSFORM(x) ( x )
        tan->setVertex4f(vertexIndex + 0, TRANSFORM(tangent));
        tan->setVertex4f(vertexIndex + 1, TRANSFORM(tangent));
        tan->setVertex4f(vertexIndex + 2, TRANSFORM(tangent));
        tan->setVertex4f(vertexIndex + 3, TRANSFORM(tangent));
#undef TRANSFORM
      }

      vertexIndex += 4;
      curPos.z += cfg.posScale.z*quadSize;
      texcoPos.y += cfg.texcoScale.y*quadSize;
    }
    texcoPos.x += cfg.texcoScale.x*quadSize;
    curPos.x += cfg.posScale.x*quadSize;
  }

  setInput(ref_ptr<ShaderInput>::cast(pos));
  if(cfg.isNormalRequired) {
    setInput(ref_ptr<ShaderInput>::cast(nor));
  }
  if(cfg.isTexcoRequired) {
    setInput(ref_ptr<ShaderInput>::cast(texco));
  }
  if(cfg.isTangentRequired && cfg.isNormalRequired && cfg.isTexcoRequired) {
    setInput(ref_ptr<ShaderInput>::cast(tan));
  }
}
