#ifndef ATOMS_OS_GL_CONSTANTS_H
#define ATOMS_OS_GL_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* --- Error Codes --- */
#define GL_NO_ERROR           0x0000
#define GL_INVALID_ENUM       0x0500
#define GL_INVALID_VALUE      0x0501
#define GL_INVALID_OPERATION  0x0502
#define GL_STACK_OVERFLOW     0x0503
#define GL_STACK_UNDERFLOW    0x0504
#define GL_OUT_OF_MEMORY      0x0505

/* --- Clear Buffer Bits --- */
#define GL_COLOR_BUFFER_BIT   0x00004000
#define GL_DEPTH_BUFFER_BIT   0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400

/* --- Capabilities --- */
#define GL_DEPTH_TEST         0x0B71
#define GL_CULL_FACE          0x0B44
#define GL_TEXTURE_2D         0x0DE1
#define GL_LIGHTING           0x0B50
#define GL_NORMALIZE          0x0BA1
#define GL_RESCALE_NORMAL     0x803A
#define GL_COLOR_MATERIAL     0x0B57
#define GL_ALPHA_TEST         0x0BC0
#define GL_BLEND              0x0BE2
#define GL_FOG                0x0B60
#define GL_SCISSOR_TEST       0x0C11
#define GL_STENCIL_TEST       0x0B90

/* --- Polygon Offset Capabilities --- */
#define GL_POLYGON_OFFSET_FILL  0x8037
#define GL_POLYGON_OFFSET_LINE  0x2A02
#define GL_POLYGON_OFFSET_POINT 0x2A01
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#define GL_POLYGON_OFFSET_UNITS  0x2A00

/* --- Light Enums (GL_LIGHT0 to GL_LIGHT7) --- */
#define GL_LIGHT0             0x4000
#define GL_LIGHT1             0x4001
#define GL_LIGHT2             0x4002
#define GL_LIGHT3             0x4003
#define GL_LIGHT4             0x4004
#define GL_LIGHT5             0x4005
#define GL_LIGHT6             0x4006
#define GL_LIGHT7             0x4007

/* --- Matrix Modes --- */
#define GL_MODELVIEW          0x1700
#define GL_PROJECTION         0x1701
#define GL_TEXTURE            0x1702

/* --- Depth & Alpha Functions --- */
#define GL_NEVER              0x0200
#define GL_LESS               0x0201
#define GL_EQUAL              0x0202
#define GL_LEQUAL             0x0203
#define GL_GREATER            0x0204
#define GL_NOTEQUAL           0x0205
#define GL_GEQUAL             0x0206
#define GL_ALWAYS             0x0207

/* --- Stencil Operations --- */
#define GL_KEEP               0x1E00
#define GL_INCR               0x1E02
#define GL_DECR               0x1E03
#define GL_INVERT             0x150A
#define GL_INCR_WRAP          0x8507
#define GL_DECR_WRAP          0x8508

/* --- Blend Factors --- */
#define GL_ZERO                   0x0000
#define GL_ONE                    0x0001
#define GL_SRC_COLOR              0x0300
#define GL_ONE_MINUS_SRC_COLOR    0x0301
#define GL_SRC_ALPHA              0x0302
#define GL_ONE_MINUS_SRC_ALPHA    0x0303
#define GL_DST_ALPHA              0x0304
#define GL_ONE_MINUS_DST_ALPHA    0x0305
#define GL_DST_COLOR              0x0306
#define GL_ONE_MINUS_DST_COLOR    0x0307
#define GL_SRC_ALPHA_SATURATE     0x0308

/* --- Fog Modes & Parameters --- */
#define GL_FOG_MODE           0x0B65
#define GL_FOG_DENSITY        0x0B62
#define GL_FOG_START          0x0B63
#define GL_FOG_END            0x0B64
#define GL_FOG_INDEX          0x0B61
#define GL_FOG_COLOR          0x0B66
#define GL_EXP                0x0800
#define GL_EXP2               0x0801

/* --- Polygon Rasterization Modes --- */
#define GL_POINT              0x1B00
#define GL_LINE               0x1B01
#define GL_FILL               0x1B02

/* --- Culling Modes & Front Face Winding --- */
#define GL_CW                 0x0900
#define GL_CCW                0x0901
#define GL_FRONT              0x0404
#define GL_BACK               0x0405
#define GL_FRONT_AND_BACK     0x0408

/* --- Shading Models --- */
#define GL_FLAT               0x1D00
#define GL_SMOOTH             0x1D01

/* --- Material & Light Parameters --- */
#define GL_AMBIENT            0x1200
#define GL_DIFFUSE            0x1201
#define GL_SPECULAR           0x1202
#define GL_EMISSION           0x1600
#define GL_SHININESS          0x1601
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#define GL_POSITION           0x1203
#define GL_SPOT_DIRECTION     0x1204
#define GL_SPOT_EXPONENT      0x1205
#define GL_SPOT_CUTOFF        0x1206
#define GL_CONSTANT_ATTENUATION 0x1207
#define GL_LINEAR_ATTENUATION 0x1208
#define GL_QUADRATIC_ATTENUATION 0x1209

