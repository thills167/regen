
--------------------------------------
--------------------------------------
---- Simple Depth-Of-Field shader. Combines blurred and original
---- texture by distance to focal plane.
--------------------------------------
--------------------------------------
-- vs
#include regen.post-passes.fullscreen.vs
-- fs
out vec4 out_color;
in vec2 in_texco;

uniform sampler2D in_inputTexture;
uniform sampler2D in_blurTexture;
uniform sampler2D in_depthTexture;
// camera input
uniform float in_far;
uniform float in_near;
// DoF input
const float in_focalDistance = 0.0;
const vec2 in_focalWidth = vec2(0.1,0.2);

#include regen.utility.utility.linearizeDepth

void main() {
    vec4 original = texture(in_inputTexture, in_texco);
    vec4 blurred = texture(in_blurTexture, in_texco);
    // get the depth value at this pixel
    float depth = texture(in_depthTexture, in_texco).r;
    depth = linearizeDepth(depth, in_near, in_far);
    // distance to point with max sharpness
    float d = abs(in_focalDistance - depth);
    float focus = smoothstep(in_focalWidth.x, in_focalWidth.y, d);
    out_color = mix(original, blurred, focus);
}