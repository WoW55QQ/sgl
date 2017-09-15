/*
 * RendererGL.cpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph
 */

#include <GL/glew.h>
#include "RendererGL.hpp"
#include <Math/Geometry/Point2.hpp>
#include <Math/Geometry/MatrixUtil.hpp>
#include <Utils/Convert.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Mesh/Vertex.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/Scene/Camera.hpp>
#include <Graphics/Scene/RenderTarget.hpp>
#include "SystemGL.hpp"
#include "FBO.hpp"
#include "RBO.hpp"
#include "ShaderManager.hpp"
#include "ShaderAttributes.hpp"
#include "GeometryBuffer.hpp"
#include "Texture.hpp"

namespace sgl {

RendererGL::RendererGL()
{
	blendMode = BLEND_OVERWRITE;
	setBlendMode(BLEND_ALPHA);
	boundTextureID.resize(32, 0);

	modelMatrix = viewMatrix = projectionMatrix = viewProjectionMatrix = mvpMatrix = matrixIdentity();
	lineWidth = 1.0f;
	pointSize = 1.0f;
	currentTextureUnit = 0;
	boundFBOID = 0;
	boundVAO = 0;
	boundShader = 0;
	wireframeMode = false;
	debugOutputExtEnabled = false;

	fxaaShader = ShaderManager->getShaderProgram({"FXAA.Vertex", "FXAA.Fragment"});
	blurShader = ShaderManager->getShaderProgram({"GaussianBlur.Vertex", "GaussianBlur.Fragment"});
	blitShader = ShaderManager->getShaderProgram({"Blit.Vertex", "Blit.Fragment"});
	resolveMSAAShader = ShaderManager->getShaderProgram({"ResolveMSAA.Vertex.GL3", "ResolveMSAA.Fragment.GL3"});
	solidShader = ShaderManager->getShaderProgram({"Mesh.Vertex.Plain", "Mesh.Fragment.Plain"});
	whiteShader = ShaderManager->getShaderProgram({"WhiteSolid.Vertex", "WhiteSolid.Fragment"});

	if (SystemGL::get()->isGLExtensionAvailable("ARB_debug_output")
			|| SystemGL::get()->isGLExtensionAvailable("KHR_debug")) {
		glEnable(GL_DEBUG_OUTPUT);
		debugOutputExtEnabled = true;
	}
}

std::vector<std::string> getErrorMessages() {
	int numMessages = 10;

	GLint maxMessageLen = 0;
	glGetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH, &maxMessageLen);

	std::vector<GLchar> messageText(numMessages * maxMessageLen);
	std::vector<GLenum> sources(numMessages);
	std::vector<GLenum> types(numMessages);
	std::vector<GLuint> ids(numMessages);
	std::vector<GLenum> severities(numMessages);
	std::vector<GLsizei> lengths(numMessages);

	GLuint numFound = glGetDebugMessageLog(numMessages, messageText.capacity(), &sources[0],
			&types[0], &ids[0], &severities[0], &lengths[0], &messageText[0]);

	sources.resize(numFound);
	types.resize(numFound);
	severities.resize(numFound);
	ids.resize(numFound);
	lengths.resize(numFound);

	std::vector<std::string> messages;
	messages.reserve(numFound);

	std::vector<GLchar>::iterator currPos = messageText.begin();
	for(size_t i = 0; i < lengths.size(); ++i) {
		messages.push_back(std::string(currPos, currPos + lengths[i] - 1));
		currPos = currPos + lengths[i];
	}

	return messages;
}

void RendererGL::errorCheck()
{
	// Check for errors
	GLenum oglError = glGetError();
	if (oglError != GL_NO_ERROR) {
		Logfile::get()->writeError(std::string() + "OpenGL error: " + toString(oglError));

		auto messages = getErrorMessages();
		for (const std::string &msg : messages) {
			Logfile::get()->writeError(std::string() + "Error message: " + msg);
		}
	}
}

