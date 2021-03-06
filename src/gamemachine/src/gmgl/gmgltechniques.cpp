﻿#include "stdafx.h"
#include <GL/glew.h>
#include "gmgltechniques.h"
#include "gmgl/gmglgraphic_engine.h"
#include "gmgl/shader_constants.h"
#include "gmgl/gmgltexture.h"
#include "gmengine/gmgameworld.h"
#include <linearmath.h>
#include "foundation/gamemachine.h"
#include "foundation/utilities/utilities.h"
#include "gmglgbuffer.h"
#include "gmglframebuffer.h"

namespace
{
	bool g_shadowDirty = true;

	inline GLenum toStencilOp(GMStencilOptions::GMStencilOp stencilOptions)
	{
		switch (stencilOptions)
		{
		case GMStencilOptions::Keep:
			return GL_KEEP;
		case GMStencilOptions::Zero:
			return GL_ZERO;
		case GMStencilOptions::Replace:
			return GL_REPLACE;
		default:
			GM_ASSERT(false);
			return GL_KEEP;
		}
	}

	inline void applyStencil(GMGLGraphicEngine& engine)
	{
		// 应用模板
		GLenum compareFunc = GL_ALWAYS;
		auto& stencilOptions = engine.getStencilOptions();
		switch (stencilOptions.compareFunc)
		{
		case GMStencilOptions::Equal:
			compareFunc = GL_EQUAL;
			break;
		case GMStencilOptions::NotEqual:
			compareFunc = GL_NOTEQUAL;
			break;
		case GMStencilOptions::Never:
			compareFunc = GL_NEVER;
			break;
		case GMStencilOptions::Always:
			compareFunc = GL_ALWAYS;
			break;
		case GMStencilOptions::Less:
			compareFunc = GL_LESS;
			break;
		case GMStencilOptions::LessEqual:
			compareFunc = GL_LEQUAL;
			break;
		case GMStencilOptions::Greater:
			compareFunc = GL_GREATER;
			break;
		case GMStencilOptions::GreaterEqual:
			compareFunc = GL_GEQUAL;
			break;
		default:
			GM_ASSERT(false);
		}

		glStencilFunc(compareFunc, 1, 0xFF);
		glStencilMask(stencilOptions.writeMask);
		glStencilOp(toStencilOp(stencilOptions.stencilFailedOp), toStencilOp(stencilOptions.stencilDepthFailedOp), toStencilOp(stencilOptions.stencilPassOp));
	}

	inline GLenum getMode(GMTopologyMode mode)
	{
		switch (mode)
		{
		case GMTopologyMode::TriangleStrip:
			return GL_TRIANGLE_STRIP;
		case GMTopologyMode::Triangles:
			return GL_TRIANGLES;
		case GMTopologyMode::Lines:
			return GL_LINES;
		default:
			GM_ASSERT(false);
			return GL_TRIANGLES;
		}
	}

	inline const char* getTechnique(GMModelType type)
	{
		switch (type)
		{
		case GMModelType::Model2D:
			return "GM_Model2D";
		case GMModelType::Model3D:
			return "GM_Model3D";
		case GMModelType::Text:
			return "GM_Text";
		case GMModelType::CubeMap:
			return "GM_CubeMap";
		case GMModelType::Particle:
			return "GM_Particle";
		case GMModelType::Custom:
			return "GM_Custom";
		default:
			GM_ASSERT(false);
			return "GM_Model2D";
		}
	}

