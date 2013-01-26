
-- shade
#ifndef __SHADE_
#define2 __SHADE_

// #define SM_DEBUG_SLICES

struct Shading {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#for INDEX to NUM_LIGHTS
#define2 __ID ${LIGHT${INDEX}_ID}
    uniform vec3 in_lightDiffuse${__ID};
    uniform vec3 in_lightSpecular${__ID};
#ifdef LIGHT_HAS_AMBIENT${__ID}
    uniform vec3 in_lightAmbient${__ID};
#endif
#ifdef LIGHT_IS_ATTENUATED${__ID}
    uniform vec3 in_lightAttenuation${__ID};
#endif
  #if LIGHT_TYPE${__ID} == SPOT
    // spot light
    uniform vec3 in_lightPosition${__ID};
    uniform vec2 in_lightConeAngles${__ID};
    uniform vec3 in_lightSpotDirection${__ID};
    #ifdef LIGHT_HAS_SM${__ID}
    uniform mat4 in_shadowMatrix${__ID};
      #ifndef __TEX_shadowMap${__ID}__
        #ifdef LIGHT_USE_SHADOW_SAMPLER${__ID}
    uniform sampler2DShadow shadowMap${__ID};
        #else
    uniform sampler2D shadowMap${__ID};
        #endif
      #endif
    #endif
  #endif
  #if LIGHT_TYPE${__ID} == POINT
    // point light
    uniform vec3 in_lightPosition${__ID};
    #ifdef LIGHT_HAS_SM${__ID}
    uniform float in_shadowFar${__ID};
    uniform float in_shadowNear${__ID};
      #ifndef __TEX_shadowMap${__ID}__
        #ifdef LIGHT_USE_SHADOW_SAMPLER${__ID}
    uniform samplerCubeShadow shadowMap${__ID};
        #else
    uniform samplerCube shadowMap${__ID};
        #endif
      #endif
    #endif
  #endif
  #if LIGHT_TYPE${__ID} == DIRECTIONAL
    // directional light
    uniform vec3 in_lightDirection${__ID};
    #ifdef LIGHT_HAS_SM${__ID}
    uniform float in_shadowFar${__ID}[NUM_SHADOW_MAP_SLICES];
    uniform mat4 in_shadowMatrices${__ID}[NUM_SHADOW_MAP_SLICES];
      #ifndef __TEX_shadowMap${__ID}__
        #ifdef LIGHT_USE_SHADOW_SAMPLER${__ID}
    uniform sampler2DArrayShadow shadowMap${__ID};
        #else
    uniform sampler2DArray shadowMap${__ID};
        #endif
      #endif
    #endif
  #endif
  #ifdef LIGHT_HAS_SM${__ID}
    uniform float shadowMapSize${__ID};
    #ifndef __TEX_shadowMap${__ID}__
#define __TEX_shadowMap${__ID}__
    #endif
  #endif
#endfor

#include shadow-mapping.sampling

#ifdef SM_DEBUG_SLICES
void debugShadowSlice(int shadowMapIndex, inout vec3 diffuse)
{
    vec3 color[8] = vec3[8](
        vec3(1.0, 0.7, 0.7),
        vec3(0.7, 1.0, 0.7),
        vec3(0.7, 0.7, 1.0),
        vec3(1.0, 1.0, 0.7),
        vec3(1.0, 0.7, 1.0),
        vec3(0.7, 1.0, 1.0),
        vec3(1.0, 1.0, 1.0),
        vec3(0.7, 0.7, 0.7));
    diffuse *= color[shadowMapIndex];
}
#endif

float distanceAttenuation(vec3 P, vec3 L, vec3 A)
{
    float dist = length(L - P);
    return min(1.0, 1.0/(A.x + A.y*dist + A.z*dist*dist));
}

float spotAmount(vec3 L, vec3 dir, vec2 coneAngles)
{
    float spotEffect = dot( -L, normalize(dir) );
    return max(0.0,sign(spotEffect-coneAngles.x))*clamp(
            1.0 - (spotEffect-coneAngles.y)/(coneAngles.x-coneAngles.y), 0.0, 1.0);
}

Shading shade(vec3 P, vec3 N, float depth, float shininess)
{
  // accumulates lighting
  Shading s;
  s.ambient = vec3(0.0);
  s.diffuse = vec3(0.0);
  s.specular = vec3(0.0);
  // contribution for a single light
  vec3 diffuse, specular;
  // defines how bright it is with respect to distance from objects
  float attenuation;
  // defines the amount of shadow for this fragment
  float shadow;
  // eye vector in world space
  vec3 E = normalize(P - in_cameraPosition);
  // light to fragment vector in world space
  vec3 L, lightVec;
  float nDotL;

  // calculate ambient light
#for INDEX to NUM_LIGHTS
#define2 __ID ${LIGHT${INDEX}_ID}
  #ifdef LIGHT_HAS_AMBIENT${__ID}
    s.ambient += in_lightAmbient${__ID};
  #endif
#endfor

#for INDEX to NUM_LIGHTS
#define2 __ID ${LIGHT${INDEX}_ID}
#ifdef LIGHT_HAS_SM${__ID}
  #define applyShadow(x) (shadow * x)
#else
  #define applyShadow(x) x
#endif
#ifdef LIGHT_IS_ATTENUATED${__ID}
  #define applyAttenuation(x) (attenuation * x)
#else
  #define applyAttenuation(x) x
#endif

#if LIGHT_TYPE${__ID} == DIRECTIONAL
    /////////
    //// Directional light
    ////////
    L = normalize(in_lightDirection${__ID});
    nDotL = dot( N, L );
    if(nDotL > 0.0) {
  #ifdef LIGHT_HAS_SM${__ID}
        // calculate shadow amount
        shadow = dirShadow${LIGHT_SM_FILTER${__ID}}(P, depth,
            shadowMap${__ID}, shadowMapSize${__ID},
            in_shadowFar${__ID}, in_shadowMatrices${__ID});
  #endif
        // calculate diffuse light
        s.diffuse += in_lightDiffuse${__ID} * applyShadow(nDotL);
        // calculate specular light
        if(shininess > 0.0) {
            float rDotE = max( dot( normalize( reflect( -L, N ) ), normalize( -E ) ), 0.0);
            s.specular += in_lightSpecular${__ID} * applyShadow(pow(rDotE, shininess));
        }
    }
  #ifdef SM_DEBUG_SLICES
    debugShadowSlice(getShadowLayer(depth, in_shadowFar${__ID}), s.diffuse);
  #endif
#endif // LIGHTn_TYPE == DIRECTIONAL

#if LIGHT_TYPE${__ID} == POINT
    /////////
    //// Point light
    ////////
    lightVec = in_lightPosition${__ID} - P;
    L = normalize(lightVec);
    nDotL = dot( N, L );
    if(nDotL>0.0) {
  #ifdef LIGHT_IS_ATTENUATED${__ID}
        attenuation = distanceAttenuation(P,in_lightPosition${__ID},in_lightAttenuation${__ID});
  #endif
  #ifdef LIGHT_HAS_SM${__ID}
        // calculate shadow amount
        shadow = pointShadow${LIGHT_SM_FILTER${__ID}}(lightVec,
            in_shadowFar${__ID}, in_shadowNear${__ID}, shadowMap${__ID}, shadowMapSize${__ID});
  #endif
        // calculate diffuse light
        s.diffuse += in_lightDiffuse${__ID} * applyShadow(applyAttenuation(nDotL));
        // calculate specular light
        if(shininess > 0.0) {
            float rDotE = max( dot( normalize( reflect( -L, N ) ), normalize( -E ) ), 0.0);
            s.specular += in_lightSpecular${__ID} * applyShadow(applyAttenuation(pow(rDotE, shininess)));
        }
    }
#endif // LIGHTn_TYPE == POINT

#if LIGHT_TYPE${__ID} == SPOT
    /////////
    //// Spot light
    ////////
    L = normalize(in_lightPosition${__ID} - P);
    nDotL = dot( N, L );
    if(nDotL>0.0) {
        attenuation = spotAmount(L, in_lightSpotDirection${__ID}, in_lightConeAngles${__ID});
  #ifdef LIGHT_IS_ATTENUATED${__ID}
        // calculate distance attenuation
        attenuation *= distanceAttenuation(P,in_lightPosition${__ID},in_lightAttenuation${__ID});
  #endif
  #ifdef LIGHT_HAS_SM${__ID}
        // calculate shadow amount
        attenuation *= spotShadow${LIGHT_SM_FILTER${__ID}}(P,
            shadowMap${__ID}, shadowMapSize${__ID}, in_shadowMatrix${__ID});
  #endif
        // calculate diffuse light
        s.diffuse += in_lightDiffuse${__ID} * (attenuation * nDotL);
        // calculate specular light
        if(shininess > 0.0) {
            float rDotE = max( dot( normalize( reflect( -L, N ) ), normalize( -E ) ), 0.0);
            s.specular += in_lightSpecular${__ID} * (attenuation * pow(rDotE, shininess));
        }
    }
#endif // LIGHTn_TYPE == SPOT
#undef applyShadow
#undef applyAttenuation

#endfor

  return s;
}
#endif // __SHADE_