/* --- Light Model Parameters --- */
#define GL_LIGHT_MODEL_AMBIENT      0x0B53
#define GL_LIGHT_MODEL_LOCAL_VIEWER 0x0B51
#define GL_LIGHT_MODEL_TWO_SIDE     0x0B52

/* --- Primitive Modes --- */
#define GL_POINTS             0x0000
#define GL_LINES              0x0001
#define GL_LINE_LOOP          0x0002
#define GL_LINE_STRIP         0x0003
#define GL_TRIANGLES          0x0004
#define GL_TRIANGLE_STRIP     0x0005
#define GL_TRIANGLE_FAN       0x0006
#define GL_QUADS              0x0007
#define GL_QUAD_STRIP         0x0008
#define GL_POLYGON            0x0009

/* --- Texture Format & Types --- */
#define GL_RGB                0x1907
#define GL_RGBA               0x1908

/* --- Texture Parameters & Filters --- */
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S     0x2802
#define GL_TEXTURE_WRAP_T     0x2803

#define GL_NEAREST            0x2600
#define GL_LINEAR             0x2601

#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST  0x2701
#define GL_NEAREST_MIPMAP_LINEAR  0x2702
#define GL_LINEAR_MIPMAP_LINEAR   0x2703

#define GL_REPEAT             0x2901
#define GL_CLAMP              0x2900

/* --- Pixel Storage --- */
#define GL_UNPACK_ALIGNMENT   0x0CF5
#define GL_PACK_ALIGNMENT     0x0CF0

/* --- Texture Environment --- */
#define GL_TEXTURE_ENV        0x2300
#define GL_TEXTURE_ENV_MODE   0x2200
#define GL_MODULATE           0x2100
#define GL_REPLACE            0x1E01

/* --- Matrix Stack Depths --- */
#define GL_MAX_MODELVIEW_STACK_DEPTH  32
#define GL_MAX_PROJECTION_STACK_DEPTH 8

/* --- Client Array Capabilities --- */
#define GL_VERTEX_ARRAY        0x8074
#define GL_NORMAL_ARRAY        0x8075
#define GL_COLOR_ARRAY         0x8076
#define GL_INDEX_ARRAY         0x8077
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_EDGE_FLAG_ARRAY     0x8079

/* --- Data Types --- */
#define GL_BYTE                0x1400
#define GL_UNSIGNED_BYTE       0x1401
#define GL_SHORT               0x1402
#define GL_UNSIGNED_SHORT      0x1403
#define GL_INT                 0x1404
#define GL_UNSIGNED_INT        0x1405
#define GL_FLOAT               0x1406
#define GL_DOUBLE              0x140A

/* --- Display List Modes --- */
#define GL_COMPILE             0x1300
#define GL_COMPILE_AND_EXECUTE 0x1301

/* --- Max Limits --- */
#define GL_MAX_VERTICES_PER_BEGIN     4096
#define GL_MAX_TEXTURE_SIZE           2048
#define GL_MAX_MIP_LEVELS             12
#define GL_MAX_TEXTURE_OBJECTS        64
#define GL_MAX_LIGHTS                 8
#define GL_MAX_LISTS                  256
#define GL_MAX_LIST_CALL_DEPTH        16
#define GL_MAX_FRAMEBUFFERS           32
#define GL_MAX_RENDERBUFFERS          32
#define GL_MAX_RENDERBUFFER_SIZE      2048

/* --- Phase 10: Framebuffer & Renderbuffer Objects --- */
#define GL_FRAMEBUFFER                            0x8D40
#define GL_READ_FRAMEBUFFER                       0x8CA8
#define GL_DRAW_FRAMEBUFFER                       0x8CA9
#define GL_RENDERBUFFER                           0x8D41

#define GL_COLOR_ATTACHMENT0                      0x8CE0
#define GL_COLOR_ATTACHMENT1                      0x8CE1
#define GL_COLOR_ATTACHMENT2                      0x8CE2
#define GL_COLOR_ATTACHMENT3                      0x8CE3
#define GL_DEPTH_ATTACHMENT                       0x8D00
#define GL_STENCIL_ATTACHMENT                     0x8D20
#define GL_DEPTH_STENCIL_ATTACHMENT               0x821A

#define GL_FRAMEBUFFER_COMPLETE                   0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT      0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS      0x8CD9
#define GL_FRAMEBUFFER_UNSUPPORTED                0x8CDD

#define GL_RENDERBUFFER_WIDTH                     0x8D42
#define GL_RENDERBUFFER_HEIGHT                    0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT           0x8D44

#define GL_RGBA8                                  0x8058
#define GL_RGBA4                                  0x8056
#define GL_RGB5_A1                                0x8057
#define GL_DEPTH_COMPONENT16                      0x81A5
#define GL_DEPTH_COMPONENT24                      0x81A6
#define GL_STENCIL_INDEX8                         0x8D48
#define GL_DEPTH24_STENCIL8                       0x88F0

#define GL_FRAMEBUFFER_BINDING                    0x8CA6
#define GL_RENDERBUFFER_BINDING                   0x8CA7

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_CONSTANTS_H
