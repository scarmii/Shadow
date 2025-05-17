#pragma once

namespace Shadow
{
	enum class PrimitiveTopology : uint8_t
	{
		Point = 0,
		Line = 1,
		LineStrip = 3,
		Triangles = 4,
		TriangleStrip = 5
	};

	enum class PolygonMode : uint8_t
	{
		Fill,
		Line,
		Point
	};

	enum class CullMode : uint8_t
	{
		Front,
		Back,
		FrontAndBack
	};

	enum class FrontFace : uint8_t
	{
		CounterClockwise,
		Clockwise
	};

	enum class BlendFactor : uint8_t
	{
		Zero,
		One,
		SrcAlpha,
		DstAlpha,
		OneMinusSrcAlpha
	};

	enum class BlendOp : uint8_t
	{
		Add
	};

	struct BlendState
	{
		bool blendEnable = false;
		BlendOp colorBlendOp = BlendOp::Add, alphaBlendOp = BlendOp::Add;
		BlendFactor srcColorBlendFactor = BlendFactor::One, dstColorBlendFactor = BlendFactor::Zero;
		BlendFactor srcAlphaBlendFactor = BlendFactor::One, dstAlphaBlendFactor = BlendFactor::Zero;
	};

	struct GraphicsPipeStates
	{
		BlendState blendState;
		PrimitiveTopology primitiveTopology = PrimitiveTopology::Triangles;
		PolygonMode polygonMode = PolygonMode::Fill;
		CullMode cullMode = CullMode::Back;
		FrontFace frontFace = FrontFace::CounterClockwise; // TODO: a bit confused abt it

		bool primitiveRestartEnable = false;
		float lineWidth = 1.0f;
	};
}