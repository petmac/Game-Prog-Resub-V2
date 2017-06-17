#include <platform/vita/graphics/vertex_buffer_vita.h>
#include <platform/vita/system/platform_vita.h>

void *graphicsAlloc(SceKernelMemBlockType type, uint32_t size, uint32_t alignment, uint32_t attribs, SceUID *uid);
void graphicsFree(SceUID uid);

namespace gef
{
	VertexBuffer*  VertexBuffer::Create(Platform& platform)
	{
		return new VertexBufferVita();
	}

	VertexBufferVita::VertexBufferVita() :
		vertices_(NULL),
		vertices_uid_(0)
	{
	}

	VertexBufferVita::~VertexBufferVita()
	{
		graphicsFree(vertices_uid_);
	}

	bool VertexBufferVita::Init(const Platform& platform, const void* vertices, const UInt32 num_vertices, const UInt32 vertex_byte_size, const bool read_only)
	{
		num_vertices_ = num_vertices;
		vertex_byte_size_ = vertex_byte_size;

		// GRC FIXME - read_only is not being processed properly
		UInt32 attributes = SCE_GXM_MEMORY_ATTRIB_READ;

		if (!read_only)
			attributes |= SCE_GXM_MEMORY_ATTRIB_WRITE;

		// if copy vertex data is set then we're assuming this buffer will be getting updated, so let's make it dynamic

		if (vertices && num_vertices && vertex_byte_size)
		{
			//			PlatformVita& platform_vita = static_cast<PlatformVita&>(platform);

			// create shaded triangle vertex/index data
			vertices_ = graphicsAlloc(
				SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
				num_vertices*vertex_byte_size,
				4,
				SCE_GXM_MEMORY_ATTRIB_READ,
				&vertices_uid_);

			memcpy(vertices_, vertices, num_vertices*vertex_byte_size);

		}

		return true;
	}

	void VertexBufferVita::Bind(const Platform& platform) const
	{
		const PlatformVita& platform_vita = static_cast<const PlatformVita&>(platform);
		SceGxmContext* context = platform_vita.context();

		sceGxmSetVertexStream(context, 0, vertices_);
	}

	void VertexBufferVita::Unbind(const Platform& platform) const
	{
	}


	bool VertexBufferVita::Update(const Platform& platform)
	{
		bool success = true;

		memcpy(vertices_, vertex_data(), num_vertices()*vertex_byte_size());


		return success;
	}

}
