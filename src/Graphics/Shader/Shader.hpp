/*!
 * Shader.hpp
 *
 *  Created on: 15.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_SHADER_SHADER_HPP_
#define GRAPHICS_SHADER_SHADER_HPP_

#include <string>
#include <list>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <glm/fwd.hpp>

#include <Defs.hpp>
#include <Graphics/Buffers/GeometryBuffer.hpp>

namespace sgl {

enum DLL_OBJECT ShaderType {
	VERTEX_SHADER, FRAGMENT_SHADER, GEOMETRY_SHADER,
	TESSELATION_EVALUATION_SHADER, TESSELATION_CONTROL_SHADER, COMPUTE_SHADER
};

class ShaderAttributes;
typedef boost::shared_ptr<ShaderAttributes> ShaderAttributesPtr;
class Color;
class Texture;
typedef boost::shared_ptr<Texture> TexturePtr;

//! A single shader, e.g. fragment (pixel) shader, vertex shader, geometry shader, ...
class DLL_OBJECT Shader
{
public:
	Shader() {}
	virtual ~Shader() {}
	virtual void setShaderText(const std::string &text)=0;
	virtual bool compile()=0;

	// The identifier used for loading the shader, e.g. "Blit.Vertex"
	inline const char *getFileID() const { return fileID.c_str(); }
	inline void setFileID(const std::string &_fileID) { fileID = _fileID; }

protected:
	std::string fileID;
};

typedef boost::shared_ptr<Shader> ShaderPtr;

//! The shader program is the sum of the different shaders attached and linked together
class DLL_OBJECT ShaderProgram
{
public:
	ShaderProgram() {}
	virtual ~ShaderProgram() {}
	inline std::list<ShaderPtr> &getShaderList() { return shaders; }

	virtual void attachShader(ShaderPtr shader)=0;
	virtual void detachShader(ShaderPtr shader)=0;
	virtual bool linkProgram()=0;
	virtual bool validateProgram()=0;
	virtual void bind()=0;

	//! Uniform variables are shared between different executions of a shader program
	virtual bool hasUniform(const char *name)=0;
	virtual int getUniformLoc(const char *name)=0;
    virtual bool setUniform(const char *name, int value)=0;
    virtual bool setUniform(const char *name, unsigned int value)=0;
	virtual bool setUniform(const char *name, bool value)=0;
	virtual bool setUniform(const char *name, float value)=0;
	virtual bool setUniform(const char *name, const glm::vec2 &value)=0;
	virtual bool setUniform(const char *name, const glm::vec3 &value)=0;
	virtual bool setUniform(const char *name, const glm::vec4 &value)=0;
	virtual bool setUniform(const char *name, const glm::mat3 &value)=0;
	virtual bool setUniform(const char *name, const glm::mat3x4 &value)=0;
	virtual bool setUniform(const char *name, const glm::mat4 &value)=0;
	virtual bool setUniform(const char *name, TexturePtr &value, int textureUnit = 0)=0;
	virtual bool setUniform(const char *name, const Color &value)=0;
    virtual bool setUniformArray(const char *name, const int *value, size_t num)=0;
    virtual bool setUniformArray(const char *name, const unsigned int *value, size_t num)=0;
	virtual bool setUniformArray(const char *name, const bool *value, size_t num)=0;
	virtual bool setUniformArray(const char *name, const float *value, size_t num)=0;
	virtual bool setUniformArray(const char *name, const glm::vec2 *value, size_t num)=0;
	virtual bool setUniformArray(const char *name, const glm::vec3 *value, size_t num)=0;
	virtual bool setUniformArray(const char *name, const glm::vec4 *value, size_t num)=0;

    virtual bool setUniform(int location, int value)=0;
    virtual bool setUniform(int location, unsigned int value)=0;
	virtual bool setUniform(int location, float value)=0;
	virtual bool setUniform(int location, const glm::vec2 &value)=0;
	virtual bool setUniform(int location, const glm::vec3 &value)=0;
	virtual bool setUniform(int location, const glm::vec4 &value)=0;
	virtual bool setUniform(int location, const glm::mat3 &value)=0;
	virtual bool setUniform(int location, const glm::mat3x4 &value)=0;
	virtual bool setUniform(int location, const glm::mat4 &value)=0;
	virtual bool setUniform(int location, TexturePtr &value, int textureUnit = 0)=0;
	virtual bool setUniform(int location, const Color &value)=0;
    virtual bool setUniformArray(int location, const int *value, size_t num)=0;
    virtual bool setUniformArray(int location, const unsigned int *value, size_t num)=0;
	virtual bool setUniformArray(int location, const bool *value, size_t num)=0;
	virtual bool setUniformArray(int location, const float *value, size_t num)=0;
	virtual bool setUniformArray(int location, const glm::vec2 *value, size_t num)=0;
	virtual bool setUniformArray(int location, const glm::vec3 *value, size_t num)=0;
	virtual bool setUniformArray(int location, const glm::vec4 *value, size_t num)=0;


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
	virtual void setUniformImageTexture(unsigned int unit, TexturePtr texture, unsigned int format = 0x8058 /*GL_RGBA8*/,
								unsigned int access = 0x88BA /*GL_READ_WRITE*/, unsigned int level = 0,
								bool layered = false, unsigned int layer = 0)=0;


	// OpenGL 3 Uniform Buffers & OpenGL 4 Shader Storage Buffers

	/**
	 * UBOs:
	 * - Binding: A global slot for UBOs in the OpenGL context.
	 * - Location (aka block index): The location of the referenced UBO within the shader.
	 * Instead of location, one can also use the name of the UBO within the shader to reference it.
	 * TODO: Outsource binding to Shader Manager (as shader programs have shared bindings).
	 */
	virtual bool setUniformBuffer(int binding, int location, GeometryBufferPtr &geometryBuffer)=0;
	virtual bool setUniformBuffer(int binding, const char *name, GeometryBufferPtr &geometryBuffer)=0;

	/**
	 * Atomic counters (GL_ATOMIC_COUNTER_BUFFER)
	 * https://www.khronos.org/opengl/wiki/Atomic_Counter
	 * - Binding: A global slot for atomic counter buffers in the OpenGL context.
	 * - Location: Not possible to specify. Oddly, only supported for uniform buffers and SSBOs in OpenGl specification.
	 * TODO: Outsource binding to Shader Manager (as shader programs have shared bindings).
	 */
	virtual bool setAtomicCounterBuffer(int binding, GeometryBufferPtr &geometryBuffer)=0;
	//virtual bool setAtomicCounterBuffer(int binding, const char *name, GeometryBufferPtr &geometryBuffer)=0;

	/**
	 * SSBOs:
	 * - Binding: A global slot for SSBOs in the OpenGL context.
	 * - Location (aka resource index): The location of the referenced SSBO within the shader.
	 * Instead of location, one can also use the name of the SSBO within the shader to reference it.
	 * TODO: Outsource binding to Shader Manager (as shader programs have shared bindings).
	 */
	virtual bool setShaderStorageBuffer(int binding, int location, GeometryBufferPtr &geometryBuffer)=0;
	virtual bool setShaderStorageBuffer(int binding, const char *name, GeometryBufferPtr &geometryBuffer)=0;

protected:
	std::list<ShaderPtr> shaders;
};

typedef boost::shared_ptr<ShaderProgram> ShaderProgramPtr;
typedef boost::weak_ptr<ShaderProgram> WeakShaderProgramPtr;

}

/*! GRAPHICS_SHADER_SHADER_HPP_ */
#endif
