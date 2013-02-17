
-- draw.vs
#include particles.vs.passThrough

-- draw.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#include particles.gs.inputs

out vec3 out_velocity;
out vec4 out_posEye;
out vec4 out_posWorld;
out vec2 out_spriteTexco;

uniform mat4 in_viewMatrix;
uniform mat4 in_inverseViewMatrix;
uniform mat4 in_projectionMatrix;
uniform vec2 in_viewport;

#include sprite.getSpritePoints
#include sprite.emit2

void main() {
    if(in_lifetime[0]<=0) { return; }

    out_velocity = in_velocity[0];    
    
    vec4 centerEye = in_viewMatrix * vec4(in_pos[0],1.0);
    vec3 quadPos[4] = getSpritePoints(centerEye.xyz, vec2(in_size[0]), vec3(0.0, 1.0, 0.0));
    emitSprite(in_inverseViewMatrix, in_projectionMatrix, quadPos);
}

-- draw.fs
#extension GL_EXT_gpu_shader4 : enable

#include particles.fs.header

void main() {
    vec3 P = in_posWorld.xyz;
    float opacity = in_particleBrightness;
    
#ifdef USE_SOFT_PARTICLES
    // fade out particles intersecting the world
    opacity *= softParticleOpacity();
#endif
#ifdef USE_NEAR_CAMERA_SOFT_PARTICLES
    // fade out particls near camera
    opacity *= smoothstep(0.0, 5.0, distance(P, in_cameraPosition));
#endif
    // fade out based on texture intensity
    opacity *= texture(in_particleTexture, in_spriteTexco).x;
    if(opacity<0.0001) discard;
    
    // TODO: direct lighting
    vec3 diffuseColor = getDiffuseLight(P, gl_FragCoord.z);
    out_color = vec4(diffuseColor,1.0);
    out_color.rgb *= opacity; // opacity weighted color
    
    out_posWorld = vec3(0.0);
    out_specular = vec4(0.0);
    out_norWorld = vec4(0.0);
}
