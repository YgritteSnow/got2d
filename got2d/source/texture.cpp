#include "render_system.h"

g2d::Texture::~Texture() {}

Texture::Texture(const char* resPath) : m_resPath(resPath)
{

}
bool Texture::IsSame(g2d::Texture* other) const
{
	Texture* timpl = dynamic_cast<Texture*>(other);
	if (timpl == nullptr)
		return false;
	return timpl->m_resPath == m_resPath;
}

void Texture::AddRef()
{
	m_refCount++;
}

void Texture::Release()
{
	if (--m_refCount == 0)
	{
		delete this;
	}
}


bool Texture2D::Create(unsigned int width, unsigned int height)
{
	if (width == 0 || height == 0)
		return false;

	D3D11_TEXTURE2D_DESC texDesc;

	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DYNAMIC;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	texDesc.MiscFlags = 0;
	if (S_OK != GetRenderSystem()->GetDevice()->CreateTexture2D(&texDesc, nullptr, &m_texture))
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	::ZeroMemory(&viewDesc, sizeof(viewDesc));
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = -1;
	viewDesc.Texture2D.MostDetailedMip = 0;
	if (S_OK != GetRenderSystem()->GetDevice()->CreateShaderResourceView(m_texture, &viewDesc, &m_shaderView))
	{
		Destroy();
		return false;
	}

	m_width = width;
	m_height = height;
	return true;
}

void Texture2D::UploadImage(unsigned char* data, bool hasAlpha)
{
	D3D11_MAPPED_SUBRESOURCE mappedRes;
	if (S_OK == GetRenderSystem()->GetContext()->Map(m_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedRes))
	{
		unsigned char* colorBuffer = static_cast<unsigned char*>(mappedRes.pData);
		if (hasAlpha)
		{
			int srcPitch = m_width * 4;
			for (unsigned int i = 0; i < m_height; i++)
			{
				auto dstPtr = colorBuffer + i * mappedRes.RowPitch;
				auto srcPtr = data + i * srcPitch;
				memcpy(dstPtr, srcPtr, srcPitch);
			}
		}
		else
		{
			int srcPitch = m_width * 3;
			for (unsigned int i = 0; i < m_height; i++)
			{
				auto dstPtr = colorBuffer + i * mappedRes.RowPitch;
				auto srcPtr = data + i * srcPitch;
				for (unsigned int j = 0; j < m_width; j++)
				{
					memcpy(dstPtr + j * 4, srcPtr + j * 3, 3);
				}
			}
		}

		GetRenderSystem()->GetContext()->Unmap(m_texture, 0);
		GetRenderSystem()->GetContext()->GenerateMips(m_shaderView);
	}

}

void Texture2D::Destroy()
{
	SR(m_texture);
	SR(m_shaderView);
	m_width = 0;
	m_height = 0;
}

#include "engine.h"
#include "file_data.h"
#include "img_data.h"

bool TexturePool::CreateDefaultTexture()
{
	if (m_defaultTexture.Create(2, 2))
	{
		unsigned char boardData[] =
		{
			0,0,0,255,255,255,
			255,255,255,0,0,0
		};
		m_defaultTexture.UploadImage(boardData, false);
		m_textures.insert(std::make_pair<>("", &m_defaultTexture));
		return true;
	}
	m_defaultTexture.Destroy();
	return false;
}

bool TexturePool::LoadTextureFromFile(std::string resourcePath)
{
	file_data f;
	if (!load_file(resourcePath.c_str(), f))
		return false;

	img_data img;
	auto result = true;
	auto ext = resourcePath.substr(resourcePath.length() - 3);
	if (ext == "bmp")
	{
		result = read_bmp(f.buffer, img);
	}
	else if (ext == "png")
	{
		result = read_png(f.buffer, img);
	}
	else if (ext == "tga")
	{
		result = read_tga(f.buffer, img);
	}
	else
	{
		result = false;
	}
	destroy_file_data(f);

	if (result)
	{
		auto tex = new ::Texture2D();
		result = tex->Create(img.width, img.height);
		if (result)
		{
			tex->UploadImage(img.raw_data, img.has_alpha);
			m_textures[resourcePath] = tex;
		}
		destroy_img_data(img);
		return result;
	}
	return false;
}

void TexturePool::Destroy()
{
	m_textures.erase("");
	m_defaultTexture.Destroy();
	for (auto& t : m_textures)
	{
		t.second->Destroy();
		delete t.second;
	}
	m_textures.clear();
}

Texture2D* TexturePool::GetTexture(const std::string& resource)
{
	if (m_textures.count(resource) == 0)
	{
		if (!LoadTextureFromFile(resource))
		{
			return GetDefaultTexture();
		}
	}

	return m_textures[resource];
}