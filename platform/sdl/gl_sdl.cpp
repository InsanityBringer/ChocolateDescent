/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_video.h"

#include "gl_sdl.h"
#include "2d/gr.h"
#include "platform/platform.h"
#include "misc/error.h"

SDL_GLContext context;
SDL_Window* window;

//Simple vertex buffer, two triangles, XY coords only, as triangle fan.
const float buf[] = { -1.0f, 3.0f, 0.0f, 0.0f,
					  -1.0f, -1.0f, 0.0f, 2.0f,
					  3.0f, -1.0f, 2.0f, 2.0f,
					  1.0f, 1.0f, 1.0f, 0.0f}; //rip

//TODO: Need to downgrade these to 150 or so
const char* vertexSource =
"#version 330 core\n"
"\n"
"layout(location=0) in vec2 point;\n"
"layout(location=1) in vec2 uvCoord;\n"
"\n"
"smooth out vec2 uv;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = vec4(point.x, point.y, 0.0, 1.0);\n"
"	uv = uvCoord;"
"}\n"
"\n";

const char* fragmentSource =
"#version 330 core\n"
"\n"
"smooth in vec2 uv;\n"
"\n"
"out vec4 color;\n"
"\n"
"uniform sampler1D palette;\n"
"uniform usampler2D srcfb;\n"
"\n"
"void main()\n"
"{\n"
"	color = texelFetch(palette, int(texture(srcfb, uv).r), 0);\n"
//"	color = texture(palette, gl_FragCoord.x / 256.0);\n"
//"	color = vec4(0.0, 0.0, 0.5, 1.0);\n"
//"	float h = texture(srcfb, vec2(gl_FragCoord.x / 640.0, gl_FragCoord.y / 480.0)).r / 255.0;\n"
//"	color = vec4(h, h, h, 1.0);\n"
"}\n"
"\n";

void GL_ErrorCheck(const char* context)
{
#ifndef NDEBUG
	int error;
	error = sglGetError();
	if (error != GL_NO_ERROR)
	{
		fprintf(stderr, "Error in context %s: ", context);
		if (error == GL_INVALID_ENUM)
			fprintf(stderr, "Invalid enum\n");
		//else if (error == GL_INVALID_NAME)
		//	fprintf(stderr, "Invalid name\n");
		else if (error == GL_INVALID_OPERATION)
			fprintf(stderr, "Invalid operation\n");
		else if (error == GL_INVALID_VALUE)
			fprintf(stderr, "Invalid value\n");
	}
#endif
}

//GL State
GLuint paletteName;
GLuint sourceFBName;
GLuint vaoName;
GLuint bufName;

GLuint phase1ProgramName;

