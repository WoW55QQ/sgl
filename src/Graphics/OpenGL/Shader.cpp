/*
 * Shader.cpp
 *
 *  Created on: 15.01.2015
 *      Author: Christoph Neuhauser
 */

#include <GL/glew.h>
#include "Shader.hpp"
#include "SystemGL.hpp"
#include "RendererGL.hpp"
#include "Texture.hpp"
#include "ShaderAttributes.hpp"
#include <Utils/File/Logfile.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sgl {

ShaderGL::ShaderGL(ShaderType _shaderType) : shaderType(_shaderType)
{
	if (shaderType == VERTEX_SHADER) {
		shaderID = glCreateShader(GL_VERTEX_SHADER);
	} else if (shaderType == FRAGMENT_SHADER) {
		shaderID = glCreateShader(GL_FRAGMENT_SHADER);
	} else if (shaderType == GEOMETRY_SHADER) {
		shaderID = glCreateShader(GL_GEOMETRY_SHADER);
	} else if (shaderType == TESSELATION_EVALUATION_SHADER) {
		shaderID = glCreateShader(GL_TESS_EVALUATION_SHADER);
	} else if (shaderType == TESSELATION_CONTROL_SHADER) {
		shaderID = glCreateShader(GL_TESS_CONTROL_SHADER);
	} else if (shaderType == COMPUTE_SHADER) {
		shaderID = glCreateShader(GL_COMPUTE_SHADER);
	}
}

ShaderGL::~ShaderGL()
{
	glDeleteShader(shaderID);
}

void ShaderGL::setShaderText(const std::string &text)
{
	// Upload the shader text to the graphics card
	const GLchar *stringPtrs[1];
	stringPtrs[0] = text.c_str();
	glShaderSource(shaderID, 1, stringPtrs, NULL);
}

bool ShaderGL::compile()
{
	glCompileShader(shaderID);
	GLint succes;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &succes);
	if (!succes) {
		GLint infoLogLength;
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar *infoLog = new GLchar[infoLogLength+1];
		glGetShaderInfoLog(shaderID, infoLogLength, NULL, infoLog);
		Logfile::get()->writeError(std::string() + "ERROR: ShaderGL::compile: Cannot compile shader! fileID: \"" + fileID + "\"");
		Logfile::get()->writeError(std::string() + "OpenGL Error: " + infoLog);
		delete[] infoLog;
		return false;
	}
	return true;
}

// Returns e.g. "Fragment Shader" for logging purposes
std::string ShaderGL::getShaderDebugType()
{
	std::string type;
	if (shaderType == VERTEX_SHADER) {
		type = "Vertex Shader";
	} else if (shaderType == FRAGMENT_SHADER) {
		type = "Fragment Shader";
	} else if (shaderType == GEOMETRY_SHADER) {
		type = "Geometry Shader";
	} else if (shaderType == TESSELATION_EVALUATION_SHADER) {
		type = "Tesselation Evaluation Shader";
	} else if (shaderType == TESSELATION_CONTROL_SHADER) {
		type = "Tesselation Control Shader";
	} else if (shaderType == COMPUTE_SHADER) {
		type = "Compute Shader";
	}
	return type;
}




// ------------------------- Shader Program -------------------------

ShaderProgramGL::ShaderProgramGL()
{
	shaderProgramID = glCreateProgram();
}

ShaderProgramGL::~ShaderProgramGL()
{
	glDeleteProgram(shaderProgramID);
}


bool ShaderProgramGL::linkProgram()
{
	// 1. Link the shader program
	glLinkProgram(shaderProgramID);
	GLint success = 0;
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &success);
	if (!success) {
		GLint infoLogLength;
		glGetProgramiv(shaderProgramID, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar *infoLog = new GLchar[infoLogLength+1];
		glGetProgramInfoLog(shaderProgramID, infoLogLength, NULL, infoLog);
		Logfile::get()->writeError("Error: Cannot link shader program!");
		Logfile::get()->writeError(std::string() + "OpenGL Error: " + infoLog);
		Logfile::get()->writeError("fileIDs of the linked shaders:");
		for (auto it = shaders.begin(); it != shaders.end(); ++it) {
			ShaderGL *shaderGL = (ShaderGL*)it->get();
			std::string type = shaderGL->getShaderDebugType();
			Logfile::get()->writeError(std::string() + "\"" + shaderGL->getFileID() + "\" (Type: " + type + ")");
		}
		delete[] infoLog;
		return false;
	}

	return true;
}