	inline void applyShadow(const GMShadowSourceDesc* shadowSourceDesc, IShaderProgram* shaderProgram, GMGLShadowFramebuffers* shadowFramebuffers, bool hasShadow)
	{
		static IShaderProgram* lastProgram = nullptr;
		static GMint64 lastShadowVersion = 0;
		static const GMString s_shadowInfo = GMString(GM_VariablesDesc.ShadowInfo.ShadowInfo) + ".";
		static const std::string s_position = (s_shadowInfo + GM_VariablesDesc.ShadowInfo.Position).toStdString();
		static const std::string s_matrix = (s_shadowInfo + GM_VariablesDesc.ShadowInfo.ShadowMatrix).toStdString();
		static const std::string s_width = (s_shadowInfo + GM_VariablesDesc.ShadowInfo.ShadowMapWidth).toStdString();
		static const std::string s_height = (s_shadowInfo + GM_VariablesDesc.ShadowInfo.ShadowMapWidth).toStdString();
		static const std::string s_biasMin = (s_shadowInfo + GM_VariablesDesc.ShadowInfo.BiasMin).toStdString();
		static const std::string s_biasMax = (s_shadowInfo + GM_VariablesDesc.ShadowInfo.BiasMax).toStdString();
		static const std::string s_hasShadow = (s_shadowInfo + GM_VariablesDesc.ShadowInfo.HasShadow).toStdString();
		static const std::string s_shadowMap = (s_shadowInfo + GM_VariablesDesc.ShadowInfo.ShadowMap).toStdString();

		if (hasShadow)
		{
			if (g_shadowDirty || lastShadowVersion != shadowSourceDesc->version)
			{
				lastProgram = shaderProgram;
				lastShadowVersion = shadowSourceDesc->version;

				shaderProgram->setBool(s_hasShadow.c_str(), 1);
				const GMCamera& camera = shadowSourceDesc->camera;
				GMFloat4 viewPosition;
				shadowSourceDesc->position.loadFloat4(viewPosition);
				shaderProgram->setVec4(s_position.c_str(), viewPosition);
				shaderProgram->setMatrix4(s_matrix.c_str(), camera.getViewMatrix() * camera.getProjectionMatrix());
				shaderProgram->setFloat(s_biasMin.c_str(), shadowSourceDesc->biasMin);
				shaderProgram->setFloat(s_biasMax.c_str(), shadowSourceDesc->biasMax);

				shaderProgram->setInt(s_width.c_str(), shadowFramebuffers->getShadowMapWidth());
				shaderProgram->setInt(s_height.c_str(), shadowFramebuffers->getShadowMapHeight());
				shaderProgram->setInt(s_shadowMap.c_str(), GMTextureRegisterQuery<GMTextureType::ShadowMap>::Value);
				g_shadowDirty = false;
			}
		}
		else
		{
			shaderProgram->setBool(s_hasShadow.c_str(), 0);
		}
	}

	GMTextureAsset createWhiteTexture(const IRenderContext* context)
	{
		GMTextureAsset texture;
		GM.getFactory()->createWhiteTexture(context, texture);
		return texture;
	}
}

void GMGammaHelper::setGamma(GMGraphicEngine* engine, IShaderProgram* shaderProgram)
{
	shaderProgram->setBool(GM_VariablesDesc.GammaCorrection.GammaCorrection, engine->needGammaCorrection());
	GMfloat gamma = engine->getGammaValue();
	if (gamma != m_gamma)
	{
		shaderProgram->setFloat(GM_VariablesDesc.GammaCorrection.GammaValue, gamma);
		shaderProgram->setFloat(GM_VariablesDesc.GammaCorrection.GammaInvValue, 1.f / gamma);
		m_gamma = gamma;
	}
}

GMGLTechnique::GMGLTechnique(const IRenderContext* context)
{
	D(d);
	d->context = context;
	d->engine = gm_cast<GMGLGraphicEngine*>(d->context->getEngine());
	d->debugConfig = GM.getConfigs().getConfig(GMConfigs::Debug).asDebugConfig();
}

void GMGLTechnique::draw(GMModel* model)
{
	D(d);
	GMGLBeginGetErrorsAndCheck();

	glBindVertexArray(model->getModelBuffer()->getMeshBuffer().arrayId);
	applyStencil(*d->engine);
	prepareScreenInfo(getShaderProgram());
	beforeDraw(model);
	GLenum mode = (d->engine->isWireFrameMode(model)) ? GL_LINE_LOOP : getMode(model->getPrimitiveTopologyMode());
	if (model->getDrawMode() == GMModelDrawMode::Vertex)
		glDrawArrays(mode, 0, gm_sizet_to<GLsizei>(model->getVerticesCount()));
	else
		glDrawElements(mode, gm_sizet_to<GLsizei>(model->getVerticesCount()), GL_UNSIGNED_INT, 0);
	afterDraw(model);
	glBindVertexArray(0);

	GMGLEndGetErrorsAndCheck();
}

void GMGLTechnique::beginModel(GMScene* scene, GMModel* model, const GMGameObject* parent)
{
	D(d);
	d->currentModel = model;
	auto shaderProgram = getShaderProgram();
	shaderProgram->useProgram();

	// We must init this uniform. Otherwise some X11 (Ubuntu) Windows will be abnormal.
	shaderProgram->setInt(GM_VariablesDesc.CubeMapTextureName, GMTextureRegisterQuery<GMTextureType::CubeMap>::Value);

	// 设置顶点颜色运算方式
	getShaderProgram()->setInt(GM_VariablesDesc.ColorVertexOp, static_cast<GMint32>(model->getShader().getVertexColorOp()));
}

void GMGLTechnique::endModel()
{
	D(d);
	d->currentModel = nullptr;
}

