
-- incrementalGaussian
const float sqrt2pi = 2.506628274631;
const float in_blurSigma = 4.0;

void incrementalGaussian() {
    // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
    float x_incrementalGaussian = 1.0/ (sqrt2pi * in_blurSigma);
    float y_incrementalGaussian = exp(-0.5 / (in_blurSigma * in_blurSigma));
    out_incrementalGaussian = vec3(
        x_incrementalGaussian,
        y_incrementalGaussian,
        y_incrementalGaussian * y_incrementalGaussian
    );
}

--------------------------------------
--------------------------------------
---- Separable blur pass. Input mesh should be a unit-quad.
---- Supports blurring cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- vs
#include regen.utility.sampling.defines
#include regen.utility.sampling.vsHeader

uniform vec2 in_inverseViewport;

#if RENDER_TARGET == 2D
flat out vec2 out_blurStep;
flat out vec3 out_incrementalGaussian;

#include regen.post-passes.blur.incrementalGaussian
#endif

void main() {
#ifdef RENDER_TARGET == 2D
    incrementalGaussian();
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
#ifdef BLUR_HORIZONTAL
    out_blurStep = vec2(in_inverseViewport.x, 0.0);
#else
    out_blurStep = vec2(0.0, in_inverseViewport.y);
#endif
#endif
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#include regen.utility.sampling.defines
#if RENDER_LAYER > 1
#include regen.utility.sampling.gsHeader
#include regen.utility.sampling.gsEmit

uniform vec2 in_inverseViewport;

flat out vec3 out_incrementalGaussian;
flat out vec3 out_blurStep;

#include regen.post-passes.blur.incrementalGaussian

void main(void) {
    int layer = gl_InvocationID;
    // select framebuffer layer
    gl_Layer = layer;

    incrementalGaussian();
#if RENDER_TARGET == CUBE
#ifdef BLUR_HORIZONTAL
    float dx = in_inverseViewport.x;
    vec3 blurStepArray[6] = vec3[](
        vec3(0.0, 0.0, -dx), // +X
        vec3(0.0, 0.0,  dx), // -X
        vec3( dx, 0.0, 0.0), // +Y
        vec3( dx, 0.0, 0.0), // -Y
        vec3( dx, 0.0, 0.0), // +Z
        vec3(-dx, 0.0, 0.0)  // -Z
    );
#else
    float dy = in_inverseViewport.y;
    vec3 blurStepArray[6] = vec3[](
        vec3(0.0,  dy, 0.0), // +X
        vec3(0.0,  dy, 0.0), // -X
        vec3(0.0, 0.0, -dy), // +Y
        vec3(0.0, 0.0,  dy), // -Y
        vec3(0.0,  dy, 0.0), // +Z
        vec3(0.0,  dy, 0.0)  // -Z
    );
#endif
    out_blurStep = blurStepArray[layer];
#else
#ifdef BLUR_HORIZONTAL
    out_blurStep = vec3(in_inverseViewport.x, 0.0, 0.0);
#else
    out_blurStep = vec3(0.0, in_inverseViewport.y, 0.0);
#endif
#endif

    emitVertex(gl_PositionIn[0], layer);
    emitVertex(gl_PositionIn[1], layer);
    emitVertex(gl_PositionIn[2], layer);
    EndPrimitive();
}
#endif

-- fs
#include regen.utility.sampling.fsHeader
out vec4 out_color;

const int in_numBlurPixels = 4;
#ifdef RENDER_TARGET == 2D
flat in vec2 in_blurStep;
#else
flat in vec3 in_blurStep;
#endif
flat in vec3 in_incrementalGaussian;

#define SAMPLE(texco) texture(in_inputTexture,texco)*incrementalGaussian.x

void main()
{
    float coefficientSum = 0.0;
    vec3 incrementalGaussian = in_incrementalGaussian;

    // Take the central sample first...
    out_color = SAMPLE(in_texco);
    coefficientSum += incrementalGaussian.x;
    incrementalGaussian.xy *= incrementalGaussian.yz;

    for(float i=1.0; i<float(in_numBlurPixels); i++) {
#ifdef RENDER_TARGET == 2D
        vec2 offset = i*in_blurStep;
#else
        vec3 offset = i*in_blurStep;
#endif
        out_color += SAMPLE( in_texco - offset );         
        out_color += SAMPLE( in_texco + offset );    
        coefficientSum += 2.0 * incrementalGaussian.x;
        incrementalGaussian.xy *= incrementalGaussian.yz;
    }

    out_color /= coefficientSum;
}

--------------------------------------
--------------------------------------
---- Horizontal blur pass. Input mesh should be a unit-quad.
---- Supports blurring cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- horizontal.vs
#define BLUR_HORIZONTAL
#include regen.post-passes.blur.vs
-- horizontal.gs
#define BLUR_HORIZONTAL
#include regen.post-passes.blur.gs
-- horizontal.fs
#define BLUR_HORIZONTAL
#include regen.post-passes.blur.fs

--------------------------------------
--------------------------------------
---- Vertical blur pass. Input mesh should be a unit-quad.
---- Supports blurring cube textures, texture arrays and regular 2D textures.
--------------------------------------
--------------------------------------
-- vertical.vs
#define BLUR_VERTICAL
#include regen.post-passes.blur.vs
-- vertical.gs
#define BLUR_VERTICAL
#include regen.post-passes.blur.gs
-- vertical.fs
#define BLUR_VERTICAL
#include regen.post-passes.blur.fs
