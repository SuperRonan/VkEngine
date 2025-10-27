#pragma once

#include <vkl/App/VkApplication.hpp>
#include <vkl/Maths/Transforms.hpp>
#include <vkl/IO/GuiContext.hpp>

#define EMISSION_FLAG_BLACK_BODY_BIT 0x1
#define EMISSION_BLACK_BODY_NORMALIZATION_BIT_INDEX 0x1
#define EMISSION_BLACK_BODY_NORMALIZATION_BIT_COUNT 0x2

namespace vkl
{
	static constexpr uint8_t BlackBodyEmission(uint norm)
	{
		uint8_t res = EMISSION_FLAG_BLACK_BODY_BIT;
		if (norm)
		{
			res |= uint8_t((norm & std::bitMask<uint8_t>(EMISSION_BLACK_BODY_NORMALIZATION_BIT_COUNT)) << uint8_t(EMISSION_BLACK_BODY_NORMALIZATION_BIT_INDEX));
		}
		return res;
	}

	static constexpr uint ExtractBlackBodyEmissionNorm(uint8_t options)
	{
		return (options >> uint8_t(EMISSION_BLACK_BODY_NORMALIZATION_BIT_INDEX)) & std::bitMask<uint8_t>(EMISSION_BLACK_BODY_NORMALIZATION_BIT_COUNT);
	}


	using vec2 = Vector2f;
	using vec3 = Vector3f;
	using vec4 = Vector4f;
	using mat3 = Matrix3f;
	using mat4 = Matrix4f;
	using uvec4 = Vector4u;

	enum LightType
	{
		None = 0,
		Point = 1,
		Directional = 2,
		Spot = 3,
		Beam = 4,
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
		uint32_t shadow_bias_data;
		
		uvec4 textures;
		
		vec3 direction;
		float z_near;

		struct SpotSpecific
		{
			vec3 up;
			float tan_half_fov;
			float aspect;
		};

		struct BeamSpecific
		{
			vec3 up;
			float radius;
			float aspect;
		};

		union
		{
			uint32_t extra_data[16] = {};
			SpotSpecific spot;
			BeamSpecific beam;
		};

		LightGLSL transform(Matrix3x4f const& mat) const;
	};

	class Light : public VkObject
	{
	public:
		static constexpr uint32_t shadowMapBitIndex()
		{
			return 8;
		}

		enum class ShadowBiasMode
		{
			None = 0,
			Offset = 1,
			FloatMult = 2,
			FloatAdd = 3,
			MAX_ENUM,
		};

	protected:

		LightType _type;
		vec3 _emission;
		uint8_t _emission_options = 0;
		bool _enable_shadow_map;
		ShadowBiasMode _shadow_bias_mode = ShadowBiasMode::FloatMult;
		bool _shadow_bias_include_cos_theta = true;
		union
		{
			int _int_shadow_bias;
			float _float_shadow_bias = 0.99999;
		};

	public:

		struct CreateInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			LightType type = LightType::None;
			vec3 emission = vec3::Zero();
			uint8_t emission_options = 0;
			bool enable_shadow_map = true;
		};
		using CI = CreateInfo;

		Light(CreateInfo const& ci);

		constexpr LightType type()const
		{
			return _type;
		}

		virtual LightGLSL getAsGLSL(Matrix3x4f const& xform) const = 0;

		virtual void declareGui(GuiContext & ctx);

		static bool DeclareEmission(vec3& emission, uint8_t& options);

		static vec3 NormalizeEmission(vec3 emission, uint8_t options, float norm=1);

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
	public:

		static constexpr float DefaultZNear()
		{
			return 0.1;
		}

	protected:

		vec3 _position;
		float _z_near = DefaultZNear();

	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			vec3 position = vec3::Zero();
			vec3 emission = vec3::Zero();
			uint8_t emission_options = 0;
			bool enable_shadow_map = true;
			float z_near = DefaultZNear();
		};
		using CI = CreateInfo;

		PointLight(CreateInfo const& ci);

		virtual LightGLSL getAsGLSL(Matrix3x4f const& xform) const override;

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
			vec3 emission = vec3::Zero();
			uint8_t emission_options = 0;
		};
		using CI = CreateInfo;

		DirectionalLight(CreateInfo const& ci);

		virtual LightGLSL getAsGLSL(Matrix3x4f const& xform) const override;

		virtual void declareGui(GuiContext& ctx) override;

		virtual uint32_t flags() const override;
	};

	class SpotBeamLight : public Light
	{
	protected:
		
		vec3 _position;
		vec3 _direction;
		vec3 _up;
		float _opening; // Spot: fov, Beam: radius
		float _ratio;
		uint8_t _attenuation = 0;
		float _znear = 1e-4;
		bool _preserve_intensity_from_opening = false;
		bool _is_beam = false;
		
	public:

		struct CreateInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			vec3 position = vec3::Zero();
			vec3 direction = vec3(1, 0, 0);
			vec3 up = vec3(0, 1, 0);
			vec3 emission = vec3::Zero();
			float aspect_ratio = 1;
			float opening = Radians(90.0f);
			uint8_t attenuation = 0;
			uint8_t emission_options = 0;
			bool enable_shadow_map = true;
			bool is_beam;
		};

		struct CreateSpotInfo
		{
			VkApplication * app = nullptr;
			std::string name = {};
			vec3 position = vec3::Zero();
			vec3 direction = vec3(1, 0, 0);
			vec3 up = vec3(0, 1, 0);
			vec3 emission = vec3::Zero();
			float aspect_ratio = 1;
			float fov = Radians(90.0f);
			uint8_t attenuation = 0;
			uint8_t emission_options = 0;
			bool enable_shadow_map = true;
		};

		struct CreateBeamInfo
		{
			VkApplication* app = nullptr;
			std::string name = {};
			vec3 position = vec3::Zero();
			vec3 direction = vec3(1, 0, 0);
			vec3 up = vec3(0, 1, 0);
			vec3 emission = vec3::Zero();
			float aspect_ratio = 1;
			float opening = 0;
			uint8_t attenuation = 0;
			uint8_t emission_options = 0;
			//bool enable_shadow_map = true;
		};

		using CI = CreateInfo;

		SpotBeamLight(CreateInfo const& ci);

		SpotBeamLight(CreateSpotInfo const& ci):
			SpotBeamLight(CreateInfo{
				.app = ci.app,
				.name = ci.name,
				.position = ci.position,
				.direction = ci.direction,
				.up = ci.up,
				.emission = ci.emission,
				.aspect_ratio = ci.aspect_ratio,
				.opening = ci.fov,
				.attenuation = ci.attenuation,
				.emission_options = ci.emission_options,
				.enable_shadow_map = ci.enable_shadow_map,
				.is_beam = false,
			})
		{}

		SpotBeamLight(CreateBeamInfo const& ci) :
			SpotBeamLight(CreateInfo{
				.app = ci.app,
				.name = ci.name,
				.position = ci.position,
				.direction = ci.direction,
				.up = ci.up,
				.emission = ci.emission,
				.aspect_ratio = ci.aspect_ratio,
				.opening = ci.opening,
				.attenuation = ci.attenuation,
				.emission_options = ci.emission_options,
				.enable_shadow_map = false,
				.is_beam = true,
			})
		{}
		
		virtual LightGLSL getAsGLSL(Matrix3x4f const& xform) const override;

		virtual void declareGui(GuiContext& ctx) override;

		virtual uint32_t flags() const override;
	};

	using SpotLight = SpotBeamLight;
	using BeamLight = SpotBeamLight;
}