
--------------------------------
--------------------------------
----- Shader for GUI rendering
--------------------------------
--------------------------------
-- vs
in vec3 in_pos;
in vec2 in_viewport;
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

#define HANDLE_IO()

void main() {
    vec2 pos = 2.0*in_pos.xy;
#ifndef USE_NORMALIZED_COORDINATES
    pos.x -= in_viewport.x;
    pos.y += in_viewport.y;
#endif
#ifdef HAS_modelMatrix
    pos.x += in_modelMatrix[3].x;
    pos.y -= in_modelMatrix[3].y;
#endif
#ifndef USE_NORMALIZED_COORDINATES
    pos /= in_viewport;
#endif

    gl_Position = vec4(pos, 0.0, 1.0);

    HANDLE_IO(gl_VertexID);
}

-- fs
#include textures.defines
#undef HAS_LIGHT
#undef HAS_MATERIAL

#include textures.input

#include textures.mapToFragmentUnshaded

out vec4 out_color;

void main() {
    out_color = vec4(1.0);
    textureMappingFragmentUnshaded(gl_FragCoord.xyz,out_color);
}
