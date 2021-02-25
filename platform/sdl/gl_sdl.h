/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#pragma once

//[ISB] Really don't want to pull in GLEW or a similar library just to load 30 functions or so, so let's do this the hard way.

#if defined(_WIN32) //you have to actually include windows.h for GL.h on windows sweet
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#endif

#include <gl/GL.h>
#include <SDL.h>

//Local funcs
void I_InitGLContext(SDL_Window* win);
void GL_SetVideoMode(int w, int h, bool highcolor, SDL_Rect *bounds);

void GL_SetPalette(uint32_t* pal);
void GL_DrawPhase1();

void I_ShutdownGL();

//GL API
//Literally done just to avoid pulling in a lightweight library. This was a dumb idea but for a good reason.
//There's gotta be a simple permissively licenced thing you can drop right into your code to do this.

//apientry needed so the linking works on windows, but it's not a problem on other platforms...
#ifndef APIENTRY
#define APIENTRY
#endif

//ugh
//TODO check if needed on linux
typedef char GLchar;

//These are how these are defined in GLEW
//Not sure why sizeiptr isn't unsigned, but okay.
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

//basics
extern void(APIENTRY* sglClear)(GLbitfield mask);
extern void(APIENTRY* sglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern void(APIENTRY* sglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
extern GLenum(APIENTRY* sglGetError)();

//shaders
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS  0x8B4A
#define GL_SAMPLER_1D 0x8B5D
#define GL_SAMPLER_2D 0x8B5E
#define GL_SAMPLER_3D 0x8B5F
#define GL_SAMPLER_CUBE 0x8B60
#define GL_SAMPLER_1D_SHADOW 0x8B61
#define GL_SAMPLER_2D_SHADOW 0x8B62
#define GL_DELETE_STATUS 0x8B80
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_VALIDATE_STATUS 0x8B83
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_ATTACHED_SHADERS 0x8B85
#define GL_ACTIVE_UNIFORMS 0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH 0x8B87
#define GL_SHADER_SOURCE_LENGTH 0x8B88
#define GL_ACTIVE_ATTRIBUTES 0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH 0x8B8A
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT 0x8B8B
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#define GL_CURRENT_PROGRAM 0x8B8D

extern GLuint(APIENTRY* sglCreateShader)(GLenum type);
extern GLuint(APIENTRY* sglCreateProgram)();
extern void(APIENTRY* sglDeleteProgram)(GLuint program);
extern void(APIENTRY* sglDeleteShader)(GLuint shader);
extern void(APIENTRY* sglLinkProgram)(GLuint program);
extern void(APIENTRY* sglShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
extern void(APIENTRY* sglUseProgram)(GLuint program);
extern void(APIENTRY* sglGetProgramiv)(GLuint program, GLenum pname, GLint* params);
extern void(APIENTRY* sglGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
extern void(APIENTRY* sglGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
extern void(APIENTRY* sglGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
extern void(APIENTRY* sglGetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source);
extern GLint(APIENTRY* sglGetUniformLocation)(GLuint program, const GLchar* name);
extern void(APIENTRY* sglAttachShader)(GLuint program, GLuint shader);
extern void(APIENTRY* sglCompileShader)(GLuint shader);

//shader uniforms
extern void (APIENTRY* sglUniform1f)(GLint location, GLfloat v0);
extern void (APIENTRY* sglUniform2f)(GLint location, GLfloat v0, GLfloat v1);
extern void (APIENTRY* sglUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void (APIENTRY* sglUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void (APIENTRY* sglUniform1i)(GLint location, GLint v0);
extern void (APIENTRY* sglUniform2i)(GLint location, GLint v0, GLint v1);
extern void (APIENTRY* sglUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
extern void (APIENTRY* sglUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern void (APIENTRY* sglUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
extern void (APIENTRY* sglUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
extern void (APIENTRY* sglUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

//Vertex arrays
extern void(APIENTRY* sglBindVertexArray)(GLuint array);
extern void(APIENTRY* sglDeleteVertexArrays)(GLsizei n, const GLuint* arrays);
extern void(APIENTRY* sglGenVertexArrays)(GLsizei n, GLuint* arrays);
extern void(APIENTRY* sglDisableVertexAttribArray)(GLuint index);
extern void(APIENTRY* sglEnableVertexAttribArray)(GLuint index);
extern void(APIENTRY* sglVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

//Buffers

#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STREAM_DRAW 0x88E0
#define GL_STREAM_READ 0x88E1
#define GL_STREAM_COPY 0x88E2
#define GL_STATIC_DRAW 0x88E4
#define GL_STATIC_READ 0x88E5
#define GL_STATIC_COPY 0x88E6
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_DYNAMIC_READ 0x88E9
#define GL_DYNAMIC_COPY 0x88EA

extern void(APIENTRY* sglBindBuffer)(GLenum target, GLuint buffer);
extern void(APIENTRY* sglDeleteBuffers)(GLsizei n, const GLuint* buffers);
extern void(APIENTRY* sglGenBuffers)(GLsizei n, GLuint* buffers);
extern void(APIENTRY* sglBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
extern void(APIENTRY* sglBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);

//Textures
//Should be more defintions but this is already way more than this project needs.
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6
#define GL_TEXTURE7 0x84C7
#define GL_TEXTURE8 0x84C8

#define GL_RG 0x8227
#define GL_RG_INTEGER 0x8228
#define GL_R8 0x8229
#define GL_R16 0x822A
#define GL_RG8 0x822B
#define GL_RG16 0x822C
#define GL_R16F 0x822D
#define GL_R32F 0x822E
#define GL_RG16F 0x822F
#define GL_RG32F 0x8230
#define GL_R8I 0x8231
#define GL_R8UI 0x8232
#define GL_R16I 0x8233
#define GL_R16UI 0x8234
#define GL_R32I 0x8235
#define GL_R32UI 0x8236
#define GL_RG8I 0x8237
#define GL_RG8UI 0x8238
#define GL_RG16I 0x8239
#define GL_RG16UI 0x823A
#define GL_RG32I 0x823B
#define GL_RG32UI 0x823C
#define GL_RGBA32UI 0x8D70
#define GL_RGB32UI 0x8D71
#define GL_RGBA16UI 0x8D76
#define GL_RGB16UI 0x8D77
#define GL_RGBA8UI 0x8D7C
#define GL_RGB8UI 0x8D7D
#define GL_RGBA32I 0x8D82
#define GL_RGB32I 0x8D83
#define GL_RGBA16I 0x8D88
#define GL_RGB16I 0x8D89
#define GL_RGBA8I 0x8D8E
#define GL_RGB8I 0x8D8F
#define GL_RED_INTEGER 0x8D94
#define GL_GREEN_INTEGER 0x8D95
#define GL_BLUE_INTEGER 0x8D96
#define GL_RGB_INTEGER 0x8D98
#define GL_RGBA_INTEGER 0x8D99
#define GL_BGR_INTEGER 0x8D9A
#define GL_BGRA_INTEGER 0x8D9B
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366

extern void (APIENTRY* sglBindTexture)(GLenum target, GLuint texture);
extern void (APIENTRY* sglDeleteTextures)(GLsizei n, const GLuint* textures);
extern void (APIENTRY* sglGenTextures)(GLsizei n, GLuint* textures);
extern void (APIENTRY* sglActiveTexture)(GLenum texture);
extern void (APIENTRY* sglTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
extern void (APIENTRY* sglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
extern void (APIENTRY* sglTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* pixels);
extern void (APIENTRY* sglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
extern void (APIENTRY* sglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRY* sglTexParameterfv)(GLenum target, GLenum pname, const GLfloat* params);
extern void (APIENTRY* sglTexParameteri)(GLenum target, GLenum pname, GLint param);
extern void (APIENTRY* sglTexParameteriv)(GLenum target, GLenum pname, const GLint* params);

//draw calls
extern void (APIENTRY* sglDrawArrays)(GLenum mode, GLint first, GLsizei count);