bool ShaderProgramGL::validateProgram()
{
	// 2. Validation
	glValidateProgram(shaderProgramID);
	GLint success = 0;
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &success);
	if (!success) {
		GLint infoLogLength;
		glGetProgramiv(shaderProgramID, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar *infoLog = new GLchar[infoLogLength+1];
		glGetProgramInfoLog(shaderProgramID, infoLogLength, NULL, infoLog);
		Logfile::get()->writeError("Error in shader program validation!");
		Logfile::get()->writeError(std::string() + "OpenGL Error: " + infoLog);
		Logfile::get()->writeError("fileIDs of the linked shaders:");
		for (std::list<ShaderPtr>::iterator it = shaders.begin(); it != shaders.end(); ++it) {
			ShaderGL *shaderGL = (ShaderGL*)it->get();
			std::string type = shaderGL->getShaderDebugType();
			Logfile::get()->writeError(std::string() + "\"" + shaderGL->getFileID() + "\" (Type: " + type + ")");
		}
		delete[] infoLog;
		return false;
	}

	return true;
}

void ShaderProgramGL::attachShader(ShaderPtr shader)
{
	ShaderGL *shaderGL = (ShaderGL*)shader.get();
	glAttachShader(shaderProgramID, shaderGL->getShaderID());
	shaders.push_back(shader);
}

void ShaderProgramGL::detachShader(ShaderPtr shader)
{
	ShaderGL *shaderGL = (ShaderGL*)shader.get();
	glDetachShader(shaderProgramID, shaderGL->getShaderID());
	for (auto it = shaders.begin(); it != shaders.end(); ++it) {
		if (shader == *it) {
			shaders.erase(it);
			break;
		}
	}
}

void ShaderProgramGL::bind()
{
	RendererGL *rendererGL = (RendererGL*)Renderer;
	rendererGL->useShaderProgram(this);
}


bool ShaderProgramGL::hasUniform(const char *name)
{
	return getUniformLoc(name) >= 0;
}

int ShaderProgramGL::getUniformLoc(const char *name)
{
	auto it = uniforms.find(name);
	if (it != uniforms.end()) {
		return it->second;
	}

	GLint uniformLoc = glGetUniformLocation(shaderProgramID, name);
	if (uniformLoc == -1) {
		return -1;
	}
	uniforms[name] = uniformLoc;
	return uniformLoc;
}

int ShaderProgramGL::getUniformLoc_error(const char *name)
{
	int location = getUniformLoc(name);
	if (location == -1) {
		Logfile::get()->writeError(std::string() + "ERROR: ShaderProgramGL::setUniform: "
				+ "No uniform variable called \"" + name + "\" in this shader program.");
	}
	return location;
}

bool ShaderProgramGL::setUniform(const char *name, int value)
{
	return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, bool value)
{
	return setUniform(getUniformLoc_error(name), (int)value);
}

bool ShaderProgramGL::setUniform(const char *name, float value)
{
	return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::vec2 &value)
{
	return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::vec3 &value)
{
	return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::vec4 &value)
{
	return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::mat4 &value)
{
	return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::mat3 &value)
{
	return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, const glm::mat3x4 &value)
{
	return setUniform(getUniformLoc_error(name), value);
}

bool ShaderProgramGL::setUniform(const char *name, TexturePtr &value, int textureUnit /* = 0 */)
{
	return setUniform(getUniformLoc_error(name), value, textureUnit);
}

bool ShaderProgramGL::setUniform(const char *name, const Color &value)
{
	return setUniform(getUniformLoc_error(name), value);
}


bool ShaderProgramGL::setUniformArray(const char *name, const int *value, size_t num)
{
	return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const bool *value, size_t num)
{
	return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const float *value, size_t num)
{
	return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const glm::vec2 *value, size_t num)
{
	return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const glm::vec3 *value, size_t num)
{
	return setUniformArray(getUniformLoc_error(name), value, num);
}

bool ShaderProgramGL::setUniformArray(const char *name, const glm::vec4 *value, size_t num)
{
	return setUniformArray(getUniformLoc_error(name), value, num);
}