// Creation functions
FramebufferObjectPtr RendererGL::createFBO()
{
	if (SystemGL::get()->openglVersionMinimum(3,2)) {
		return FramebufferObjectPtr(new FramebufferObjectGL);
	}
	return FramebufferObjectPtr(new FramebufferObjectGL2);
}

RenderbufferObjectPtr RendererGL::createRBO(int _width, int _height, RenderbufferType rboType, int _samples /* = 0 */)
{
	return RenderbufferObjectPtr(new RenderbufferObjectGL(_width, _height, rboType, _samples));
}

GeometryBufferPtr RendererGL::createGeometryBuffer(size_t size, BufferType type /* = VERTEX_BUFFER */, BufferUse bufferUse /* = BUFFER_STATIC */)
{
	GeometryBufferPtr geomBuffer(new GeometryBufferGL(size, type, bufferUse));
	return geomBuffer;
}

GeometryBufferPtr RendererGL::createGeometryBuffer(size_t size, void *data, BufferType type /* = VERTEX_BUFFER */, BufferUse bufferUse /* = BUFFER_STATIC */)
{
	GeometryBufferPtr geomBuffer(new GeometryBufferGL(size, data, type, bufferUse));
	return geomBuffer;
}


// Functions for managing viewports/render targets
void RendererGL::bindFBO(FramebufferObjectPtr _fbo, bool force /* = false */)
{
	if (boundFBO.get() != _fbo.get() || force) {
		boundFBO = _fbo;
		if (_fbo.get() != NULL) {
			boundFBOID = _fbo->_bindInternal();
		} else {
			unbindFBO(force);
		}
	}
}

void RendererGL::unbindFBO(bool force /* = false */)
{
	if (boundFBO.get() != 0 || force) {
		boundFBO = FramebufferObjectPtr();
		boundFBOID = 0;
		if (SystemGL::get()->openglVersionMinimum(3,2)) {
			glBindFramebuffer(GL_FRAMEBUFFER, boundFBOID);
		} else {
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, boundFBOID);
		}
	}
}

FramebufferObjectPtr RendererGL::getFBO()
{
	return boundFBO;
}

void RendererGL::clearFramebuffer(unsigned int buffers /* = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT */,
		const Color& col /* = Color(0, 0, 0) */, float depth /* = 0.0f */, unsigned short stencil /* = 0*/ )
{
	glClearColor(col.getFloatR(), col.getFloatG(), col.getFloatB(), col.getFloatA());
	glClearDepth(depth);
	glClearStencil(stencil);
	glClear(buffers);
}

void RendererGL::setCamera(CameraPtr _camera, bool force)
{
	if (camera != _camera || force) {
		camera = _camera;
		glm::ivec4 ltwh =  camera->getViewportLTWH();
		glViewport(ltwh.x, ltwh.y, ltwh.z, ltwh.w); // left, top, width, height

		RenderTargetPtr target = camera->getRenderTarget();
		target->bindRenderTarget();
	}
}

CameraPtr RendererGL::getCamera()
{
	return camera;
}


// State changes
void RendererGL::bindTexture(TexturePtr &tex, unsigned int textureUnit /* = 0 */)
{
	TextureGL *textureGL = (TextureGL*)tex.get();
	if (boundTextureID.at(textureUnit) != textureGL->getTexture()) {
		boundTextureID.at(textureUnit) = textureGL->getTexture();

		if (currentTextureUnit != textureUnit) {
			glActiveTexture(GL_TEXTURE0 + textureUnit);
			currentTextureUnit = textureUnit;
		}

		if (tex->getNumSamples() == 0) {
			glBindTexture(GL_TEXTURE_2D, textureGL->getTexture());
		} else {
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureGL->getTexture());
		}
	}
}

