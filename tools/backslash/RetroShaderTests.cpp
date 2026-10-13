// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// Standalone WARP checks for the production built-in retro shader. No app registration.
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr) { if (FAILED(hr)) { throw std::runtime_error("D3D operation failed"); } }
static ComPtr<ID3DBlob> Compile(const wchar_t* path, const char* target)
{
    ComPtr<ID3DBlob> code, errors;
    const auto hr = D3DCompileFromFile(path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", target, D3DCOMPILE_ENABLE_STRICTNESS, 0, &code, &errors);
    if (FAILED(hr) && errors) { std::fprintf(stderr, "%s", static_cast<const char*>(errors->GetBufferPointer())); }
    Check(hr);
    return code;
}
static void Near(float actual, float expected, const char* label)
{
    if (std::abs(actual - expected) > 0.00001f) { std::fprintf(stderr, "%s: got %g, expected %g\n", label, actual, expected); throw std::runtime_error(label); }
    std::printf("PASS: %s (%g)\n", label, actual);
}
int wmain(int argc, wchar_t** argv)
try
{
    if (argc != 3) { throw std::runtime_error("Expected production vertex and pixel shader paths"); }
    const auto vsCode = Compile(argv[1], "vs_4_0");
    const auto psCode = Compile(argv[2], "ps_4_0");
    ComPtr<ID3D11ShaderReflection> reflection;
    Check(D3DReflect(psCode->GetBufferPointer(), psCode->GetBufferSize(), IID_PPV_ARGS(&reflection)));
    for (const auto& entry : std::array<std::pair<const char*, UINT>, 2>{{ {"PixelShaderSettings",32}, {"RetroShaderSettings",16} }})
    {
        D3D11_SHADER_BUFFER_DESC desc{};
        Check(reflection->GetConstantBufferByName(entry.first)->GetDesc(&desc));
        if (desc.Size != entry.second) { throw std::runtime_error("Shader constant-buffer size mismatch"); }
        std::printf("PASS: reflected %s size %u\n", entry.first, desc.Size);
    }
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    Check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context));
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    Check(device->CreateVertexShader(vsCode->GetBufferPointer(),vsCode->GetBufferSize(),nullptr,&vs));
    Check(device->CreatePixelShader(psCode->GetBufferPointer(),psCode->GetBufferSize(),nullptr,&ps));
    constexpr UINT size=32;
    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width=size; textureDesc.Height=size; textureDesc.MipLevels=1; textureDesc.ArraySize=1;
    textureDesc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT; textureDesc.SampleDesc.Count=1;
    textureDesc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    std::vector<std::array<float,4>> source(size*size,{0.25f,0.25f,0.25f,1.0f});
    D3D11_SUBRESOURCE_DATA initial{source.data(),size*16,0};
    ComPtr<ID3D11Texture2D> input, output, readback;
    Check(device->CreateTexture2D(&textureDesc,&initial,&input));
    ComPtr<ID3D11ShaderResourceView> srv;
    Check(device->CreateShaderResourceView(input.Get(),nullptr,&srv));
    textureDesc.BindFlags=D3D11_BIND_RENDER_TARGET;
    Check(device->CreateTexture2D(&textureDesc,nullptr,&output));
    ComPtr<ID3D11RenderTargetView> rtv;
    Check(device->CreateRenderTargetView(output.Get(),nullptr,&rtv));
    textureDesc.BindFlags=0; textureDesc.Usage=D3D11_USAGE_STAGING; textureDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    Check(device->CreateTexture2D(&textureDesc,nullptr,&readback));
    const float b0Data[8]{0,1,size,size,0,0,0,1};
    D3D11_BUFFER_DESC bufferDesc{}; bufferDesc.ByteWidth=32; bufferDesc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA bufferInitial{b0Data,0,0};
    ComPtr<ID3D11Buffer> b0,b1;
    Check(device->CreateBuffer(&bufferDesc,&bufferInitial,&b0));
    bufferDesc.ByteWidth=16;
    Check(device->CreateBuffer(&bufferDesc,nullptr,&b1));
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU=samplerDesc.AddressV=samplerDesc.AddressW=D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.MaxLOD=D3D11_FLOAT32_MAX; samplerDesc.ComparisonFunc=D3D11_COMPARISON_NEVER;
    ComPtr<ID3D11SamplerState> sampler;
    Check(device->CreateSamplerState(&samplerDesc,&sampler));
    D3D11_RASTERIZER_DESC rasterDesc{}; rasterDesc.FillMode=D3D11_FILL_SOLID; rasterDesc.CullMode=D3D11_CULL_NONE;
    ComPtr<ID3D11RasterizerState> raster;
    Check(device->CreateRasterizerState(&rasterDesc,&raster));
    context->RSSetState(raster.Get());
    const D3D11_VIEWPORT viewport{0,0,size,size,0,1};
    context->RSSetViewports(1,&viewport);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->VSSetShader(vs.Get(),nullptr,0);
    context->PSSetShader(ps.Get(),nullptr,0);
    context->PSSetShaderResources(0,1,srv.GetAddressOf());
    context->PSSetSamplers(0,1,sampler.GetAddressOf());
    ID3D11Buffer* buffers[]{b0.Get(),b1.Get()};
    context->PSSetConstantBuffers(0,2,buffers);
    auto render=[&](float glow,float scanline) {
        const float values[4]{glow,scanline,0,0};
        context->UpdateSubresource(b1.Get(),0,nullptr,values,0,0);
        context->OMSetRenderTargets(1,rtv.GetAddressOf(),nullptr);
        context->Draw(4,0);
        context->OMSetRenderTargets(0,nullptr,nullptr);
        context->CopyResource(readback.Get(),output.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        Check(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped));
        const auto row=[&](UINT y) { return reinterpret_cast<const float*>(static_cast<const char*>(mapped.pData)+y*mapped.RowPitch)[16*4]; };
        const std::array<float,2> result{row(16),row(17)};
        context->Unmap(readback.Get(),0);
        return result;
    };
    auto neutral=render(0,0);
    Near(neutral[0],0.25f,"Both zero preserves even row");
    Near(neutral[1],0.25f,"Both zero preserves odd row");
    auto scan=render(0,0.5f);
    Near(scan[0],0.25f,"Scanlines preserve even row");
    Near(scan[1],0.125f,"Scanline 0.5 halves odd row");
    Near(render(0,1)[1],0,"Scanline 1 zeros odd row");
    auto glow=render(0.3f,0);
    if (glow[0]<=neutral[0]) { throw std::runtime_error("Glow did not increase RGB"); }
    Near(glow[0],glow[1],"Scanline zero leaves glow uniform");
    auto defaults=render(0.3f,0.5f);
    Near(defaults[0],glow[0],"Default gain preserves even glow row");
    Near(defaults[1],glow[1]*0.5f,"Default scanline halves glow row");
    Near(render(1,0)[0]-0.25f,(glow[0]-0.25f)/0.3f,"Glow scales linearly through same shader");
    Near(render(0,0)[1],0.25f,"Live parameter reset restores neutral output");
    std::puts("WARP retro shader tests passed");
    return 0;
}
catch (const std::exception& e) { std::fprintf(stderr,"FAIL: %s\n",e.what()); return 1; }