void GMGLTechnique::activateTextureTransform(GMModel* model, GMTextureType type)
{
	static const std::string u_scrolls_affix = (GMString(".") + GM_VariablesDesc.TextureAttributes.OffsetX).toStdString();
	static const std::string u_scrollt_affix = (GMString(".") + GM_VariablesDesc.TextureAttributes.OffsetY).toStdString();
	static const std::string u_scales_affix = (GMString(".") + GM_VariablesDesc.TextureAttributes.ScaleX).toStdString();
	static const std::string u_scalet_affix = (GMString(".") + GM_VariablesDesc.TextureAttributes.ScaleY).toStdString();

	auto shaderProgram = getShaderProgram();
	const char* uniform = getTextureUniformName(type);

	static char u_scrolls[GMGL_MAX_UNIFORM_NAME_LEN],
		u_scrollt[GMGL_MAX_UNIFORM_NAME_LEN],
		u_scales[GMGL_MAX_UNIFORM_NAME_LEN],
		u_scalet[GMGL_MAX_UNIFORM_NAME_LEN];

	combineUniform(u_scrolls, uniform, u_scrolls_affix.c_str());
	combineUniform(u_scrollt, uniform, u_scrollt_affix.c_str());
	combineUniform(u_scales, uniform, u_scales_affix.c_str());
	combineUniform(u_scalet, uniform, u_scalet_affix.c_str());
	shaderProgram->setFloat(u_scrolls, 0.f);
	shaderProgram->setFloat(u_scrollt, 0.f);
	shaderProgram->setFloat(u_scales, 1.f);
	shaderProgram->setFloat(u_scalet, 1.f);

	auto applyCallback = [&](GMS_TextureTransformType type, Pair<GMfloat, GMfloat>&& args) {
		if (type == GMS_TextureTransformType::Scale)
		{
			shaderProgram->setFloat(u_scales, args.first);
			shaderProgram->setFloat(u_scalet, args.second);
		}
		else if (type == GMS_TextureTransformType::Scroll)
		{
			shaderProgram->setFloat(u_scrolls, args.first);
			shaderProgram->setFloat(u_scrollt, args.second);
		}
		else
		{
			GM_ASSERT(false);
		}
	};

	if (model)
		model->getShader().getTextureList().getTextureSampler(type).applyTexMode(GM.getRunningStates().elapsedTime, applyCallback);
}

GMint32 GMGLTechnique::activateTexture(GMModel* model, GMTextureType type)
{
	static const std::string u_tex_enabled = std::string(".") + GM_VariablesDesc.TextureAttributes.Enabled;
	static const std::string u_tex_texture = std::string(".") + GM_VariablesDesc.TextureAttributes.Texture;

	GMint32 idx = (GMint32)type;
	GMint32 texId = getTextureID(type);

	auto shaderProgram = getShaderProgram();
	const char* uniform = getTextureUniformName(type);
	static char u_texture[GMGL_MAX_UNIFORM_NAME_LEN], u_enabled[GMGL_MAX_UNIFORM_NAME_LEN];
	combineUniform(u_texture, uniform, u_tex_texture.c_str());
	combineUniform(u_enabled, uniform, u_tex_enabled.c_str());
	shaderProgram->setInt(u_texture, texId);
	shaderProgram->setInt(u_enabled, 1);

	activateTextureTransform(model, type);
	return texId;
}

void GMGLTechnique::deactivateTexture(GMTextureType type)
{
	static const std::string u_tex_enabled = (GMString(".") + GM_VariablesDesc.TextureAttributes.Enabled).toStdString();
	GMint32 texId = getTextureID(type);

	auto shaderProgram = getShaderProgram();
	GMint32 idx = (GMint32)type;
	const char* uniform = getTextureUniformName(type);
	static char u[GMGL_MAX_UNIFORM_NAME_LEN];
	combineUniform(u, uniform, u_tex_enabled.c_str());
	shaderProgram->setInt(u, 0);
}

GMint32 GMGLTechnique::getTextureID(GMTextureType type)
{
	switch (type)
	{
	case GMTextureType::Ambient:
		return GMTextureRegisterQuery<GMTextureType::Ambient>::Value;
	case GMTextureType::Diffuse:
		return GMTextureRegisterQuery<GMTextureType::Diffuse>::Value;
	case GMTextureType::Specular:
		return GMTextureRegisterQuery<GMTextureType::Specular>::Value;
	case GMTextureType::NormalMap:
		return GMTextureRegisterQuery<GMTextureType::NormalMap>::Value;
	case GMTextureType::Albedo:
		return GMTextureRegisterQuery<GMTextureType::Albedo>::Value;
	case GMTextureType::MetallicRoughnessAO:
		return GMTextureRegisterQuery<GMTextureType::MetallicRoughnessAO>::Value;
	case GMTextureType::Lightmap:
		return GMTextureRegisterQuery<GMTextureType::Lightmap>::Value;
	default:
		GM_ASSERT(false);
		return -1;
	}
}

