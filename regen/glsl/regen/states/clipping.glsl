
-- defines
#if HAS_clipPlane || RENDER_TARGET == DUAL_PARABOLOID || RENDER_TARGET == PARABOLOID
#define HAS_CLIPPING
#endif

-- input
#ifndef __clipInput_Included__
#define __clipInput_Included__
#include regen.states.clipping.defines
#ifdef HAS_clipPlane
uniform vec4 in_clipPlane;
#endif
#endif // __clipInput_Included__

-- isClipped
#ifndef __isClipped_Included__
#define __isClipped_Included__
#include regen.states.clipping.input
#ifdef HAS_CLIPPING
bool isClipped(vec3 posWorld)
{
#if RENDER_TARGET == DUAL_PARABOLOID || RENDER_TARGET == PARABOLOID
  if(sign(in_posEye.z) != sign(2*in_layer - 1)) return true;
  //if(in_posEye.z<0.0) return true;
#endif
#ifdef HAS_clipPlane
  if(dot(posWorld,in_clipPlane.xyz)-in_clipPlane.w<=0.0) return true;
#endif
  return false;
}
#else
#define isClipped(x) false
#endif // HAS_CLIPPING
#endif // __isClipped_Included__
