
-- vsHeader
in vec3 in_pos;
#ifdef IS_2D_TEXTURE
out vec2 out_texco;
#endif

-- gsHeader
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

out vec3 out_texco;

-- gsEmit
#ifdef IS_CUBE_TEXTURE
#include regen.utility.utility.computeCubeDirection
void emitVertex(vec4 P, int layer)
{
    gl_Position = P;
    out_texco = computeCubeDirection(vec2(P.x, -P.y), layer);
    EmitVertex();
}
#else
void emitVertex(vec4 P, int layer)
{
    gl_Position = P;
    out_texco = vec3(0.5*(gl_Position.xy+vec2(1.0)), layer);
    EmitVertex();
}
#endif

-- fsHeader
#ifdef IS_2D_TEXTURE
in vec2 in_texco;
#else
in vec3 in_texco;
#endif

#ifdef IS_ARRAY_TEXTURE
uniform sampler2DArray in_inputTexture;
#elif IS_CUBE_TEXTURE
uniform samplerCube in_inputTexture;
#else // IS_2D_TEXTURE
uniform sampler2D in_inputTexture;
#endif

--------------------------------------
--------------------------------------
---- Sample an input texture.
---- Supports cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- vs
#include regen.utility.sampling.vsHeader

void main() {
#ifdef IS_2D_TEXTURE
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
#endif
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#ifndef IS_2D_TEXTURE
#include regen.utility.sampling.gsHeader
#include regen.utility.sampling.gsEmit

void main(void) {
#ifdef IS_CUBE_TEXTURE
#define2 __LAYER_COUNT__ 6
#else
#define2 __LAYER_COUNT__ ${NUM_TEXTURE_LAYERS}
#endif
  
#for LAYER to ${__LAYER_COUNT__}
#ifndef SKIP_LAYER${LAYER}
    // select framebuffer layer
    gl_Layer = ${LAYER};
    emitVertex(gl_PositionIn[0], ${LAYER});
    emitVertex(gl_PositionIn[1], ${LAYER});
    emitVertex(gl_PositionIn[2], ${LAYER});
    EndPrimitive();
#endif
#endfor
}
#endif

-- fs
#include regen.utility.sampling.fsHeader
out vec4 out_color;
void main()
{
    out_color = texture(in_inputTexture, in_texco);
}

--------------------------------------
--------------------------------------
---- Sample down an input texture.
---- Supports cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- downsample.vs
#include regen.utility.sampling.vs
-- downsample.gs
#include regen.utility.sampling.gs
-- downsample.fs
#include regen.utility.sampling.fs