GLuint GL_CompileShader(const char* src, GLenum type)
{
	GLuint name = sglCreateShader(type);
	GLint status;
	GLint len = (GLint)(strlen(src));
	sglShaderSource(name, 1, &src, &len);
	sglCompileShader(name);

	sglGetShaderiv(name, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		GLint infoLogLength;
		sglGetShaderiv(name, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* buf = new char [infoLogLength];
		sglGetShaderInfoLog(name, infoLogLength, NULL, buf);

		fprintf(stderr, "Error compiling shader:\n%s\n", buf);
		delete[] buf;
	}

	return name;
}

GLuint GL_LinkProgram(GLuint vshader, GLuint fshader)
{
	GLuint name = sglCreateProgram();
	GLint status;
	sglAttachShader(name, vshader);
	sglAttachShader(name, fshader);
	sglLinkProgram(name);

	sglGetProgramiv(name, GL_LINK_STATUS, &status);
	if (!status)
	{
		GLint infoLogLength;
		sglGetProgramiv(name, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* buf = new char[infoLogLength];
		sglGetProgramInfoLog(name, infoLogLength, NULL, buf);

		fprintf(stderr, "Error linking program:\n%s\n", buf);
		delete[] buf;

		return 0;
	}

	return name;
}

void I_InitGLContext(SDL_Window *win)
{
	window = win;
	context = SDL_GL_CreateContext(win);
	if (!context)
	{
		Error("I_InitGLContext: Cannot create GL context: %s\n", SDL_GetError());
	}

	SDL_GL_MakeCurrent(win, context);

	//Context created, so start loading funcs
	//TODO: Is there a way to avoid the hideous casts in C++?
	sglGetError = (GLenum(APIENTRY*)())SDL_GL_GetProcAddress("glGetError");
	sglClear = (void (APIENTRY*)(GLbitfield mask))SDL_GL_GetProcAddress("glClear");
	sglClearColor = (void (APIENTRY*)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha))SDL_GL_GetProcAddress("glClearColor");
	sglViewport = (void (APIENTRY*)(GLint x, GLint y, GLsizei width, GLsizei height))SDL_GL_GetProcAddress("glViewport");
	sglCreateShader = (GLuint(APIENTRY*)(GLenum type))SDL_GL_GetProcAddress("glCreateShader");
	sglCreateProgram = (GLuint(APIENTRY*)())SDL_GL_GetProcAddress("glCreateProgram");
	sglDeleteProgram = (void(APIENTRY*)(GLuint program))SDL_GL_GetProcAddress("glDeleteProgram");
	sglDeleteShader = (void(APIENTRY*)(GLuint shader))SDL_GL_GetProcAddress("glDeleteShader");
	sglLinkProgram = (void(APIENTRY*)(GLuint program))SDL_GL_GetProcAddress("glLinkProgram");
	sglShaderSource = (void(APIENTRY*)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint * length))SDL_GL_GetProcAddress("glShaderSource");
	sglUseProgram = (void(APIENTRY*)(GLuint program))SDL_GL_GetProcAddress("glUseProgram");
	sglGetProgramiv = (void(APIENTRY*)(GLuint program, GLenum pname, GLint * params))SDL_GL_GetProcAddress("glGetProgramiv");
	sglGetProgramInfoLog = (void(APIENTRY*)(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog))SDL_GL_GetProcAddress("glGetProgramInfoLog");
	sglGetShaderiv = (void(APIENTRY*)(GLuint shader, GLenum pname, GLint * params))SDL_GL_GetProcAddress("glGetShaderiv");
	sglGetShaderInfoLog = (void(APIENTRY*)(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog))SDL_GL_GetProcAddress("glGetShaderInfoLog");
	sglGetShaderSource = (void(APIENTRY*)(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * source))SDL_GL_GetProcAddress("glGetShaderSource");
	sglGetUniformLocation = (GLint(APIENTRY*)(GLuint program, const GLchar * name))SDL_GL_GetProcAddress("glGetUniformLocation");
	sglAttachShader = (void(APIENTRY*)(GLuint program, GLuint shader))SDL_GL_GetProcAddress("glAttachShader");
	sglCompileShader = (void(APIENTRY*)(GLuint shader))SDL_GL_GetProcAddress("glCompileShader");
	sglUniform1f = (void (APIENTRY*)(GLint location, GLfloat v0))SDL_GL_GetProcAddress("glUniform1f");
	sglUniform2f = (void (APIENTRY*)(GLint location, GLfloat v0, GLfloat v1))SDL_GL_GetProcAddress("glUniform2f");
	sglUniform3f = (void (APIENTRY*)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2))SDL_GL_GetProcAddress("glUniform3f");
	sglUniform4f = (void (APIENTRY*)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3))SDL_GL_GetProcAddress("glUniform4f");
	sglUniform1i = (void (APIENTRY*)(GLint location, GLint v0))SDL_GL_GetProcAddress("glUniform1i");
	sglUniform2i = (void (APIENTRY*)(GLint location, GLint v0, GLint v1))SDL_GL_GetProcAddress("glUniform2i");
	sglUniform3i = (void (APIENTRY*)(GLint location, GLint v0, GLint v1, GLint v2))SDL_GL_GetProcAddress("glUniform3i");
	sglUniform4i = (void (APIENTRY*)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3))SDL_GL_GetProcAddress("glUniform4i");
	sglUniformMatrix2fv = (void (APIENTRY*)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value))SDL_GL_GetProcAddress("glUniformMatrix2fv");
	sglUniformMatrix3fv = (void (APIENTRY*)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value))SDL_GL_GetProcAddress("glUniformMatrix3fv");
	sglUniformMatrix4fv = (void (APIENTRY*)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value))SDL_GL_GetProcAddress("glUniformMatrix4fv");
	sglBindVertexArray = (void(APIENTRY*)(GLuint array))SDL_GL_GetProcAddress("glBindVertexArray");
	sglDeleteVertexArrays = (void(APIENTRY*)(GLsizei n, const GLuint * arrays))SDL_GL_GetProcAddress("glDeleteVertexArrays");
	sglGenVertexArrays = (void(APIENTRY*)(GLsizei n, GLuint * arrays))SDL_GL_GetProcAddress("glGenVertexArrays");
	sglDisableVertexAttribArray = (void(APIENTRY*)(GLuint index))SDL_GL_GetProcAddress("glDisableVertexAttribArray");
	sglEnableVertexAttribArray = (void(APIENTRY*)(GLuint index))SDL_GL_GetProcAddress("glEnableVertexAttribArray");
	sglVertexAttribPointer = (void(APIENTRY*)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer))SDL_GL_GetProcAddress("glVertexAttribPointer");
	sglBindBuffer = (void(APIENTRY*)(GLenum target, GLuint buffer))SDL_GL_GetProcAddress("glBindBuffer");
	sglDeleteBuffers = (void(APIENTRY*)(GLsizei n, const GLuint * buffers))SDL_GL_GetProcAddress("glDeleteBuffers");
	sglGenBuffers = (void(APIENTRY*)(GLsizei n, GLuint * buffers))SDL_GL_GetProcAddress("glGenBuffers");
	sglBufferData = (void(APIENTRY*)(GLenum target, GLsizeiptr size, const void* data, GLenum usage))SDL_GL_GetProcAddress("glBufferData");
	sglBufferSubData = (void(APIENTRY*)(GLenum target, GLintptr offset, GLsizeiptr size, const void* data))SDL_GL_GetProcAddress("glBufferSubData");
	sglBindTexture = (void (APIENTRY*)(GLenum target, GLuint texture))SDL_GL_GetProcAddress("glBindTexture");
	sglDeleteTextures = (void (APIENTRY*)(GLsizei n, const GLuint * textures))SDL_GL_GetProcAddress("glDeleteTextures");
	sglGenTextures = (void (APIENTRY*)(GLsizei n, GLuint * textures))SDL_GL_GetProcAddress("glGenTextures");
	sglActiveTexture = (void (APIENTRY*)(GLenum texture))SDL_GL_GetProcAddress("glActiveTexture");
	sglTexImage1D = (void (APIENTRY*)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels))SDL_GL_GetProcAddress("glTexImage1D");
	sglTexImage2D = (void (APIENTRY*)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels))SDL_GL_GetProcAddress("glTexImage2D");
	sglTexSubImage1D = (void (APIENTRY*)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels))SDL_GL_GetProcAddress("glTexSubImage1D");
	sglTexSubImage2D = (void (APIENTRY*)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels))SDL_GL_GetProcAddress("glTexSubImage2D");
	sglTexParameterf = (void (APIENTRY*)(GLenum target, GLenum pname, GLfloat param))SDL_GL_GetProcAddress("glTexParameterf");
	sglTexParameterfv = (void (APIENTRY*)(GLenum target, GLenum pname, const GLfloat * params))SDL_GL_GetProcAddress("glTexParameterfv");
	sglTexParameteri = (void (APIENTRY*)(GLenum target, GLenum pname, GLint param))SDL_GL_GetProcAddress("glTexParameteri");
	sglTexParameteriv = (void (APIENTRY*)(GLenum target, GLenum pname, const GLint * params))SDL_GL_GetProcAddress("glTexParameteriv");
	sglDrawArrays = (void (APIENTRY *)(GLenum mode, GLint first, GLsizei count))SDL_GL_GetProcAddress("glDrawArrays");

	//The window size is constant, so just do this now
	int w, h;
	SDL_GL_GetDrawableSize(win, &w, &h);
	sglViewport(0, 0, w, h);

	//Only need one VAO, so create that now
	sglGenVertexArrays(1, &vaoName);
	sglBindVertexArray(vaoName);

	//Textures shouldn't need to be recreated ever, so get that done too
	sglGenTextures(1, &paletteName);
	sglGenTextures(1, &sourceFBName);

	GL_ErrorCheck("Creating GL resources");

	//Bind and fill out initial data for the palette texture
	sglActiveTexture(GL_TEXTURE2);
	sglBindTexture(GL_TEXTURE_1D, paletteName);
	sglTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	sglTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	sglTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GL_ErrorCheck("Creating palette texture");

	//Bind the initial source framebuffer, but don't create it yet. It will be created when the video mode changes.
	sglActiveTexture(GL_TEXTURE0);
	sglBindTexture(GL_TEXTURE_2D, sourceFBName);

	//Compile the shaders and link the phase 1 program
	GLuint p1vert = GL_CompileShader(vertexSource, GL_VERTEX_SHADER);
	GLuint p1frag = GL_CompileShader(fragmentSource, GL_FRAGMENT_SHADER);
	GL_ErrorCheck("Compiling shaders");

	phase1ProgramName = GL_LinkProgram(p1vert, p1frag);

	if (!phase1ProgramName)
	{
		Error("I_InitGLContext: Can't link shader program.");
	}
	GL_ErrorCheck("Linking shaders");
	sglUseProgram(phase1ProgramName);

	sglDeleteShader(p1vert);
	sglDeleteShader(p1frag);

	//Create the VBO and create the vertex attributes.
	sglGenBuffers(1, &bufName);
	sglBindBuffer(GL_ARRAY_BUFFER, bufName);
	sglBufferData(GL_ARRAY_BUFFER, sizeof(buf), buf, GL_STATIC_DRAW);
	GL_ErrorCheck("Creating buffer");

	sglEnableVertexAttribArray(0); //position
	sglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
	sglEnableVertexAttribArray(1); //uv
	sglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const void*)(sizeof(float) * 2));
	GL_ErrorCheck("Enabling vertex attributes");

	int paletteUniform = sglGetUniformLocation(phase1ProgramName, "palette");
	sglUniform1i(paletteUniform, 2);
	int srcFBUniform = sglGetUniformLocation(phase1ProgramName, "srcfb");
	sglUniform1i(srcFBUniform, 0);

	SDL_GL_SetSwapInterval(SwapInterval);
}

