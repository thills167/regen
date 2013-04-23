--------------------------------------
--------------------------------------
---- Geometry-Picking implementation.
---- Link together with mesh TESS/Vertex shaders. 
---- The shader output is an object id, an instance id and the depth of the
---- hovered object.
--------------------------------------
--------------------------------------
-- gs
#version 150

layout(triangles) in;
layout(points, max_vertices=1) out;

// the picker output
out int out_pickObjectID;
out int out_pickInstanceID;
out float out_pickDepth;
out float out_sceneDepth;
// pretend to be fragment shader for name matching.
in int fs_instanceID[3];

// camera input
uniform vec2 in_viewport;
// current mouse position on the viewport
uniform vec2 in_mousePosition;
// mesh id
uniform int in_pickObjectID;
// scene depth
uniform sampler2D in_depthTexture;

// converts to barycentric coordinates for faster intersection test.
vec2 barycentricCoordinate(vec3 dev0, vec3 dev1, vec3 dev2, vec2 mouseDev) {
   vec2 u = dev2.xy - dev0.xy;
   vec2 v = dev1.xy - dev0.xy;
   vec2 r = mouseDev - dev0.xy;
   float d00 = dot(u, u);
   float d01 = dot(u, v);
   float d02 = dot(u, r);
   float d11 = dot(v, v);
   float d12 = dot(v, r);
   float id = 1.0 / (d00 * d11 - d01 * d01);
   float ut = (d11 * d02 - d01 * d12) * id;
   float vt = (d00 * d12 - d01 * d02) * id;
   return vec2(ut, vt);
}
bool isInsideTriangle(vec2 b) {
   return (
       (b.x >= 0.0) &&
       (b.y >= 0.0) &&
       (b.x + b.y <= 1.0)
   );
}
bool isInsideTriangle(vec3 b) {
   return (
       (b.x > 0.0) &&
       (b.y > 0.0) &&
       (b.z > 0.0) &&
       (b.x + b.y + b.z <= 1.0)
   );
}
float intersectionDepth(vec3 dev0, vec3 dev1, vec3 dev2, vec2 mouseDev) {
    float dm0 = distance(mouseDev,dev0.xy);
    float dm1 = distance(mouseDev,dev1.xy);
    float dm2 = distance(mouseDev,dev2.xy);
    float dm12 = dm1+dm2;
    return (dm2/dm12)*((dev0.z*dm1 + dev1.z*dm0)/(dm0+dm1)) +
           (dm1/dm12)*((dev0.z*dm2 + dev2.z*dm0)/(dm0+dm2));
}

void main()
{
    vec3 dev0 = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;
    vec3 dev1 = gl_in[1].gl_Position.xyz/gl_in[1].gl_Position.w;
    vec3 dev2 = gl_in[2].gl_Position.xyz/gl_in[2].gl_Position.w;
    
    vec2 texco = vec2(in_mousePosition.x,
        in_viewport.y-in_mousePosition.y)/in_viewport;
    vec3 mouse = 2.0*vec3(texco, texture(in_depthTexture,texco).x) - vec3(1.0);
    
    // http://en.wikibooks.org/wiki/GLSL_Programming/Rasterization
    vec3 u = dev1-dev0;
    vec3 v = dev2-dev0;
    vec3 q = mouse-dev0;
    vec3 a = vec3(
        length(cross(dev1-mouse,dev2-mouse)),
        length(cross(q,v)),
        length(cross(u,q))
      ) / length(cross(u,v));
    if(!isInsideTriangle(a)) return;
    
    /**
    vec2 bc = barycentricCoordinate(dev0, dev1, dev2, mouseDev);
    //if(!isInsideTriangle(bc)) return;
    float d = bc.x*dev0.z + bc.y*dev1.z + (1.0-bc.x-bc.y)*dev2.z;
    //if(d<0.0) return;
    **/
    
    out_pickObjectID = in_pickObjectID;
    out_pickInstanceID = fs_instanceID[0];
    out_pickDepth = mouse.z;
    EmitVertex();
    EndPrimitive();
}

