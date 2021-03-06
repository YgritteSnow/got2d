#include "render_system.h"
#include <d3dcompiler.h>
#include <assert.h>
#pragma comment(lib,"d3dcompiler.lib")

bool Shader::Create(const char* vsCode, const char* psCode)
{
	auto vsCodeLength = strlen(vsCode) + 1;
	auto psCodeLength = strlen(psCode) + 1;

	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	do
	{

		//compile shader
		auto ret = ::D3DCompile(
			vsCode, vsCodeLength,
			NULL, NULL, NULL,
			"VSMain", "vs_5_0",
			0, 0,
			&vsBlob, &errorBlob);

		if (S_OK != ret)
		{
			const char* reason = (const char*)errorBlob->GetBufferPointer();
			assert(0);
			errorBlob->Release();
			errorBlob = nullptr;
			break;
		}

		ret = ::D3DCompile(
			psCode, psCodeLength,
			NULL, NULL, NULL,
			"PSMain", "ps_5_0",
			0, 0,
			&psBlob, &errorBlob);

		if (S_OK != ret)
		{
			const char* reason = (const char*)errorBlob->GetBufferPointer();
			assert(0);
			errorBlob->Release();
			errorBlob = nullptr;
			break;
		}

		// create shader
		ret = GetRenderSystem()->GetDevice()->CreateVertexShader(
			vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(),
			NULL,
			&m_vertexShader);

		if (S_OK != ret)
			break;

		ret = GetRenderSystem()->GetDevice()->CreatePixelShader(
			psBlob->GetBufferPointer(),
			psBlob->GetBufferSize(),
			NULL,
			&m_pixelShader);

		if (S_OK != ret)
			break;

		D3D11_INPUT_ELEMENT_DESC layoutDesc[3];
		::ZeroMemory(layoutDesc, sizeof(layoutDesc));

		layoutDesc[0].SemanticName = "POSITION";
		layoutDesc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
		layoutDesc[0].AlignedByteOffset = 0;
		layoutDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		layoutDesc[1].SemanticName = "TEXCOORD";
		layoutDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		layoutDesc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		layoutDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		layoutDesc[2].SemanticName = "COLOR";
		layoutDesc[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		layoutDesc[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		layoutDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

		ret = GetRenderSystem()->GetDevice()->CreateInputLayout(
			layoutDesc, sizeof(layoutDesc) / sizeof(layoutDesc[0]),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			&m_shaderLayout);

		if (S_OK != ret)
			break;

		SR(vsBlob);
		SR(psBlob);
		return true;
	} while (false);

	SR(vsBlob);
	SR(psBlob);
	Destroy();
	return false;
}
void Shader::Destroy()
{
	SR(m_vertexShader);
	SR(m_pixelShader);
	SR(m_shaderLayout);
}

ID3D11VertexShader* Shader::GetVertexShader() { return m_vertexShader; }
ID3D11PixelShader* Shader::GetPixelShader() { return m_pixelShader; }
ID3D11InputLayout* Shader::GetInputLayout() { return m_shaderLayout; }

class SimpleColorShader : public ShaderSource
{
	const char* GetShaderName() override
	{
		return "simple.color";
	}
	const char* GetVertexShaderCode() override
	{
		return R"(
			cbuffer scene
			{
				float4x4 matrixProj;
				float4x4 matrixWorld;
			}
			struct GeometryVertex
			{
				float2 position : POSITION;
				float2 texcoord : TEXCOORD0;
				float4 vtxcolor : COLOR;
			};
			struct VertexOutput
			{
				float4 position : SV_POSITION;
				float4 vtxcolor : COLOR;
			};
			VertexOutput VSMain(GeometryVertex input)
			{
				VertexOutput output;
				output.position = mul(float4(input.position, 0, 1), matrixProj);
				output.vtxcolor = input.vtxcolor;
				return output;
			}
		)";
	}
	const char* GetPixelShaderCode() override
	{
		return R"(
			struct VertexInput
			{
				float4 position : SV_POSITION;
				float4 vtxcolor : COLOR;
			};
			float4 PSMain(VertexInput input):SV_TARGET
			{
				return input.vtxcolor;
			}
		)";
	}
};