bool GMGLTechnique::drawTexture(GMModel* model, GMTextureType type)
{
	D(d);
	if (d->engine->isNeedDiscardTexture(model, type))
		return false;

	// 按照贴图类型选择纹理动画序列
	GMTextureSampler& sampler = model->getShader().getTextureList().getTextureSampler(type);

	// 获取序列中的这一帧
	GMTextureAsset texture = getTexture(sampler);
	if (!texture.isEmpty())
	{
		// 激活动画序列
		GMint32 texId = activateTexture(model, (GMTextureType)type);
		texture.getTexture()->bindSampler(&sampler);
		texture.getTexture()->useTexture(texId);
		return true;
	}
	return false;
}

GMTextureAsset GMGLTechnique::getTexture(GMTextureSampler& frames)
{
	if (frames.getFrameCount() == 0)
		return GMAsset::invalidAsset();

	if (frames.getFrameCount() == 1)
		return frames.getFrameByIndex(0);

	// 如果frameCount > 1，说明是个动画，要根据Shader的间隔来选择合适的帧
	// TODO
	GMint32 elapsed = GM.getRunningStates().elapsedTime * 1000;

	return frames.getFrameByIndex((elapsed / frames.getAnimationMs()) % frames.getFrameCount());
}

void GMGLTechnique::updateCameraMatrices(IShaderProgram* shaderProgram)
{
	D(d);
	GMCamera& camera = d->engine->getCamera();
	if (d->lastShaderProgram_camera != shaderProgram || camera.isDirty())
	{
		const GMMat4& viewMatrix = camera.getViewMatrix();
		const GMCameraLookAt& lookAt = camera.getLookAt();
		GMFloat4 vec;
		lookAt.position.loadFloat4(vec);

		GMGraphicEngine::getDefaultShaderVariablesDesc();
		shaderProgram->setVec4(GM_VariablesDesc.ViewPosition, vec);
		shaderProgram->setMatrix4(GM_VariablesDesc.ViewMatrix, camera.getViewMatrix());
		shaderProgram->setMatrix4(GM_VariablesDesc.InverseViewMatrix, camera.getInverseViewMatrix());
		shaderProgram->setMatrix4(GM_VariablesDesc.ProjectionMatrix, camera.getProjectionMatrix());
		d->lastShaderProgram_camera = shaderProgram;
		camera.cleanDirty();
	}
}

void GMGLTechnique::prepareScreenInfo(IShaderProgram* shaderProgram)
{
	D(d);
	static std::string s_multisampling = std::string(GM_VariablesDesc.ScreenInfoAttributes.ScreenInfo) + "." + GM_VariablesDesc.ScreenInfoAttributes.Multisampling;
	static std::string s_screenWidth = std::string(GM_VariablesDesc.ScreenInfoAttributes.ScreenInfo) + "." + GM_VariablesDesc.ScreenInfoAttributes.ScreenWidth;
	static std::string s_screenHeight = std::string(GM_VariablesDesc.ScreenInfoAttributes.ScreenInfo) + "." + GM_VariablesDesc.ScreenInfoAttributes.ScreenHeight;
	if (d->lastShaderProgram_screenInfo != shaderProgram) //或者窗口属性发生改变
	{
		const GMWindowStates& windowStates = d->context->getWindow()->getWindowStates();
		shaderProgram->setInt(s_multisampling.c_str(), windowStates.sampleCount > 1);
		shaderProgram->setInt(s_screenWidth.c_str(), windowStates.renderRect.width);
		shaderProgram->setInt(s_screenHeight.c_str(), windowStates.renderRect.height);
		d->lastShaderProgram_screenInfo = shaderProgram;
	}
}

void GMGLTechnique::dirtyShadowMapAttributes()
{
	g_shadowDirty = true;
}

void GMGLTechnique::applyShader(GMModel* model)
{
	prepareBlend(model);
	prepareFrontFace(model);
	prepareCull(model);
	prepareDepth(model);
	prepareDebug(model);
}

void GMGLTechnique::prepareCull(GMModel* model)
{
	const GMShader& shader = model->getShader();
	if (shader.getCull() == GMS_Cull::Cull)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
}