bool ShaderProgramGL::setUniform(int location, int value)
{
	bind();
	glUniform1i(location, value);
	return true;
}

bool ShaderProgramGL::setUniform(int location, float value)
{
	bind();
	glUniform1f(location, value);
	return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::vec2 &value)
{
	bind();
	glUniform2f(location, value.x, value.y);
	return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::vec3 &value)
{
	bind();
	glUniform3f(location, value.x, value.y, value.z);
	return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::vec4 &value)
{
	bind();
	glUniform4f(location, value.x, value.y, value.z, value.w);
	return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::mat3 &value)
{
	bind();
	glUniformMatrix3fv(location, 1, false, glm::value_ptr(value));
	return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::mat3x4 &value)
{
	bind();
	glUniformMatrix3x4fv(location, 1, false, glm::value_ptr(value));
	return true;
}

bool ShaderProgramGL::setUniform(int location, const glm::mat4 &value)
{
	bind();
	glUniformMatrix4fv(location, 1, false, glm::value_ptr(value));
	return true;
}

bool ShaderProgramGL::setUniform(int location, TexturePtr &value, int textureUnit /* = 0 */)
{
	bind();
	Renderer->bindTexture(value, textureUnit);
	glUniform1i(location, textureUnit);
	return true;
}

bool ShaderProgramGL::setUniform(int location, const Color &value)
{
	bind();
	float color[] = { value.getFloatR(), value.getFloatG(), value.getFloatB(), value.getFloatA() };
	glUniform4fv(location, 1, color);
	return true;
}


bool ShaderProgramGL::setUniformArray(int location, const int *value, size_t num)
{
	bind();
	glUniform1iv(location, num, value);
	return true;
}

bool ShaderProgramGL::setUniformArray(int location, const bool *value, size_t num)
{
	bind();
	glUniform1iv(location, num, (int*)value);
	return true;
}

bool ShaderProgramGL::setUniformArray(int location, const float *value, size_t num)
{
	bind();
	glUniform1fv(location, num, value);
	return true;
}

bool ShaderProgramGL::setUniformArray(int location, const glm::vec2 *value, size_t num)
{
	bind();
	glUniform2fv(location, num, (float*)value);
	return true;
}

bool ShaderProgramGL::setUniformArray(int location, const glm::vec3 *value, size_t num)
{
	bind();
	glUniform3fv(location, num, (float*)value);
	return true;
}

bool ShaderProgramGL::setUniformArray(int location, const glm::vec4 *value, size_t num)
{
	bind();
	glUniform4fv(location, num, (float*)value);
	return true;
}



// OpenGL 3 Uniform Buffers & OpenGL 4 Shader Storage Buffers
bool ShaderProgramGL::setUniformBuffer(int binding, int location, GeometryBufferPtr &geometryBuffer)
{
	GLuint ubo = static_cast<GeometryBufferGL*>(geometryBuffer.get())->getBuffer();

	// Binding point is unique for _all_ shaders
	glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo);

	// Location is set per shader (by name, explicitly, or by layout modifier)
	glUniformBlockBinding(shaderProgramID, location, binding);

	return true;
}

bool ShaderProgramGL::setUniformBuffer(int binding, const char *name, GeometryBufferPtr &geometryBuffer)
{
	// Block index (aka location in the shader) can be queried by name in the shader
	unsigned int blockIndex = glGetUniformBlockIndex(shaderProgramID, name);
	return setUniformBuffer(binding, blockIndex, geometryBuffer);
}


bool ShaderProgramGL::setShaderStorageBuffer(int binding, int location, GeometryBufferPtr &geometryBuffer)
{
	GLuint ssbo = static_cast<GeometryBufferGL*>(geometryBuffer.get())->getBuffer();

	// Binding point is unique for _all_ shaders
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);

	// Set location to resource index per shader
	glShaderStorageBlockBinding(shaderProgramID, location, binding);

	return true;
}

bool ShaderProgramGL::setShaderStorageBuffer(int binding, const char *name, GeometryBufferPtr &geometryBuffer)
{
	// Resource index (aka location in the shader) can be queried by name in the shader
	unsigned int resourceIndex = glGetProgramResourceIndex(shaderProgramID, GL_SHADER_STORAGE_BLOCK, name);
	return setShaderStorageBuffer(binding, resourceIndex, geometryBuffer);
}

}