extern unsigned char* gr_video_memory;
void GL_SetVideoMode(int w, int h, SDL_Rect *bounds)
{
	sglActiveTexture(GL_TEXTURE0);
	sglBindTexture(GL_TEXTURE_2D, sourceFBName);
	//I'd prefer immutable textures for this, but I'd rather have wider compatibility if possible
	//I've heard whispers that sampler objects perform better, so being able to use those too would be nice.

	//Create the texture with the current contents of video memory.
	//TODO: Do a GL version check and conditionally use immutable textures/samplers.
	sglTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, w, h, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, gr_video_memory);
	GL_ErrorCheck("Creating source framebuffer texture");
	sglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	sglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GL_ErrorCheck("Setting source framebuffer filter mode");

	//TODO: Does this need to be DPI aware in order to work on macs?
	sglViewport(bounds->x, bounds->y, bounds->w, bounds->h);
}

void GL_SetPalette(uint32_t* pal)
{
	sglActiveTexture(GL_TEXTURE2);
	sglTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pal);
}

void GL_DrawPhase1()
{
	SDL_GL_MakeCurrent(window, context);
	//Blit the current contents of the buffer
	sglActiveTexture(GL_TEXTURE0);
	//I need to explicitly rebind the texture any time we do something with it. 
	//Discord disrupts the binding at GL_TEXTURE0 when you start streaming using it.
	//I could move to a "safe" slot, but if this is a problem to contend with, maybe it's safer
	//to take the minor perf penalty and just make sure we're safely bound each frame. 
	sglBindTexture(GL_TEXTURE_2D, sourceFBName);

	sglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h, GL_RED_INTEGER, GL_UNSIGNED_BYTE, gr_video_memory);

	sglClear(GL_COLOR_BUFFER_BIT);
	sglDrawArrays(GL_TRIANGLE_FAN, 0, 3);
}

