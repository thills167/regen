
--------------------------------
--------------------------------
----- Raycasting Shader
--------------------------------
--------------------------------
-- vs
#undef HAS_LIGHT
#undef HAS_MATERIAL

in vec3 in_pos;

out vec3 out_rayOrigin;
out vec3 out_rayDirection;

#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif
#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
#include regen.states.model.transformModel

#define HANDLE_IO()

void main() {
    vec4 posWorld = transformModel(vec4(in_pos,1.0));
    vec4 posEye = transformWorldToEye(posWorld,0);
#ifdef HAS_modelMatrix
    out_rayOrigin = (
        transpose(in_modelMatrix) *
        vec4(in_cameraPosition,1.0)).xyz;
#else
    out_rayOrigin = in_cameraPosition;
#endif
    out_rayDirection = in_pos.xyz - out_rayOrigin;

    gl_Position = transformEyeToScreen(posEye,0);

    HANDLE_IO(gl_VertexID);
}

-- tcs
#include regen.models.mesh.tcs
-- tes
#include regen.models.mesh.tes
-- gs
#include regen.models.mesh.gs
-- fs
#define DRAW_RAY_LENGTH 0
#define DRAW_RAY_START 0
#define DRAW_RAY_STOP 0
#include regen.models.mesh.fs-outputs

#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

in vec3 in_rayOrigin;
in vec3 in_rayDirection;

uniform sampler3D in_volumeTexture;
uniform sampler2D in_transferTexture;
uniform vec3 in_cameraPosition;

const float in_rayStep=0.02;
const float in_densityThreshold=0.125;
const float in_densityScale=2.0;
const vec3 in_matAmbient = vec3(1.0);

#include regen.states.textures.mapToFragment
#include regen.states.textures.mapToLight
#include regen.models.mesh.writeOutput

vec4 volumeTransfer(float val)
{
    vec3 col = texture(in_transferTexture, vec2(val)).rgb;
    return vec4(col, min(1.0,in_densityScale*val));
}

bool intersectBox(vec3 origin, vec3 dir, out float t0, out float t1)
{
    vec3 invR = 1.0 / dir;
    vec3 tbot = invR * (vec3(-1.0)-origin);
    vec3 ttop = invR * (vec3(+1.0)-origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t0 < t1;
}

void main() {
    vec3 rayDirection = normalize(in_rayDirection);

    float tnear, tfar;
    intersectBox( in_rayOrigin, rayDirection, tnear, tfar);
    tnear = max(tnear,0.0);
    
    vec3 rayStart = in_rayOrigin + rayDirection * tnear;
    vec3 rayStop = in_rayOrigin + rayDirection * tfar;
#ifdef SWITCH_VOLUME_Y
    rayStart.y *= -1; rayStop.y *= -1;
#endif
    
    // Transform from object space to texture coordinate space:
    rayStart = 0.5 * (rayStart + vec3(1.0));
    rayStop = 0.5 * (rayStop + vec3(1.0));
    
    vec3 ray = rayStop - rayStart;
    vec3 stepVector = normalize(ray) * in_rayStep;
    
    vec3 pos = rayStart;
    vec4 dst = vec4(0);
#ifdef USE_AVERAGE_INTENSITY
    float counter = 0.0;
#endif
    for(float rayLength=length(ray); rayLength>=0.0; rayLength-=in_rayStep)
    {
        float value = texture(in_volumeTexture, pos).x;
#ifdef USE_MAX_INTENSITY
        if(value > max(in_densityThreshold,dst.a)) {
            dst = volumeTransfer(value);
        }
#elif USE_AVERAGE_INTENSITY
        if(value > in_densityThreshold) {
            dst += volumeTransfer(value);
            counter += 1.0;
        }
#elif USE_FIRST_MAXIMUM
        if(value > in_densityThreshold) {
            if(dst.a <= value) {
                dst = volumeTransfer(value);
            }
            else { break; }
        }
#else // emission/absorbtion
        if(value > in_densityThreshold) {
            vec4 src = volumeTransfer(value);
            // opacity weighted color
            src.rgb *= src.a;
            // front-to-back blending
            dst = (1.0 - dst.a) * src + dst;
            // break out of the loop if alpha reached 1.0
            if(dst.a > 0.95) break;
        }
#endif
        pos += stepVector;
    }
#ifdef USE_AVERAGE_INTENSITY
    dst /= counter;
#endif

#if DRAW_RAY_LENGTH==1
    writeOutput(pos,vec3(0,1,0),vec4(vec3(length(ray)), 1.0));
#elif DRAW_RAY_START==1
    writeOutput(pos,vec3(0,1,0),vec4(rayStart, 1.0));
#elif DRAW_RAY_STOP==1
    writeOutput(pos,vec3(0,1,0),vec4(rayStop, 1.0));
#else
    writeOutput(pos,vec3(0,1,0),dst);
#endif
}

