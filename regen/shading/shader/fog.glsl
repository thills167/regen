
-- fogIntensity
float fogIntensity(float d)
{
    float x = smoothstep(in_fogDistance.x, in_fogDistance.y, d);
#ifdef USE_EXP_FOG
    return 1.0 - exp( -pow(1.75*x, 2.0) );
#else
    return x;
#endif
}

--------------------------------------
--------------------------------------
---- Computes fog by distance to camera. Input is a unit-quad mesh.
--------------------------------------
--------------------------------------
-- distance.vs
#include utility.fullscreen.vs

-- distance.fs
out vec4 out_color;
in vec2 in_texco;

uniform sampler2D in_gDepthTexture;
#ifdef USE_TBUFFER
uniform sampler2D in_tDepthTexture;
uniform sampler2D in_tColorTexture;
#endif

#ifdef USE_SKY_COLOR
uniform samplerCube in_skyColorTexture;
#else
const vec3 in_fogColor = vec3(1.0);
#endif
const vec2 in_fogDistance = vec2(0.0,100.0);
const float in_fogDensity = 1.0;

uniform vec3 in_cameraPosition;
uniform mat4 in_viewMatrix;
uniform mat4 in_inverseViewProjectionMatrix;

#include fog.fogIntensity
#include utility.texcoToWorldSpace

void main() {
    float d0 = texture(in_gDepthTexture, in_texco).x;
    if(d0==1.0) discard; // discard background pixels
    vec3 eye0 = texcoToWorldSpace(in_texco, d0) - in_cameraPosition;
    float factor0 = fogIntensity(length(eye0));
    
#ifdef USE_SKY_COLOR
    vec3 fogColor = texture(in_skyColorTexture, vec3(0.0,1.0,0.0)).rgb;
#else
    vec3 fogColor = in_fogColor;
#endif
    
#ifdef USE_TBUFFER
    float d1 = texture(in_tDepthTexture, in_texco).x;
    vec3 eye1 = texcoToWorldSpace(in_texco, d1) - in_cameraPosition;
    
    // use standard fog color from eye to transparent object
    float factor1 = fogIntensity(length(eye1));
    out_color = (factor1*in_fogDensity)*fogColor;
    
    // starting from transparent object to scene depth sample use alpha blended fog color.
    vec4 tcolor = texture(in_tColorTexture, in_texco).x;
    vec3 blended = fogColor*(1.0-tcolor.a) + tcolor.rgb*tcolor.a;
    // substract intensity from eye to p1
    factor0 -= factor1;
    // multiple by alpha value (no fog behind opaque objects)
    factor0 *= (1.0-tcolor.a);
    out_color += (factor0*in_fogDensity) * blended;

#else
    out_color = vec4(fogColor, factor0*in_fogDensity);
#endif
}

--------------------------------------
---- Draw a fog volume. Can be used for spot and point lights.
--------------------------------------
-- volumetric.fs
#extension GL_EXT_gpu_shader4 : enable

out vec3 out_color;
#ifdef IS_SPOT_LIGHT
in vec3 in_intersection;
#endif

// G-buffer input
uniform sampler2D in_gDepthTexture;
#ifdef USE_TBUFFER
// T-buffer input
uniform sampler2D in_tDepthTexture;
uniform sampler2D in_tColorTexture;
#endif
// light input
uniform vec3 in_lightPosition;
#ifdef IS_SPOT_LIGHT
uniform vec3 in_lightDirection;
uniform vec2 in_lightConeAngles;
uniform mat4 in_modelMatrix;
#endif
uniform vec2 in_lightRadius;
uniform vec3 in_lightDiffuse;
#ifdef USE_SHADOW_MAP
// shadow input
uniform float in_shadowFar;
uniform float in_shadowNear;
uniform float in_shadowInverseSize;
#ifdef IS_SPOT_LIGHT
  #ifdef USE_SHADOW_SAMPLER
uniform sampler2DShadow in_shadowTexture;
  #else
uniform sampler2D in_shadowTexture;
  #endif
uniform mat4 in_shadowMatrix;
#else // !IS_SPOT_LIGHT
  #ifdef USE_SHADOW_SAMPLER
uniform samplerCubeShadow in_shadowTexture;
  #else
uniform samplerCube in_shadowTexture;
  #endif
uniform mat4 in_shadowMatrix[6];
#endif // !IS_SPOT_LIGHT
const float in_shadowSampleStep = 0.025;
const float in_shadowSampleThreshold = 0.075;
#endif // USE_SHADOW_MAP
// camera input
uniform vec2 in_viewport;
uniform vec3 in_cameraPosition;
uniform mat4 in_inverseViewProjectionMatrix;
uniform mat4 in_viewProjectionMatrix;
// fog input
uniform float in_fogExposure;
uniform vec2 in_fogRadiusScale;
#ifdef IS_SPOT_LIGHT
uniform vec2 in_fogConeScale;
#endif
uniform vec2 in_fogDistance;