void I_ShutdownGL()
{
	if (context)
	{
		sglDisableVertexAttribArray(0);
		sglDisableVertexAttribArray(1);
		sglUseProgram(0);
		sglBindBuffer(GL_ARRAY_BUFFER, 0);
		sglActiveTexture(GL_TEXTURE2);
		sglBindTexture(GL_TEXTURE_1D, 0);
		sglActiveTexture(GL_TEXTURE0);
		sglBindTexture(GL_TEXTURE_2D, 0);

		sglDeleteProgram(phase1ProgramName);
		sglDeleteBuffers(1, &bufName);
		sglDeleteTextures(1, &paletteName);
		sglDeleteTextures(1, &sourceFBName);

		sglBindVertexArray(0);
		sglDeleteVertexArrays(1, &vaoName);
		SDL_GL_DeleteContext(context);
	}
}

//stuffing API funcs here for now...
GLenum(APIENTRY* sglGetError)();
void (APIENTRY* sglClear)(GLbitfield mask);
void (APIENTRY* sglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void (APIENTRY* sglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

//shaders
GLuint(APIENTRY* sglCreateShader)(GLenum type);
GLuint(APIENTRY* sglCreateProgram)();
void(APIENTRY* sglDeleteProgram)(GLuint program);
void(APIENTRY* sglDeleteShader)(GLuint shader);
void(APIENTRY* sglLinkProgram)(GLuint program);
void(APIENTRY* sglShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
void(APIENTRY* sglUseProgram)(GLuint program);
void(APIENTRY* sglGetProgramiv)(GLuint program, GLenum pname, GLint* params);
void(APIENTRY* sglGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
void(APIENTRY* sglGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
void(APIENTRY* sglGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
void(APIENTRY* sglGetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source);
GLint(APIENTRY* sglGetUniformLocation)(GLuint program, const GLchar* name);
void(APIENTRY* sglAttachShader)(GLuint program, GLuint shader);
void(APIENTRY* sglCompileShader)(GLuint shader);

//shader uniforms
void (APIENTRY* sglUniform1f)(GLint location, GLfloat v0);
void (APIENTRY* sglUniform2f)(GLint location, GLfloat v0, GLfloat v1);
void (APIENTRY* sglUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void (APIENTRY* sglUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void (APIENTRY* sglUniform1i)(GLint location, GLint v0);
void (APIENTRY* sglUniform2i)(GLint location, GLint v0, GLint v1);
void (APIENTRY* sglUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
void (APIENTRY* sglUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void (APIENTRY* sglUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void (APIENTRY* sglUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void (APIENTRY* sglUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

//Vertex arrays
void(APIENTRY* sglBindVertexArray)(GLuint array);
void(APIENTRY* sglDeleteVertexArrays)(GLsizei n, const GLuint* arrays);
void(APIENTRY* sglGenVertexArrays)(GLsizei n, GLuint* arrays);
void(APIENTRY* sglDisableVertexAttribArray)(GLuint index);
void(APIENTRY* sglEnableVertexAttribArray)(GLuint index);
void(APIENTRY* sglVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

void(APIENTRY* sglBindBuffer)(GLenum target, GLuint buffer);
void(APIENTRY* sglDeleteBuffers)(GLsizei n, const GLuint* buffers);
void(APIENTRY* sglGenBuffers)(GLsizei n, GLuint* buffers);
void(APIENTRY* sglBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
void(APIENTRY* sglBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);

void (APIENTRY* sglBindTexture)(GLenum target, GLuint texture);
void (APIENTRY* sglDeleteTextures)(GLsizei n, const GLuint* textures);
void (APIENTRY* sglGenTextures)(GLsizei n, GLuint* textures);
void (APIENTRY* sglActiveTexture)(GLenum texture);
void (APIENTRY* sglTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
void (APIENTRY* sglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
void (APIENTRY* sglTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid* pixels);
void (APIENTRY* sglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
void (APIENTRY* sglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
void (APIENTRY* sglTexParameterfv)(GLenum target, GLenum pname, const GLfloat* params);
void (APIENTRY* sglTexParameteri)(GLenum target, GLenum pname, GLint param);
void (APIENTRY* sglTexParameteriv)(GLenum target, GLenum pname, const GLint* params);

void (APIENTRY* sglDrawArrays)(GLenum mode, GLint first, GLsizei count);
