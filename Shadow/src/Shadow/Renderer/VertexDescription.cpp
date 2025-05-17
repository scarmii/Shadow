#include"shpch.hpp"
#include"VertexDescription.hpp"

namespace Shadow
{

	uint32_t VertexAttribute::getComponentCount() const
	{
		if (type == VertexAttribType::Float || type == VertexAttribType::Uint || type == VertexAttribType::Int)
			return 1;
		else if (type == VertexAttribType::Vec2f || type == VertexAttribType::Vec2u || type == VertexAttribType::Vec2i)
			return 2;
		else if (type == VertexAttribType::Vec3f || type == VertexAttribType::Vec3u || type == VertexAttribType::Vec3i)
			return 3;
		else if (type == VertexAttribType::Vec4f || type == VertexAttribType::Vec4u || type == VertexAttribType::Vec4i)
			return 4;
		else if (type == VertexAttribType::Mat4x4)
			return 16;	
		return 0;
	}

	VertexInput::VertexInput(std::initializer_list<VertexAttribute> vertexAttribs)
		: m_vertexAttribs(vertexAttribs)
	{
		uint32_t calculatedOffset = 0;
		m_stride = 0;

		for (size_t i = 0; i < m_vertexAttribs.size(); i++)
		{
			VertexAttribute& vertexAttrib = m_vertexAttribs[i];
			if (vertexAttrib.type == VertexAttribType::Mat4x4)
			{
				auto iter = m_vertexAttribs.cbegin() + i;
				m_vertexAttribs.insert(iter,
					{VertexAttribType::Vec4f, VertexAttribType::Vec4f, 
					VertexAttribType::Vec4f, VertexAttribType::Vec4f });
				m_vertexAttribs.erase(m_vertexAttribs.cbegin()+i+4);
				i--;
				continue;
			}

			vertexAttrib.offset = vertexAttrib.offset ? vertexAttrib.offset : calculatedOffset;
			calculatedOffset += vertexAttrib.size;
			m_stride += vertexAttrib.size;
		}
	}
}