class SimpleTextureShader : public ShaderSource
{
	const char* GetShaderName() override
	{
		return "simple.texture";
	}
	const char* GetVertexShaderCode() override
	{
		return R"(
			cbuffer scene
			{
				float4x4 matrixProj;
				float4x4 matrixWorld;
			}
			struct GeometryVertex
			{
				float2 position : POSITION;
				float2 texcoord : TEXCOORD0;
				float4 vtxcolor : COLOR;
			};
			struct VertexOutput
			{
				float4 position : SV_POSITION;
				float2 texcoord : TEXCOORD0;
			};
			VertexOutput VSMain(GeometryVertex input)
			{
				VertexOutput output;
				output.position = mul(float4(input.position, 0, 1), matrixProj);
				output.texcoord = input.texcoord;
				return output;
			}
		)";
	}
	const char* GetPixelShaderCode() override
	{
		return R"(
			Texture2D Tex;
			SamplerState State;
			struct VertexInput
			{
				float4 position : SV_POSITION;
				float2 texcoord : TEXCOORD0;
			};
			float4 PSMain(VertexInput input):SV_TARGET
			{
				return Tex.Sample(State, input.texcoord);
			}
		)";
	}
};

class DefaultShader : public ShaderSource
{
	const char* GetShaderName() override
	{
		return "default";
	}
	const char* GetVertexShaderCode() override
	{
		return R"(
			cbuffer scene
			{
				float4x4 matrixProj;
				float4x4 matrixWorld;
			}
			struct GeometryVertex
			{
				float2 position : POSITION;
				float2 texcoord : TEXCOORD0;
				float4 vtxcolor : COLOR;
			};
			struct VertexOutput
			{
				float4 position : SV_POSITION;
				float2 texcoord : TEXCOORD0;
				float4 vtxcolor : COLOR;
			};
			VertexOutput VSMain(GeometryVertex input)
			{
				VertexOutput output;
				output.position = mul(float4(input.position, 0, 1), matrixProj);
				output.texcoord = input.texcoord;
				output.vtxcolor = input.vtxcolor;
				return output;
			}
		)";
	}
	const char* GetPixelShaderCode() override
	{
		return R"(
			Texture2D Tex;
			SamplerState State;
			struct VertexInput
			{
				float4 position : SV_POSITION;
				float2 texcoord : TEXCOORD0;
				float4 vtxcolor : COLOR;
			};
			float4 PSMain(VertexInput input):SV_TARGET
			{
				return input.vtxcolor * Tex.Sample(State, input.texcoord);
			}
		)";
	}
};


ShaderLib::ShaderLib()
{
	ShaderSource* p = new DefaultShader();
	m_sources[p->GetShaderName()] = p;

	p = new SimpleTextureShader();
	m_sources[p->GetShaderName()] = p;

	p = new SimpleColorShader();
	m_sources[p->GetShaderName()] = p;
}

Shader* ShaderLib::GetShaderByName(const char* name)
{
	std::string nameStr = name;
	if (!m_shaders.count(nameStr))
	{
		if (!BuildShader(nameStr))
		{
			return false;
		}
	}
	return m_shaders[nameStr];
}

bool ShaderLib::BuildShader(const std::string& name)
{
	if (!m_sources.count(name))
	{
		return false;
	}
	ShaderSource* shaderRes = m_sources[name];
	Shader* shader = new Shader();
	if (shader->Create(shaderRes->GetVertexShaderCode(), shaderRes->GetPixelShaderCode()))
	{
		m_shaders[name] = shader;
		return true;
	}
	delete shader;
	m_sources.erase(name);

	return false;
}

g2d::Pass::~Pass() {}

g2d::Material::~Material() {}

Pass::Pass(const Pass& other)
	: m_effectName(other.m_effectName)
	, m_textures(other.m_textures.size())
	, m_vsConstants(other.m_vsConstants.size())
	, m_psConstants(other.m_psConstants.size())
{
	for (size_t i = 0, n = m_textures.size(); i < n; i++)
	{
		m_textures[i] = other.m_textures[i];
		m_textures[i]->AddRef();
	}

	if (m_vsConstants.size() > 0)
	{
		memcpy(&(m_vsConstants[0]), &(other.m_vsConstants[0]), m_vsConstants.size() * sizeof(float));
	}
	if (m_psConstants.size() > 0)
	{
		memcpy(&(m_psConstants[0]), &(other.m_psConstants[0]), m_psConstants.size() * sizeof(float));
	}
}