void GMGLTechnique::prepareFrontFace(GMModel* model)
{
	const GMShader& shader = model->getShader();
	if (shader.getFrontFace() == GMS_FrontFace::Closewise)
		glFrontFace(GL_CW);
	else
		glFrontFace(GL_CCW);
}

void GMGLTechnique::prepareBlend(GMModel* model)
{
	D(d);
	bool blend = false;
	const GMShader& shader = model->getShader();
	const GMGlobalBlendStateDesc& globalBlendState = d->engine->getGlobalBlendState();
	if (globalBlendState.enabled)
	{
		blend = true;
		if (shader.getBlend())
		{
			blend = true;
			glEnable(GL_BLEND);
			GMGLUtility::blendFunc(
				shader.getBlendFactorSourceRGB(),
				shader.getBlendFactorDestRGB(),
				shader.getBlendOpRGB(),
				shader.getBlendFactorSourceAlpha(),
				shader.getBlendFactorDestAlpha(),
				shader.getBlendOpAlpha()
			);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}
	else
	{
		if (shader.getBlend())
		{
			blend = true;
			glEnable(GL_BLEND);
			GMGLUtility::blendFunc(
				shader.getBlendFactorSourceRGB(),
				shader.getBlendFactorDestRGB(),
				shader.getBlendOpRGB(),
				shader.getBlendFactorSourceAlpha(),
				shader.getBlendFactorDestAlpha(),
				shader.getBlendOpAlpha()
			);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}
}

void GMGLTechnique::prepareDepth(GMModel* model)
{
	const GMShader& shader = model->getShader();
	if (shader.getNoDepthTest())
		glDisable(GL_DEPTH_TEST); // glDepthMask(GL_FALSE);
	else
		glEnable(GL_DEPTH_TEST); // glDepthMask(GL_TRUE);
}

void GMGLTechnique::prepareDebug(GMModel* model)
{
	D(d);
	GMint32 mode = d->debugConfig.get(gm::GMDebugConfigs::DrawPolygonNormalMode).toInt();
	getShaderProgram()->setInt(GM_VariablesDesc.Debug.Normal, mode);
}

//////////////////////////////////////////////////////////////////////////
void GMGLTechnique_3D::beginModel(GMScene* scene, GMModel* model, const GMGameObject* parent)
{
	Base::beginModel(scene, model, parent);

	D(d);
	D_BASE(db, GMGLTechnique);
	auto shaderProgram = getShaderProgram();
	updateCameraMatrices(shaderProgram);

	if (parent)
	{
		shaderProgram->setMatrix4(GM_VariablesDesc.ModelMatrix, parent->getTransform());
		shaderProgram->setMatrix4(GM_VariablesDesc.InverseTransposeModelMatrix, InverseTranspose(parent->getTransform()));
	}
	else
	{
		shaderProgram->setMatrix4(GM_VariablesDesc.ModelMatrix, Identity<GMMat4>());
		shaderProgram->setMatrix4(GM_VariablesDesc.InverseTransposeModelMatrix, Identity<GMMat4>());
	}

	GMGeometryPassingState state = db->engine->getGBuffer()->getGeometryPassingState();
	if (state != GMGeometryPassingState::PassingGeometry)
	{
		shaderProgram->setInterfaceInstance(
			GMGLShaderProgram::techniqueName(),
			getTechnique(model->getType()),
			GMShaderType::Vertex);
		shaderProgram->setInterfaceInstance(
			GMGLShaderProgram::techniqueName(),
			getTechnique(model->getType()),
			GMShaderType::Pixel);
		db->engine->activateLights(this);
	}

	const GMShadowSourceDesc& shadowSourceDesc = db->engine->getShadowSourceDesc();
	if (shadowSourceDesc.type != GMShadowSourceDesc::NoShadow)
	{
		GMGLShadowFramebuffers* shadowFramebuffers = gm_cast<GMGLShadowFramebuffers*>(db->engine->getShadowMapFramebuffers());
		shadowFramebuffers->getShadowMapTexture().getTexture()->useTexture(0);
		applyShadow(&shadowSourceDesc, shaderProgram, shadowFramebuffers, true);
	}
	else
	{
		applyShadow(nullptr, shaderProgram, nullptr, false);
	}

	db->gammaHelper.setGamma(db->engine, shaderProgram);
}

void GMGLTechnique_3D::beforeDraw(GMModel* model)
{
	D(d);
	D_BASE(db, GMGLTechnique);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// 材质
	activateMaterial(model->getShader());

	// 应用Shader
	applyShader(model);
	
	// 设置光照模型
	IShaderProgram* shaderProgram = getShaderProgram();
	GMShader& shader = model->getShader();
	GMIlluminationModel illuminationModel = shader.getIlluminationModel();
	shaderProgram->setInt(GM_VariablesDesc.IlluminationModel, (GMint32)illuminationModel);

	// 纹理
	GM_FOREACH_ENUM_CLASS(type, GMTextureType::Ambient, GMTextureType::EndOfCommonTexture)
	{
		if (!drawTexture(model, (GMTextureType)type))
		{
			if (illuminationModel == GMIlluminationModel::Phong && (
				type == GMTextureType::Ambient ||
				type == GMTextureType::Diffuse ||
				type == GMTextureType::Specular ||
				type == GMTextureType::Lightmap
				))
			{
				GMint32 texId = activateTexture(nullptr, (GMTextureType)type);
				getWhiteTexture().getTexture()->useTexture(texId);
			}
		}
	}

	// 调试绘制
	drawDebug();
}

void GMGLTechnique_3D::afterDraw(GMModel* model)
{
	D(d);
	GM_FOREACH_ENUM_CLASS(type, GMTextureType::Ambient, GMTextureType::EndOfCommonTexture)
	{
		deactivateTexture((GMTextureType)type);
	}
}

IShaderProgram* GMGLTechnique_3D::getShaderProgram()
{
	D_BASE(d, Base);
	GMGeometryPassingState s = d->engine->getGBuffer()->getGeometryPassingState();
	if (s == GMGeometryPassingState::PassingGeometry)
		return d->engine->getShaderProgram(GMShaderProgramType::DeferredGeometryPassShaderProgram);

	return d->engine->getShaderProgram(GMShaderProgramType::ForwardShaderProgram);
}

void GMGLTechnique_3D::activateMaterial(const GMShader& shader)
{
	D(d);
	auto shaderProgram = getShaderProgram();
	static const std::string GMSHADER_MATERIAL_KA = std::string(GM_VariablesDesc.MaterialName) + "." + GM_VariablesDesc.MaterialAttributes.Ka;
	static const std::string GMSHADER_MATERIAL_KD = std::string(GM_VariablesDesc.MaterialName) + "." + GM_VariablesDesc.MaterialAttributes.Kd;
	static const std::string GMSHADER_MATERIAL_KS = std::string(GM_VariablesDesc.MaterialName) + "." + GM_VariablesDesc.MaterialAttributes.Ks;
	static const std::string GMSHADER_MATERIAL_SHININESS = std::string(GM_VariablesDesc.MaterialName) + "." + GM_VariablesDesc.MaterialAttributes.Shininess;
	static const std::string GMSHADER_MATERIAL_REFRACTIVITY = std::string(GM_VariablesDesc.MaterialName) + "." + GM_VariablesDesc.MaterialAttributes.Refreactivity;
	static const std::string GMSHADER_MATERIAL_F0 = std::string(GM_VariablesDesc.MaterialName) + "." + GM_VariablesDesc.MaterialAttributes.F0;

	const GMMaterial& material = shader.getMaterial();
	shaderProgram->setVec3(GMSHADER_MATERIAL_KA.c_str(), ValuePointer(material.ka));
	shaderProgram->setVec3(GMSHADER_MATERIAL_KD.c_str(), ValuePointer(material.kd));
	shaderProgram->setVec3(GMSHADER_MATERIAL_KS.c_str(), ValuePointer(material.ks));
	shaderProgram->setFloat(GMSHADER_MATERIAL_SHININESS.c_str(), material.shininess);
	shaderProgram->setFloat(GMSHADER_MATERIAL_REFRACTIVITY.c_str(), material.refractivity);
	shaderProgram->setVec3(GMSHADER_MATERIAL_F0.c_str(), ValuePointer(material.f0));
}

void GMGLTechnique_3D::drawDebug()
{
	D(d);
	D_BASE(db, Base);
	auto shaderProgram = getShaderProgram();
	shaderProgram->setInt(GMSHADER_DEBUG_DRAW_NORMAL, db->debugConfig.get(GMDebugConfigs::DrawPolygonNormalMode).toInt());
}

GMTextureAsset GMGLTechnique_3D::getWhiteTexture()
{
	D(d);
	D_BASE(db, Base);
	if (d->whiteTexture.isEmpty())
		d->whiteTexture = createWhiteTexture(db->context);
	return d->whiteTexture;
}

//////////////////////////////////////////////////////////////////////////
void GMGLTechnique_2D::beforeDraw(GMModel* model)
{
	D(d);
	// 应用Shader
	applyShader(model);

	// 只选择环境光纹理
	GMTextureSampler& sampler = model->getShader().getTextureList().getTextureSampler(GMTextureType::Ambient);

	// 获取序列中的这一帧
	GMTextureAsset texture = getTexture(sampler);
	if (!texture.isEmpty())
	{
		// 激活动画序列
		GMint32 texId = activateTexture(model, GMTextureType::Ambient);
		texture.getTexture()->bindSampler(&sampler);
		texture.getTexture()->useTexture(texId);
	}
}

//////////////////////////////////////////////////////////////////////////
void GMGLTechnique_CubeMap::beginModel(GMScene* scene, GMModel* model, const GMGameObject* parent)
{
	Base::beginModel(scene, model, parent);
	IShaderProgram* shaderProgram = getShaderProgram();
	updateCameraMatrices(shaderProgram);
	shaderProgram->setMatrix4(GM_VariablesDesc.ModelMatrix, GMMat4(Inhomogeneous(parent->getTransform())));
}

void GMGLTechnique_CubeMap::beforeDraw(GMModel* model)
{
	D(d);
	D_BASE(db, Base);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	GMTextureList& texture = model->getShader().getTextureList();
	GMTextureSampler& sampler = texture.getTextureSampler(GMTextureType::CubeMap);
	GMTextureAsset glTex = sampler.getFrameByIndex(0);
	if (!glTex.isEmpty())
	{
		IShaderProgram* shaderProgram = getShaderProgram();
		GM_ASSERT(model->getType() == GMModelType::CubeMap);
		shaderProgram->setInterfaceInstance(GMGLShaderProgram::techniqueName(), getTechnique(model->getType()), GMShaderType::Vertex);
		shaderProgram->setInterfaceInstance(GMGLShaderProgram::techniqueName(), getTechnique(model->getType()), GMShaderType::Pixel);
		shaderProgram->setInt(GM_VariablesDesc.CubeMapTextureName, GMTextureRegisterQuery<GMTextureType::CubeMap>::Value);
		glTex.getTexture()->bindSampler(&sampler);
		glTex.getTexture()->useTexture(GMTextureRegisterQuery<GMTextureType::CubeMap>::Value);
		db->engine->setCubeMap(glTex);
	}
}

void GMGLTechnique_CubeMap::afterDraw(GMModel* model)
{
}

void GMGLTechnique_Filter::beforeDraw(GMModel* model)
{
	applyShader(model);
	GMTextureSampler& sampler = model->getShader().getTextureList().getTextureSampler(GMTextureType::Ambient);
	GMTextureAsset texture = getTexture(sampler);
	GM_ASSERT(!texture.isEmpty());
	GMint32 texId = activateTexture(model, GMTextureType::Ambient);
	texture.getTexture()->bindSampler(&sampler);
	texture.getTexture()->useTexture(texId);
}

void GMGLTechnique_Filter::afterDraw(GMModel* model)
{
}

void GMGLTechnique_Filter::beginModel(GMScene* scene, GMModel* model, const GMGameObject* parent)
{
	Base::beginModel(scene, model, parent);

	D(d);
	D_BASE(db, Base);
	IShaderProgram* shaderProgram = getShaderProgram();
	if (d->state.HDR != db->engine->needHDR() || d->state.toneMapping != db->engine->getToneMapping())
	{
		d->state.HDR = db->engine->needHDR();
		d->state.toneMapping = db->engine->getToneMapping();
		if (d->state.HDR)
		{
			db->gammaHelper.setGamma(db->engine, shaderProgram);
			setHDR(shaderProgram);
		}
		else
		{
			shaderProgram->setBool(GM_VariablesDesc.HDR.HDR, false);
		}
	}
}

void GMGLTechnique_Filter::setHDR(IShaderProgram* shaderProgram)
{
	D(d);
	shaderProgram->setBool(GM_VariablesDesc.HDR.HDR, true);
	shaderProgram->setInt(GM_VariablesDesc.HDR.ToneMapping, d->state.toneMapping);
}

IShaderProgram* GMGLTechnique_Filter::getShaderProgram()
{
	D_BASE(db, GMGLTechnique);
	return db->engine->getShaderProgram(GMShaderProgramType::FilterShaderProgram);
}

GMint32 GMGLTechnique_Filter::activateTexture(GMModel* model, GMTextureType type)
{
	D(d);
	D_BASE(db, Base);
	GMint32 texId = getTextureID(type);
	IShaderProgram* shaderProgram = getShaderProgram();
	shaderProgram->setInt(GMSHADER_FRAMEBUFFER, texId);

	bool b = shaderProgram->setInterfaceInstance(GM_VariablesDesc.FilterAttributes.Filter, GM_VariablesDesc.FilterAttributes.Types[db->engine->getCurrentFilterMode()], GMShaderType::Pixel);
	GM_ASSERT(b);
	return texId;
}

IShaderProgram* GMGLTechnique_LightPass::getShaderProgram()
{
	D_BASE(db, GMGLTechnique);
	return db->engine->getShaderProgram(GMShaderProgramType::DeferredLightPassShaderProgram);
}

void GMGLTechnique_LightPass::beginModel(GMScene* scene, GMModel* model, const GMGameObject* parent)
{
	GMGLTechnique::beginModel(scene, model, parent);

	D_BASE(d, GMGLTechnique);
	IShaderProgram* shaderProgram = getShaderProgram();
	updateCameraMatrices(shaderProgram);

	const GMShadowSourceDesc& shadowSourceDesc = d->engine->getShadowSourceDesc();
	if (shadowSourceDesc.type != GMShadowSourceDesc::NoShadow)
	{
		GMGLShadowFramebuffers* shadowFramebuffers = gm_cast<GMGLShadowFramebuffers*>(d->engine->getShadowMapFramebuffers());
		shadowFramebuffers->getShadowMapTexture().getTexture()->useTexture(0);
		applyShadow(&shadowSourceDesc, shaderProgram, shadowFramebuffers, true);
	}
	else
	{
		applyShadow(nullptr, shaderProgram, nullptr, false);
	}

	d->gammaHelper.setGamma(d->engine, shaderProgram);
}

void GMGLTechnique_LightPass::afterDraw(GMModel* model)
{
}

void GMGLTechnique_LightPass::beforeDraw(GMModel* model)
{
	D_BASE(d, GMGLTechnique);
	IShaderProgram* shaderProgram = getShaderProgram();
	d->engine->activateLights(this);
	IGBuffer* gBuffer = d->engine->getGBuffer();
	IFramebuffers* gBufferFramebuffers = gBuffer->getGeometryFramebuffers();
	GMsize_t cnt = gBufferFramebuffers->count();
	for (GMsize_t i = 0; i < cnt; ++i)
	{
		GMTextureAsset texture;
		gBufferFramebuffers->getFramebuffer(i)->getTexture(texture);
		const GMsize_t textureIndex = (GMTextureRegisterQuery<GMTextureType::GeometryPasses>::Value + i);
		shaderProgram->setInt(GMGLGBuffer::GBufferGeometryUniformNames()[i].c_str(), gm_sizet_to_uint(textureIndex));
		texture.getTexture()->useTexture((GMuint32)textureIndex);
	}

	GMTextureAsset cubeMap = d->engine->getCubeMap();
	if (!cubeMap.isEmpty())
	{
		const GMsize_t id = GMTextureRegisterQuery<GMTextureType::GeometryPasses>::Value + 1 + cnt;
		shaderProgram->setInt(GM_VariablesDesc.CubeMapTextureName, gm_sizet_to_uint(id));
		cubeMap.getTexture()->useTexture((GMuint32)id);
	}
}

void GMGLTechnique_3D_Shadow::beginModel(GMScene* scene, GMModel* model, const GMGameObject* parent)
{
	GMGLTechnique_3D::beginModel(scene, model, parent);

	D_BASE(d, Base);
	IShaderProgram* shaderProgram = getShaderProgram();
	if (parent)
		shaderProgram->setMatrix4(GM_VariablesDesc.ModelMatrix, parent->getTransform());
	else
		shaderProgram->setMatrix4(GM_VariablesDesc.ModelMatrix, Identity<GMMat4>());

	GMGLShadowFramebuffers* shadowFramebuffers = gm_cast<GMGLShadowFramebuffers*>(d->engine->getShadowMapFramebuffers());
	applyShadow(&d->engine->getShadowSourceDesc(), shaderProgram, shadowFramebuffers, true);

	bool b = shaderProgram->setInterfaceInstance(
		GMGLShaderProgram::techniqueName(),
		"GM_Shadow",
		GMShaderType::Vertex);
	GM_ASSERT(b);
	b = shaderProgram->setInterfaceInstance(
		GMGLShaderProgram::techniqueName(),
		"GM_Shadow",
		GMShaderType::Pixel);
	GM_ASSERT(b);
}

IShaderProgram* GMGLTechnique_Custom::getShaderProgram()
{
	D_BASE(d, GMGLTechnique);
	GMRenderTechniqueManager* rtm = d->engine->getRenderTechniqueManager();
	IShaderProgram* shaderProgram = rtm->getShaderProgram(d->currentModel->getTechniqueId());
	GM_ASSERT(shaderProgram);
	return shaderProgram;
}