void RendererGL::setBlendMode(BlendMode mode)
{
	if (mode == blendMode || (SystemGL::get()->isPremulAphaEnabled()
			&& ((mode == BLEND_ALPHA && blendMode == BLEND_ADDITIVE)
					|| (mode == BLEND_ADDITIVE && blendMode == BLEND_ALPHA))))
		return;

	if (SystemGL::get()->isPremulAphaEnabled()) {
		if (mode == BLEND_OVERWRITE) {
			// Disables blending of textures with the scene
			glDisable(GL_BLEND);
		} else if (mode == BLEND_ALPHA) {
			// This allows alpha blending of textures with the scene
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);
			// TODO: glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);?
		} else if (mode == BLEND_ADDITIVE) {
			// This allows additive blending of textures with the scene
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glBlendEquation(GL_FUNC_ADD);
		} else if (mode == BLEND_SUBTRACTIVE) {
			// This allows additive blending of textures with the scene
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
		} else if (mode == BLEND_MODULATIVE) {
			// This allows modulative blending of textures with the scene
			glEnable(GL_BLEND);
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
			glBlendEquation(GL_FUNC_ADD);
		}
	} else {
		if (mode == BLEND_OVERWRITE) {
			// Disables blending of textures with the scene
			glDisable(GL_BLEND);
		} else if (mode == BLEND_ALPHA) {
			// This allows alpha blending of textures with the scene
			glEnable(GL_BLEND);
			//glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
			//glBlendEquation(GL_FUNC_ADD);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
			glBlendEquation(GL_FUNC_ADD);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// PREMUL: TODO
			/*glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
			glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);*/

		} else if (mode == BLEND_ADDITIVE) {
			// This allows additive blending of textures with the scene
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glBlendEquation(GL_FUNC_ADD);
		} else if (mode == BLEND_SUBTRACTIVE) {
			// This allows additive blending of textures with the scene
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
		} else if (mode == BLEND_MODULATIVE) {
			// This allows modulative blending of textures with the scene
			glEnable(GL_BLEND);
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
			glBlendEquation(GL_FUNC_ADD);
		}
	}

	// BLEND_OVERWRITE, BLEND_ALPHA, BLEND_ADDITIVE, BLEND_MODULATIVE
	blendMode = mode;
}

void RendererGL::setModelMatrix(const glm::mat4 &matrix)
{
	modelMatrix = matrix;
	mvpMatrix = viewProjectionMatrix * modelMatrix;
}

void RendererGL::setViewMatrix(const glm::mat4 &matrix)
{
	viewMatrix = matrix;
	viewProjectionMatrix = projectionMatrix * viewMatrix;
	mvpMatrix = viewProjectionMatrix * modelMatrix;
}

void RendererGL::setProjectionMatrix(const glm::mat4 &matrix)
{
	projectionMatrix = matrix;
	viewProjectionMatrix = projectionMatrix * viewMatrix;
	mvpMatrix = viewProjectionMatrix * modelMatrix;
}

void RendererGL::setLineWidth(float width)
{
	if (width != lineWidth) {
		lineWidth = width;
		glLineWidth(width);
	}
}

void RendererGL::setPointSize(float size)
{
	if (size != pointSize) {
		pointSize = size;
		glPointSize(size);
	}
}


// Stencil buffer
void RendererGL::enableStencilTest()
{
	glEnable(GL_STENCIL_TEST);
}

void RendererGL::disableStencilTest()
{
	glDisable(GL_STENCIL_TEST);
}

void RendererGL::setStencilMask(unsigned int mask)
{
	glStencilMask(mask);
}

void RendererGL::clearStencilBuffer()
{
	glClear(GL_STENCIL_BUFFER_BIT);
}

void RendererGL::setStencilFunc(unsigned int func, int ref, unsigned int mask)
{
	glStencilFunc(func, ref, mask);
}

void RendererGL::setStencilOp(unsigned int sfail, unsigned int dpfail, unsigned int dppass)
{
	glStencilOp(sfail, dpfail, dppass);
}