Pass::~Pass()
{
	for (auto& t : m_textures)
	{
		t->Release();
	}
	m_textures.clear();
}

Pass* Pass::Clone()
{
	Pass* p = new Pass(*this);
	return p;
}
bool Pass::IsSame(g2d::Pass* other) const
{
	Pass* p = dynamic_cast<Pass*>(other);
	if (p == nullptr)
		return false;

	if (m_effectName != p->m_effectName)
		return false;

	if (m_textures.size() != p->m_textures.size() ||
		m_vsConstants.size() != p->m_vsConstants.size() ||
		m_psConstants.size() != p->m_psConstants.size())
	{
		return false;
	}

	for (size_t i = 0, n = m_textures.size(); i < n; i++)
	{
		if (m_textures[i]->IsSame(p->m_textures[i]))
		{
			return false;
		}
	}

	if (m_vsConstants.size() > 0 &&
		0 != memcmp(&(m_vsConstants[0]), &(p->m_vsConstants[0]), m_vsConstants.size() * sizeof(float)))
	{
		return  false;
	}

	if (m_psConstants.size() > 0 &&
		0 != memcmp(&(m_psConstants[0]), &(p->m_psConstants[0]), m_psConstants.size() * sizeof(float)))
	{
		return false;
	}


	return true;
}

void Pass::SetTexture(unsigned int index, g2d::Texture* tex, bool autoRelease)
{
	size_t size = m_textures.size();
	if (index >= size)
	{
		m_textures.resize(index + 1);
		for (size_t i = size; i < index; i++)
		{
			m_textures[i] = nullptr;
		}
	}

	if (m_textures[index])
	{
		m_textures[index]->Release();
	}
	m_textures[index] = tex;
	if (!autoRelease)
	{
		m_textures[index]->AddRef();
	}
}
void Pass::SetVSConstant(unsigned int index, float* data, unsigned int size, unsigned int count)
{
	if (count == 0)
		return;

	if (index + count > m_vsConstants.size())
	{
		m_vsConstants.resize(index + count);
	}

	for (unsigned int i = 0; i < count; i++)
	{
		memcpy(&(m_vsConstants[index + i]), data + i*size, size);
	}
}

void Pass::SetPSConstant(unsigned int index, float* data, unsigned int size, unsigned int count)
{
	if (count == 0)
		return;

	if (index + count > m_vsConstants.size())
	{
		m_vsConstants.resize(index + count);
	}

	for (unsigned int i = 0; i < count; i++)
	{
		memcpy(&(m_vsConstants[index + i]), data + i*size, size);
	}
}

Material::Material(unsigned int passCount)
	: m_passes(passCount)
{

}

Material::Material(const Material& other)
	: m_passes(other.m_passes.size())
{
	for (size_t i = 0, n = m_passes.size(); i < n; i++)
	{
		m_passes[i] = other.m_passes[i]->Clone();
	}
}

void Material::SetPass(unsigned int index, Pass* p)
{
	assert(index < m_passes.size());
	m_passes[index] = p;
}


Material::~Material()
{
	for (auto& p : m_passes)
	{
		p->Release();
	}
	m_passes.clear();
}

g2d::Pass* Material::GetPass(unsigned int index) const
{
	if (m_passes.size() <= index)
		return nullptr;
	return m_passes[index];
}

unsigned int Material::GetPassCount() const
{
	return static_cast<unsigned int>(m_passes.size());
}

bool Material::IsSame(g2d::Material* other) const
{
	if (other->GetPassCount() != GetPassCount())
		return false;

	for (unsigned int i = 0; i < GetPassCount(); i++)
	{
		if (!GetPass(i)->IsSame(other->GetPass(i)))
		{
			return false;
		}
	}
	return true;
}

g2d::Material* Material::Clone() const
{
	Material* newMat = new Material(*this);
	return newMat;
}

void Material::Release()
{
	delete this;
}