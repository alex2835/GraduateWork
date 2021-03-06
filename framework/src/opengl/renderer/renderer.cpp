
#include "renderer.h"


namespace Bubble
{
	const Framebuffer* Renderer::sActiveViewport;
	RenderingMod Renderer::sActiveMod;
	Ref<VertexArray> Renderer::sFullScreenVAO;
	Ref<Shader> Renderer::sPassThroughShader;

	static uint32_t OpenGLDrawType(DrawType draw_type)
	{
		uint32_t opengl_draw_type = 0;
		switch (draw_type)
		{
			case DrawType::POINTS:
				opengl_draw_type = GL_POINT;
				break;
			case DrawType::LINES:
				opengl_draw_type = GL_LINES;
				break;
			case DrawType::TRIANGLES:
				opengl_draw_type = GL_TRIANGLES;
				break;
			default:
				BUBBLE_CORE_ASSERT(false, "Unknown draw type");
		}
		return opengl_draw_type;
	}


	void Renderer::Init(RenderingMod start_mode)
	{
		LoadData();
		sActiveMod = start_mode;
		switch (start_mode)
		{
			case Bubble::RenderingMod::Graphic3D:
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_CULL_FACE);
				break;
			case Bubble::RenderingMod::Graphic2D:
			case Bubble::RenderingMod::ImageProccessing:
				// Nothing to set
				break;
			default:
				BUBBLE_CORE_ASSERT(false, "Mod not supported");
		}
	}


	// ==================== Options ====================

	void Renderer::Wareframe(bool on)
	{
		if (on) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	void Renderer::AlphaBlending(bool on, uint32_t sfactor, uint32_t dfactor)
	{
		if (on)
		{
			glEnable(GL_BLEND);
			glBlendFunc(sfactor, dfactor);
		}
		else {
			glDisable(GL_BLEND);
		}
	}

	void Renderer::BackfaceCulling(bool on)
	{
		on ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
	}

	void Renderer::DepthTest(bool on)
	{
		on ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	}


	// ==================== Set active components ====================

	void Renderer::SetViewport(const Framebuffer& framebuffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		sActiveViewport = &framebuffer;
		glm::ivec2 RenderPos;
		glm::ivec2 RenderSize;

		if (width > 0 && height > 0)
		{
			RenderPos = { x, y };
			RenderSize = { width, height };
		}
		else {
			RenderPos = { 0, 0 };
			RenderSize = { framebuffer.GetWidth(), framebuffer.GetHeight()};
		}

		framebuffer.Bind();
		glViewport(RenderPos.x, RenderPos.y, RenderSize.x, RenderSize.y);
	}


	// ==================== Clearing ====================

	void Renderer::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void Renderer::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Renderer::ClearDepth()
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void Renderer::ClearColor()
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}


	// ==================== Rendering ====================

	void Renderer::DrawIndices(const Ref<VertexArray>& vertex_array, DrawType draw_type, uint32_t index_count)
	{
		vertex_array->Bind();
		uint32_t count = index_count ? index_count : vertex_array->GetIndexBuffer().GetCount();
		glcall(glDrawElements(OpenGLDrawType(draw_type), count, GL_UNSIGNED_INT, nullptr));
	}

	void Renderer::DrawVertices(const Ref<VertexArray>& vertex_array, DrawType draw_type, uint32_t count)
	{
		vertex_array->Bind();
		count = count ? count : vertex_array->GetVertexBuffers()[0].GetSize();
		glcall(glDrawArrays(OpenGLDrawType(draw_type), 0, count));
	}


	// ==================== Image ====================

	Ref<Texture2D> Renderer::CopyTexture2D(const Ref<Texture2D>& texture)
	{
		Texture2D copy(texture->mSpecification);
		Framebuffer fb(std::move(copy));

		sPassThroughShader->SetTexture2D("uImage", texture->GetRendererID());
		fb.Bind();
		DrawIndices(sFullScreenVAO);
		return CreateRef<Texture2D>(fb.GetColorAttachment());
	}

	Texture2D Renderer::CopyTexture2D(const Texture2D& texture)
	{
		Texture2D copy(texture.mSpecification);
		Framebuffer fb(std::move(copy));

		sPassThroughShader->SetTexture2D("uImage", texture);
		fb.Bind();
		DrawIndices(sFullScreenVAO);
		return fb.GetColorAttachment();
	}


	void Renderer::DrawTexture2D(const Texture2D& src, Texture2D& dst)
	{
		Framebuffer fb(std::move(dst));

		fb.Bind();
		sPassThroughShader->SetTexture2D("uImage", src);
		DrawIndices(sFullScreenVAO);
		dst = fb.GetColorAttachment();
	}


    Texture2D Renderer::ResizeTexture2D(const Texture2D& src, int width, int height)
    {
        Texture2D dst(width, height);
        Renderer::DrawTexture2D(src, dst);
		return dst;
    }

    // ==================== Private methods ====================

	void Renderer::LoadData()
	{
		// Vertex array that covered full screen
		float quad_vertices[] = {
			// positions        // tex coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		uint32_t indies[] = { 0, 1, 2, 1, 2, 3 };

		VertexBuffer vb(quad_vertices, 4 * 5 * sizeof(float));
		BufferLayout layout
		{
			{ GLSLDataType::Float3, "Position" },
			{ GLSLDataType::Float2, "TexCoords" }
		};
		vb.SetLayout(layout);
		IndexBuffer ib(indies, 6);
		sFullScreenVAO = CreateRef<VertexArray>();
		sFullScreenVAO->AddVertexBuffer(std::move(vb));
		sFullScreenVAO->SetIndexBuffer(std::move(ib));

		// Pass through shader
		sPassThroughShader = ShaderLoader::Load("resources/shaders/pass_through.glsl");
	}

}