// Rendering
void RendererGL::render(ShaderAttributesPtr &shaderAttributes)
{
	if (wireframeMode) {
		ShaderAttributesPtr attr = shaderAttributes->copy(solidShader);
		attr->bind();
		attr->setModelViewProjectionMatrices(modelMatrix, viewMatrix, projectionMatrix, mvpMatrix);

		if (attr->getNumIndices() > 0) {
			glDrawRangeElements((GLuint)attr->getVertexMode(), 0, attr->getNumVertices()-1,
					attr->getNumIndices(), attr->getIndexFormat(), NULL);
		} else {
			glDrawArrays((GLuint)attr->getVertexMode(), 0, attr->getNumVertices());
		}
	} else {
		// Normal rendering
		shaderAttributes->bind();
		shaderAttributes->setModelViewProjectionMatrices(modelMatrix, viewMatrix, projectionMatrix, mvpMatrix);

		if (shaderAttributes->getNumIndices() > 0) {
			glDrawRangeElements((GLuint)shaderAttributes->getVertexMode(), 0, shaderAttributes->getNumVertices()-1,
					shaderAttributes->getNumIndices(), shaderAttributes->getIndexFormat(), NULL);
		} else {
			glDrawArrays((GLuint)shaderAttributes->getVertexMode(), 0, shaderAttributes->getNumVertices());
		}
	}
}

void  RendererGL::setPolygonMode(unsigned int polygonMode) // For debugging purposes
{
	glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
}

void RendererGL::enableWireframeMode(const Color &_wireframeColor)
{
	wireframeMode = true;
	wireframeColor = _wireframeColor;
	solidShader->setUniform("color", wireframeColor);
	setPolygonMode(GL_LINE);
}

void RendererGL::disableWireframeMode()
{
	wireframeMode = false;
	setPolygonMode(GL_FILL);
}


// Utility functions
void RendererGL::blitTexture(TexturePtr &tex, const AABB2 &renderRect)
{
	blitTexture(tex, renderRect, blitShader);
}

std::vector<VertexTextured> createTexturedQuad(const AABB2 &renderRect)
{
	glm::vec2 min = renderRect.getMinimum();
	glm::vec2 max = renderRect.getMaximum();
	std::vector<VertexTextured> quad{
		VertexTextured(glm::vec3(max.x,max.y,0), glm::vec2(1, 1)),
		VertexTextured(glm::vec3(min.x,min.y,0), glm::vec2(0, 0)),
		VertexTextured(glm::vec3(max.x,min.y,0), glm::vec2(1, 0)),
		VertexTextured(glm::vec3(min.x,min.y,0), glm::vec2(0, 0)),
		VertexTextured(glm::vec3(max.x,max.y,0), glm::vec2(1, 1)),
		VertexTextured(glm::vec3(min.x,max.y,0), glm::vec2(0, 1))};
	return quad;
}

void RendererGL::blitTexture(TexturePtr &tex, const AABB2 &renderRect, ShaderProgramPtr &shader)
{
	// Set-up the vertex data of the rectangle
	std::vector<VertexTextured> fullscreenQuad(createTexturedQuad(renderRect));

	// Feed the shader with the data and render the quad
	int stride = sizeof(VertexTextured);
	GeometryBufferPtr geomBuffer(new GeometryBufferGL(sizeof(VertexTextured)*fullscreenQuad.size(), &fullscreenQuad.front()));
	ShaderAttributesPtr shaderAttributes = ShaderManager->createShaderAttributes(shader);
	shaderAttributes->addGeometryBuffer(geomBuffer, "position", ATTRIB_FLOAT, 3, 0, stride);
	shaderAttributes->addGeometryBuffer(geomBuffer, "texcoord", ATTRIB_FLOAT, 2, sizeof(glm::vec3), stride);
	shaderAttributes->getShaderProgram()->setUniform("texture", tex);
	render(shaderAttributes);
}

