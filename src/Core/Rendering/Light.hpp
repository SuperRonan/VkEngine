#pragma once

#include <Core/App/VkApplication.hpp>
#include <Core/Maths/Transforms.hpp>
#include <Core/IO/GuiContext.hpp>

namespace vkl
{
	using vec2 = glm::vec2;
	using vec3 = glm::vec3;
	using vec4 = glm::vec4;
	using mat3 = glm::mat3;
	using mat4 = glm::mat4;
	using mat4x3 = glm::mat4x3;
	using mat3x4 = glm::mat3x4;
	using uvec4 = glm::uvec4;

	enum LightType
	{
		None = 0,
		POINT = 1,
		DIRECTIONAL = 2,
		SPOT = 3,
	};

	struct LightGLSL
	{
		vec3 position;
		// light flags composition:
		// bits [00..15]: general flags
		// - bits [00..07]: light type (id)
		// - bit 8: enable shadow map
		// bits [16..31]: specific flags
		uint32_t flags;
		vec3 emission;
		int pad0;
		uvec4 textures;
		mat4 matrix;

		static LightGLSL MakePoint(vec3 position, vec3 emission);

		static LightGLSL MakeDirectional(vec3 dir, vec3 emission);

		LightGLSL transform(mat4 const& mat) const;
	};

	class Light : public VkObject
	{
	public:
		static constexpr uint32_t shadowMapBitIndex()
		{
			return 8;	
		}
	protected:

		LightType _type;
		vec3 _emission;
		bool _enable_shadow_map;

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			LightType type = LightType::None;
			vec3 emission = vec3(0);
			bool enable_shadow_map = true;
		};
		using CI = CreateInfo;

		Light(CreateInfo const& ci);

		constexpr LightType type()const
		{
			return _type;
		}

		virtual LightGLSL getAsGLSL(mat4 const& xform) const = 0;

		virtual void declareGui(GuiContext & ctx);

		bool enableShadowMap()const
		{
			return _enable_shadow_map;
		}

		void setEnableShadowMap(bool enable = true)
		{
			_enable_shadow_map = enable;
		}

		virtual uint32_t flags() const;
	};

	class PointLight : public Light
	{
	protected:

		vec3 _position;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			vec3 position = vec3(0);
			vec3 emission = vec3(0);
			bool enable_shadow_map = true;
		};
		using CI = CreateInfo;

		PointLight(CreateInfo const& ci);

		virtual LightGLSL getAsGLSL(mat4 const& xform) const override;

		virtual void declareGui(GuiContext& ctx) override;

		virtual uint32_t flags() const override;
	};

	class DirectionalLight : public Light
	{
	protected:

		vec3 _direction;

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			vec3 direction = vec3(1, 0, 0);
			vec3 emission = vec3(0);
		};
		using CI = CreateInfo;

		DirectionalLight(CreateInfo const& ci);

		virtual LightGLSL getAsGLSL(mat4 const& xform) const override;

		virtual void declareGui(GuiContext& ctx) override;

		virtual uint32_t flags() const override;
	};

	class SpotLight : public Light
	{
	protected:

		vec3 _position;
		vec3 _direction;
		vec3 _up;
		float _ratio;
		float _fov;
		uint8_t _attenuation = 0;
		float _znear = 1e-4;
		bool _preserve_intensity_from_fov = false;
		
	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			vec3 position = vec3(0);
			vec3 direction = vec3(1, 0, 0);
			vec3 up = vec3(0, 1, 0);
			vec3 emission = vec3(0);
			float aspect_ratio = 1;
			float fov = glm::radians(90.0f);
			uint8_t attenuation = 0;
			bool enable_shadow_map = true;
		};
		using CI = CreateInfo;

		SpotLight(CreateInfo const& ci);
		
		virtual LightGLSL getAsGLSL(mat4 const& xform) const override;

		virtual void declareGui(GuiContext& ctx) override;

		virtual uint32_t flags() const override;
	};
}