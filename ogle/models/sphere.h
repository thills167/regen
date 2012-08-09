/*
 * sphere.h
 *
 *  Created on: 31.08.2011
 *      Author: daniel
 */

#ifndef SPHERE_H_
#define SPHERE_H_

#include <ogle/states/attribute-state.h>
#include <ogle/algebra/vector.h>

/**
 * Perfectly round geometrical object in
 * three-dimensional space - a sphere ;)
 * The sphere is centered at (0,0,0) and scaled by a user specified
 * factor.
 */
class UnitSphere : public AttributeState
{
public:
  enum TexcoMode {
    // do not generate texture coordinates
    TEXCO_MODE_NONE,
    // generate 2D uv coordinates
    TEXCO_MODE_UV,
    // generate 3D coordinates for cube mapping
    TEXCO_MODE_SPHERE_MAP
  };
  struct Config {
    // scaling for the position attribute.
    // with vec3(1) a unit sphere is created
    Vec3f posScale;
    // scaling vector for TEXCO_MODE_UV
    Vec2f texcoScale;
    // number of surface divisions
    GLuint levelOfDetail;
    // texture coordinate mode
    TexcoMode texcoMode;
    // generate normal attribute ?
    GLboolean isNormalRequired;
    Config();
  };

  UnitSphere();
  void updateAttributes(const Config &cfg);
};

#endif /* SPHERE_H_ */