#include utility.pointVectorDistance
#include utility.texcoToWorldSpace
#include utility.worldSpaceToTexco

#include shading.radiusAttenuation
#ifdef IS_SPOT_LIGHT
  #include shading.spotConeAttenuation
#endif

#ifdef USE_SHADOW_MAP
  #ifdef IS_SPOT_LIGHT
    #include shadow_mapping.sampling.spot
  #else
    #include shadow_mapping.sampling.point
    #include utility.computeCubeLayer
  #endif
#endif // USE_SHADOW_MAP

#include fog.fogIntensity

#ifdef IS_SPOT_LIGHT
void solvableQuadratic(
    float a, float b, float c,
    out float t0, out float t1)
{
    // Note: discriminant should always be >=0.0 because we are
    // using the cone mesh as input.
    float discriminant = b*b - 4.0*a*c;
    // numerical receipes 5.6 (this method ensures numerical accuracy is preserved)
    float t = -0.5 * (b + sign(b)*sqrt(discriminant));
    t0 = t / a;
    t1 = c / t;
}
vec2 computeConeIntersections(
    vec3 pos, vec3 ray,
    vec3 conePos, vec3 coneDir,
    float cosAngle)
{
    // TODO: cone intersection could be simplified knowing one intersection, i guess
    vec2 t = vec2(0.0);
    vec3 dp = pos-conePos;
    float a = dot(coneDir,ray);
    float b = dot(coneDir,dp);
    float phi = cosAngle*cosAngle;
    solvableQuadratic(
         a*a - phi*dot(ray,ray),
        (a*b - phi*dot(ray,dp))*2.0,
         b*b - phi*dot(dp,dp),
         t.x,t.y);
    // t.x is backface intersection and t.y frontface
    t = vec2( min(t.x,t.y), max(t.x,t.y) );
    // compute intersection points
    vec3 x0 = pos + t.x*ray;
    vec3 x1 = pos + t.y*ray;
    // near intersects reflected cone ?
    float reflected0 = float(dot(coneDir, x0-conePos)<0.0);
    // far intersects reflected cone ?
    float reflected1 = float(dot(coneDir, x1-conePos)<0.0);
    t = (1.0-reflected0-reflected1)*t +
        vec2(reflected0*t.y, reflected0 + reflected1*t.x);
    return t;
}
#endif

#ifdef USE_SHADOW_MAP
float volumeShadow(vec3 start, vec3 stop, float _step)
{
  vec3 p = start, lightVec;
  float shadow = 0.0, shadowDepth;
  // ray through the light volume
  vec3 stepRay = stop-start;
  // scale factor for the ray (clamp to minimum to avoid tight samples)
  float step = max(_step, in_shadowSampleThreshold/length(stepRay));
  stepRay *= step;
  // step through the volume
  for(float i=step; i<1.0; i+=step)
  {
    lightVec = in_lightPosition - p;
#ifdef IS_POINT_LIGHT
    shadowDepth = (vec4(lightVec,1.0)*
      in_shadowMatrix[computeCubeLayer(lightVec)]).z;
    shadow += pointShadowSingle(
      in_shadowTexture, lightVec, shadowDepth,
      in_shadowNear, in_shadowFar, in_shadowInverseSize);
#endif
#ifdef IS_SPOT_LIGHT
    shadow += spotShadowSingle(
      in_shadowTexture, in_shadowMatrix*vec4(p,1.0),
      lightVec, in_shadowNear, in_shadowFar);
#endif
    p += stepRay;
  }
  return shadow*step;
}
#endif

