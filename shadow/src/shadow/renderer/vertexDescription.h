#pragma once

namespace Shadow
{
	enum class VertexAttribType
	{
		Float,
		Vec2f,
		Vec3f,
		Vec4f,

		Uint,
		Vec2u,
		Vec3u,
		Vec4u,

		Int,
		Vec2i,
		Vec3i,
		Vec4i,

		Mat4x4
	};

	struct VertexAttribute
	{
		VertexAttribType type;
		uint32_t offset = 0;
		uint32_t size;

		// 4 is the size (in bytes) of float, int and uint
		VertexAttribute(VertexAttribType type)
			: type(type), size(4 * getComponentCount())
		{
		}

		uint32_t getComponentCount() const;
	};

	class VertexInput
	{
	public:
		VertexInput(std::initializer_list<VertexAttribute> vertexAttrbs);

		inline uint32_t getStride() const { return m_stride; }
		inline const std::vector<VertexAttribute>& getVertexAttribs() const { return m_vertexAttribs; }
	private:
		uint32_t m_stride = 0;
		std::vector<VertexAttribute> m_vertexAttribs;
	};
}