//#define RESOLVE_BLIT_FBO
TexturePtr RendererGL::resolveMultisampledTexture(TexturePtr &tex) // Just returns tex if not multisampled
{
	if (tex->getNumSamples() <= 0) {
		return tex;
	}

#ifdef RESOLVE_BLIT_FBO
	TexturePtr resolvedTexture = TextureManager->createEmptyTexture(tex->getW(), tex->getH(),
			tex->getMinificationFilter(), tex->getMagnificationFilter(), tex->getWrapS(), tex->getWrapT());
	FramebufferObjectPtr msaaFBO = createFBO();
	FramebufferObjectPtr resolvedFBO = createFBO();
	msaaFBO->bind2DTexture(tex);
	resolvedFBO->bind2DTexture(resolvedTexture);
	int w = tex->getW(), h = tex->getH();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO->getID());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolvedFBO->getID());
	glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#else
	// OR:
	TexturePtr resolvedTexture = TextureManager->createEmptyTexture(tex->getW(), tex->getH(),
			tex->getMinificationFilter(), tex->getMagnificationFilter(), tex->getWrapS(), tex->getWrapT());
	FramebufferObjectPtr fbo = createFBO();
	fbo->bind2DTexture(resolvedTexture);

	// Set a new temporal MV and P matrix to render a fullscreen quad
	glm::mat4 oldProjMatrix = projectionMatrix;
	glm::mat4 oldViewMatrix = viewMatrix;
	glm::mat4 oldModelMatrix = modelMatrix;
	glm::mat4 projectionMatrix(matrixOrthogonalProjection(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f));
	setProjectionMatrix(projectionMatrix);
	setViewMatrix(matrixIdentity());
	setModelMatrix(matrixIdentity());

	// Set-up the vertex data of the rectangle
	std::vector<VertexTextured> fullscreenQuad(createTexturedQuad(AABB2(glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, 1.0f))));

	// Feed the shader with the rendering data
	int stride = sizeof(VertexTextured);
	GeometryBufferPtr geomBuffer(new GeometryBufferGL(sizeof(VertexTextured)*fullscreenQuad.size(), &fullscreenQuad.front()));
	ShaderAttributesPtr shaderAttributes = ShaderManager->createShaderAttributes(resolveMSAAShader);
	shaderAttributes->addGeometryBuffer(geomBuffer, "position", ATTRIB_FLOAT, 3, 0, stride);
	shaderAttributes->addGeometryBuffer(geomBuffer, "texcoord", ATTRIB_FLOAT, 2, sizeof(glm::vec3), stride);
	shaderAttributes->getShaderProgram()->setUniform("texture", tex);
	shaderAttributes->getShaderProgram()->setUniform("numSamples", tex->getNumSamples());

	// Now resolve the texture
	bindFBO(fbo);
	render(shaderAttributes);

	// Reset the matrices
	setProjectionMatrix(oldProjMatrix);
	setViewMatrix(oldViewMatrix);
	setModelMatrix(oldModelMatrix);
#endif

	return resolvedTexture;
}

void RendererGL::blurTexture(TexturePtr &tex)
{
	// Create a framebuffer and a temporal texture for blurring
	FramebufferObjectPtr blurFramebuffer = createFBO();
	TexturePtr tempBlurTexture = TextureManager->createEmptyTexture(tex->getW(), tex->getH(),
			GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);

	// Set a new temporal MV and P matrix to render a fullscreen quad
	glm::mat4 oldProjMatrix = projectionMatrix;
	glm::mat4 oldViewMatrix = viewMatrix;
	glm::mat4 oldModelMatrix = modelMatrix;
	glm::mat4 projectionMatrix(matrixOrthogonalProjection(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f));
	setProjectionMatrix(projectionMatrix);
	setViewMatrix(matrixIdentity());
	setModelMatrix(matrixIdentity());

	// Set-up the vertex data of the rectangle
	std::vector<VertexTextured> fullscreenQuad(createTexturedQuad(AABB2(glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, 1.0f))));

	// Feed the shader with the data and render the quad
	int stride = sizeof(VertexTextured);
	GeometryBufferPtr geomBuffer(new GeometryBufferGL(sizeof(VertexTextured)*fullscreenQuad.size(), &fullscreenQuad.front()));
	ShaderAttributesPtr shaderAttributes = ShaderManager->createShaderAttributes(blurShader);
	shaderAttributes->addGeometryBuffer(geomBuffer, "position", ATTRIB_FLOAT, 3, 0, stride);
	shaderAttributes->addGeometryBuffer(geomBuffer, "texcoord", ATTRIB_FLOAT, 2, sizeof(glm::vec3), stride);
	shaderAttributes->getShaderProgram()->setUniform("texture", tex);

	// Perform a horizontal and a vertical blur
	bindFBO(blurFramebuffer);
	shaderAttributes->getShaderProgram()->setUniform("horz_blur", 1);
	blurFramebuffer->bind2DTexture(tempBlurTexture);
	render(shaderAttributes);

	shaderAttributes->getShaderProgram()->setUniform("horz_blur", 0);
	blurFramebuffer->bind2DTexture(tex);
	render(shaderAttributes);

	// Reset the matrices
	setProjectionMatrix(oldProjMatrix);
	setViewMatrix(oldViewMatrix);
	setModelMatrix(oldModelMatrix);
}

