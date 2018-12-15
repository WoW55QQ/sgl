/*!
 * Shader.hpp
 *
 *  Created on: 15.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_OPENGL_SHADER_HPP_
#define GRAPHICS_OPENGL_SHADER_HPP_

#include <Graphics/Shader/Shader.hpp>
#include <GL/glew.h>
#include <list>
#include <map>

namespace sgl {

class DLL_OBJECT ShaderGL : public Shader
{
public:
	ShaderGL(ShaderType _shaderType);
	~ShaderGL();
	void setShaderText(const std::string &text);
	bool compile();

	//! Implementation dependent
	inline GLuint getShaderID() const { return shaderID; }
	inline GLuint getShaderType() const { return shaderType; }
	//! Returns e.g. "Fragment Shader" for logging purposes
	std::string getShaderDebugType();

private:
	GLuint shaderID;
	ShaderType shaderType;
};

typedef boost::shared_ptr<Shader> ShaderPtr;

class DLL_OBJECT ShaderProgramGL : public ShaderProgram
{
public:
	ShaderProgramGL();
	~ShaderProgramGL();

	bool linkProgram();
	bool validateProgram();
	void attachShader(ShaderPtr shader);
	void detachShader(ShaderPtr shader);
	void bind();

	// Compute shader interface
	void dispatchCompute(int numGroupsX, int numGroupsY, int numGroupsZ = 1);

	bool hasUniform(const char *name);
	int getUniformLoc(const char *name);
    bool setUniform(const char *name, int value);
    bool setUniform(const char *name, const glm::ivec2 &value);
    bool setUniform(const char *name, const glm::ivec3 &value);
    bool setUniform(const char *name, const glm::ivec4 &value);
    bool setUniform(const char *name, unsigned int value);
    bool setUniform(const char *name, const glm::uvec2 &value);
    bool setUniform(const char *name, const glm::uvec3 &value);
    bool setUniform(const char *name, const glm::uvec4 &value);
    bool setUniform(const char *name, bool value);
    bool setUniform(const char *name, const glm::bvec2 &value);
    bool setUniform(const char *name, const glm::bvec3 &value);
    bool setUniform(const char *name, const glm::bvec4 &value);
	bool setUniform(const char *name, float value);
	bool setUniform(const char *name, const glm::vec2 &value);
	bool setUniform(const char *name, const glm::vec3 &value);
	bool setUniform(const char *name, const glm::vec4 &value);
	bool setUniform(const char *name, const glm::mat3 &value);
	bool setUniform(const char *name, const glm::mat3x4 &value);
	bool setUniform(const char *name, const glm::mat4 &value);
	bool setUniform(const char *name, TexturePtr &value, int textureUnit = 0);
	bool setUniform(const char *name, const Color &value);
	bool setUniformArray(const char *name, const int *value, size_t num);
	bool setUniformArray(const char *name, const unsigned int *value, size_t num);
	bool setUniformArray(const char *name, const bool *value, size_t num);
	bool setUniformArray(const char *name, const float *value, size_t num);
	bool setUniformArray(const char *name, const glm::vec2 *value, size_t num);
	bool setUniformArray(const char *name, const glm::vec3 *value, size_t num);
	bool setUniformArray(const char *name, const glm::vec4 *value, size_t num);

    bool setUniform(int location, int value);
    bool setUniform(int location, const glm::ivec2 &value);
    bool setUniform(int location, const glm::ivec3 &value);
    bool setUniform(int location, const glm::ivec4 &value);
    bool setUniform(int location, unsigned int value);
    bool setUniform(int location, const glm::uvec2 &value);
    bool setUniform(int location, const glm::uvec3 &value);
    bool setUniform(int location, const glm::uvec4 &value);
    bool setUniform(int location, bool value);
    bool setUniform(int location, const glm::bvec2 &value);
    bool setUniform(int location, const glm::bvec3 &value);
    bool setUniform(int location, const glm::bvec4 &value);
	bool setUniform(int location, float value);
	bool setUniform(int location, const glm::vec2 &value);
	bool setUniform(int location, const glm::vec3 &value);
	bool setUniform(int location, const glm::vec4 &value);
	bool setUniform(int location, const glm::mat3 &value);
	bool setUniform(int location, const glm::mat3x4 &value);
	bool setUniform(int location, const glm::mat4 &value);
	bool setUniform(int location, TexturePtr &value, int textureUnit = 0);
	bool setUniform(int location, const Color &value);
	bool setUniformArray(int location, const int *value, size_t num);
	bool setUniformArray(int location, const unsigned int *value, size_t num);
	bool setUniformArray(int location, const bool *value, size_t num);
	bool setUniformArray(int location, const float *value, size_t num);
	bool setUniformArray(int location, const glm::vec2 *value, size_t num);
	bool setUniformArray(int location, const glm::vec3 *value, size_t num);
	bool setUniformArray(int location, const glm::vec4 *value, size_t num);


	// Image Load and Store

	/**
	 * Binds a level of a texture to a uniform image unit in a shader.
	 * For more details see: https://www.khronos.org/opengl/wiki/GLAPI/glBindImageTexture
	 * @param unit: The binding in the shader to which the image should be attached.
	 * @param texture: The texture to bind an image from.
	 * @param format: The format used when performing formatted stores to the image.
	 * @param access: GL_READ_ONLY, GL_WRITE_ONLY, or GL_READ_WRITE.
	 * @param level: The level of a texture (usually of a mip-map) to be bound.
	 * @param layered: When using a layered texture (e.g. GL_TEXTURE_2D_ARRAY) whether all layers should be bound.
	 * @param layer: The layer to bind if "layered" is false.
	 */
	void setUniformImageTexture(unsigned int unit, TexturePtr texture, unsigned int format = GL_RGBA8,
								unsigned int access = GL_READ_WRITE, unsigned int level = 0,
								bool layered = false, unsigned int layer = 0);


	// OpenGL 3 Uniform Buffers & OpenGL 4 Shader Storage Buffers

	/**
	 * UBOs:
	 * - Binding: A global slot for UBOs in the OpenGL context.
	 * - Location (aka block index): The location of the referenced UBO within the shader.
	 * Instead of location, one can also use the name of the UBO within the shader to reference it.
	 * TODO: Outsource binding to Shader Manager (as shader programs have shared bindings).
	 */
	bool setUniformBuffer(int binding, int location, GeometryBufferPtr &geometryBuffer);
	bool setUniformBuffer(int binding, const char *name, GeometryBufferPtr &geometryBuffer);

	/**
	 * Atomic counters (GL_ATOMIC_COUNTER_BUFFER)
	 * https://www.khronos.org/opengl/wiki/Atomic_Counter
	 * - Binding: A global slot for atomic counter buffers in the OpenGL context.
	 * - Location: Not possible to specify. Oddly, only supported for uniform buffers and SSBOs in OpenGl specification.
	 * TODO: Outsource binding to Shader Manager (as shader programs have shared bindings).
	 */
	bool setAtomicCounterBuffer(int binding, GeometryBufferPtr &geometryBuffer);
	//bool setAtomicCounterBuffer(int binding, const char *name, GeometryBufferPtr &geometryBuffer);

	/**
	 * SSBOs:
	 * - Binding: A global slot for SSBOs in the OpenGL context.
	 * - Location (aka resource index): The location of the referenced SSBO within the shader.
	 * Instead of location, one can also use the name of the SSBO within the shader to reference it.
	 * TODO: Outsource binding to Shader Manager (as shader programs have shared bindings).
	 */
	bool setShaderStorageBuffer(int binding, int location, GeometryBufferPtr &geometryBuffer);
	bool setShaderStorageBuffer(int binding, const char *name, GeometryBufferPtr &geometryBuffer);

	inline GLuint getShaderProgramID() { return shaderProgramID; }

private:
	//! Prints an error message if the uniform doesn't exist
	int getUniformLoc_error(const char *name);
	std::map<std::string, int> uniforms;
	std::map<std::string, int> attributes;
	GLuint shaderProgramID;
};

}

/*! GRAPHICS_OPENGL_SHADER_HPP_ */
#endif