void main()
{
    vec2 texco = gl_FragCoord.xy/in_viewport;
    vec3 vertexPos = texcoToWorldSpace(texco, texture(in_gDepthTexture, texco).x);
    vec3 vertexRay = vertexPos-in_cameraPosition;
    // fog volume scales light radius
    vec2 lightRadius = in_lightRadius*in_fogRadiusScale;
    // compute point in the volume with maximum light intensity
#ifdef IS_SPOT_LIGHT
    // compute a ray. all intersections must be in range [0,1]*ray
    vec3 ray1 = in_intersection - in_cameraPosition;
    float toggle = float(dot(ray1,ray1) > dot(vertexRay,vertexRay));
    vec3 ray = toggle*vertexRay + (1.0-toggle)*ray1;
    // compute intersection points
    vec2 t = computeConeIntersections(
        in_cameraPosition, ray,
        in_lightPosition,
        normalize(in_lightDirection),
        in_lightConeAngles.y);
    t.x = clamp(t.x,0.0,1.0);
    t.y = clamp(t.y,0.0,1.0);
    // clamp to ray length
    vec3 x = in_cameraPosition + 0.5*(t.x+t.y)*ray;
#else
    float d = clamp(pointVectorDistance(
        vertexRay, in_lightPosition - in_cameraPosition), 0.0, 1.0);
    vec3 x = in_cameraPosition + d*vertexRay;
#endif
    // compute fog exposure by distance to camera
    float dCam = length(x-in_cameraPosition)/length(vertexRay);
    // compute fog exposure by distance to camera
    float exposure = in_fogExposure * (1.0 - fogIntensity(dCam));
#ifdef IS_SPOT_LIGHT
    // approximate spot falloff.
    exposure *= spotConeAttenuation(
        normalize(in_lightPosition - x),
        in_lightDirection,
        in_lightConeAngles*in_fogConeScale);
    vec3 start = in_cameraPosition + t.x*ray;
    vec3 stop = in_cameraPosition + t.y*ray;
    // compute distance attenuation.
    float a0 = radiusAttenuation(min(
        distance(in_lightPosition, start),
        distance(in_lightPosition, stop)),
        lightRadius.x, lightRadius.y);
#else
    // compute distance attenuation.
    // vertexRay and the light position.
    float lightDistance = distance(in_lightPosition, x);
    float a0 = radiusAttenuation(lightDistance, lightRadius.x, lightRadius.y);
#endif

#ifdef USE_SHADOW_MAP
    // sample shadow map along ray through volume
#ifdef IS_POINT_LIGHT
    float omega = sqrt(lightRadius.y*lightRadius.y - lightDistance*lightDistance);
    vec3 start = in_cameraPosition + clamp(d+omega, 0.0, 1.0)*vertexRay;
    vec3 stop = in_cameraPosition + clamp(d-omega, 0.0, 1.0)*vertexRay;
#endif
    exposure *= volumeShadow(start,stop,in_shadowSampleStep);
#endif

#ifdef USE_TBUFFER
    // TODO: test
    vec3 alphaPos = texcoToWorldSpace(texco, texture(in_tDepthTexture, texco).x);
    float dLightAlpha = distance(alphaPos, in_lightPosition);
    float a1 = radiusAttenuation(dLightAlpha, lightRadius.x, lightRadius.y));
    vec4 tcolor = texture(in_tColorTexture, texco);
#if 0
    float dz = sqrt(pow(in_radius,2) - pow(dnl,2));
    float blendFactor = smoothstep(dLightNearest - dz, dLightNearest + dz, distance(in_cameraPosition,alphaPos));
#else
    // x=1 -> transparent object in front else x=0
    // when transparent object is in front then at least 50% of the volume
    // is blended with the transparent color
    float x = float(dCamNearest>distance(in_cameraPosition,alphaPos));
    // occlusion=1 -> the other 50% are also occluded.
    float occlusion = x - (2.0*x - 1.0)*a1/a0;
    // linear blend between unoccluded volume and transparency occluded
    // volume.
    float blendFactor = x*0.5 + occlusion*0.5;
    // apply unoccluded fog
    out_color  = (1.0-blendFactor) * in_lightDiffuse;
    // apply transparency occluded fog using alpha blending between fog
    // and transparency color. Also scale result by alpha inverse.
    out_color += (blendFactor*(1.0-tcolor.a)) *
        (in_lightColor*(1.0-tcolor.a) + tcolor.rgb*tcolor.a);
    // scale by attenuation and exposure factor
    out_color *= exposure * a0;
#endif

#else
    out_color = (exposure * a0) * in_lightDiffuse;
#endif // USE_TBUFFER
}

--------------------------------------
--------------------------------------
---- Volumetric Fog for point lights.
--------------------------------------
--------------------------------------
-- volumetric.point.vs
#define IS_POINT_LIGHT
// #undef IS_SPOT_LIGHT
#include shading.deferred.point.vs
-- volumetric.point.fs
#define IS_POINT_LIGHT
// #undef IS_SPOT_LIGHT
#include fog.volumetric.fs

--------------------------------------
--------------------------------------
---- Volumetric Fog for spot lights.
--------------------------------------
--------------------------------------
-- volumetric.spot.vs
//#undef IS_POINT_LIGHT
#define IS_SPOT_LIGHT
#include shading.deferred.spot.vs
-- volumetric.spot.fs
//#undef IS_POINT_LIGHT
#define IS_SPOT_LIGHT
#include fog.volumetric.fs