TexturePtr RendererGL::getScaledTexture(TexturePtr &tex, Point2 newSize)
{
	// Create a framebuffer and the storage for the scaled texture
	FramebufferObjectPtr blurFramebuffer = createFBO();
	TexturePtr scaledTexture = TextureManager->createEmptyTexture(newSize.x, newSize.y,
			tex->getMinificationFilter(), tex->getMagnificationFilter(), tex->getWrapS(), tex->getWrapT());

	// Set a new temporal MV and P matrix to render a fullscreen quad
	glm::mat4 oldProjMatrix = projectionMatrix;
	glm::mat4 oldViewMatrix = viewMatrix;
	glm::mat4 oldModelMatrix = modelMatrix;
	glm::mat4 projectionMatrix(matrixOrthogonalProjection(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f));
	setProjectionMatrix(projectionMatrix);
	setViewMatrix(matrixIdentity());
	setModelMatrix(matrixIdentity());

	// Create a scaled copy of the texture
	bindFBO(blurFramebuffer);
	blurFramebuffer->bind2DTexture(scaledTexture);
	blitTexture(tex, AABB2(glm::vec2(-1,-1), glm::vec2(1,1)));

	// Reset the matrices
	setProjectionMatrix(oldProjMatrix);
	setViewMatrix(oldViewMatrix);
	setModelMatrix(oldModelMatrix);

	return scaledTexture;
}

void RendererGL::blitTextureFXAAAntialiased(TexturePtr &tex)
{
	// Set a new temporal MV and P matrix to render a fullscreen quad
	glm::mat4 oldProjMatrix = projectionMatrix;
	glm::mat4 oldViewMatrix = viewMatrix;
	glm::mat4 oldModelMatrix = modelMatrix;
	glm::mat4 projectionMatrix(matrixOrthogonalProjection(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f));
	setProjectionMatrix(projectionMatrix);
	setViewMatrix(matrixIdentity());
	setModelMatrix(matrixIdentity());

	// Set the attributes of the shader
	//fxaaShader->setUniform("m_Texture", tex);
	fxaaShader->setUniform("g_Resolution", glm::vec2(tex->getW(), tex->getH()));
	fxaaShader->setUniform("m_SubPixelShift", 1.0f / 4.0f);
	fxaaShader->setUniform("m_ReduceMul", 0.0f * 1.0f / 8.0f);
	//fxaaShader->setUniform("m_VxOffset", 0.0f);
	fxaaShader->setUniform("m_SpanMax", 16.0f);

	// Create a scaled copy of the texture
	blitTexture(tex, AABB2(glm::vec2(-1,-1), glm::vec2(1,1)), fxaaShader);

	// Reset the matrices
	setProjectionMatrix(oldProjMatrix);
	setViewMatrix(oldViewMatrix);
	setModelMatrix(oldModelMatrix);
}


// OpenGL-specific calls
void RendererGL::bindVAO(GLuint vao)
{
	if (vao != boundVAO) {
		boundVAO = vao;
		glBindVertexArray(vao);
	}
}

GLuint RendererGL::getVAO()
{
	return boundVAO;
}

void RendererGL::useShaderProgram(ShaderProgramGL *shader)
{
	unsigned int shaderID = shader ? shader->getShaderProgramID() : 0;
	if (shaderID != boundShader) {
		boundShader = shaderID;
		glUseProgram(shaderID);
	}
}

}
