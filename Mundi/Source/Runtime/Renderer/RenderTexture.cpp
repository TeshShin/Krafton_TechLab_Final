#include "pch.h"
#include "RenderTexture.h"

IMPLEMENT_CLASS(URenderTexture)

void URenderTexture::InitResolution(ID3D11Device* Device, const float InResolution, ID3D11Texture2D* FrameBufferTex)
{
	if (InResolution <= 0)
	{
		return;
	}
	if (Resolution != InResolution)
	{
		Resolution = InResolution;
		RenderTexSizeType = ERenderTexSizeType::Resolution;
		D3D11_TEXTURE2D_DESC TexDesc;
		FrameBufferTex->GetDesc(&TexDesc);
		TexWidth = static_cast<uint32>(std::round(TexDesc.Width * InResolution));
		TexHeight = static_cast<uint32>(std::round(TexDesc.Height * InResolution));
		CreateResources(Device, TexWidth, TexHeight);
	}
}
void URenderTexture::InitFixedSize(ID3D11Device* Device, const uint32 InWidth, const uint32 InHeight)
{
	if (TexWidth != InWidth || TexHeight != InHeight)
	{
		TexWidth = InWidth;
		TexHeight = InHeight;
		RenderTexSizeType = ERenderTexSizeType::Fixed;
		CreateResources(Device, TexWidth, TexHeight);
	}
}

void URenderTexture::CreateResources(ID3D11Device* Device, const uint32 Width, const uint32 Height)
{
	D3D11_TEXTURE2D_DESC TexDesc = {};
	TexDesc.Width = Width;
	TexDesc.Height = Height;
	TexDesc.MipLevels = 1;
	TexDesc.ArraySize = 1;
	TexDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	TexDesc.Usage = D3D11_USAGE_DEFAULT;
	TexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	RTVDesc.Texture2D.MipSlice = 0;

	Device->CreateTexture2D(&TexDesc, nullptr, TexResource.GetAddressOf());
	Device->CreateShaderResourceView(TexResource.Get(), &SRVDesc, SRV.GetAddressOf());
	Device->CreateRenderTargetView(TexResource.Get(), &RTVDesc, RTV.GetAddressOf());
}
