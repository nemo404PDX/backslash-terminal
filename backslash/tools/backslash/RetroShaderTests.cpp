// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// Standalone WARP checks for the production built-in retro shader. No app registration.
#define NOMINMAX
#include <d3d11.h>
#include "../../src/cascadia/inc/PhosphorColor.h"
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <fstream>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr) { if (FAILED(hr)) { throw std::runtime_error("D3D operation failed"); } }
static ComPtr<ID3DBlob> Compile(const wchar_t* path, const char* target)
{
    ComPtr<ID3DBlob> code, errors;
    const auto hr = D3DCompileFromFile(path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", target, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0, &code, &errors);
    if (FAILED(hr) && errors) { std::fprintf(stderr, "%s", static_cast<const char*>(errors->GetBufferPointer())); }
    Check(hr);
    return code;
}
static void Near(float actual, float expected, const char* label)
{
    if (!std::isfinite(actual) || std::abs(actual - expected) > 0.00001f) { std::fprintf(stderr, "%s: got %g, expected %g\n", label, actual, expected); throw std::runtime_error(label); }
    std::printf("PASS: %s (%g)\n", label, actual);
}
int wmain(int argc, wchar_t** argv)
try
{
    if (argc < 3 || argc > 4) { throw std::runtime_error("Expected production vertex and pixel shader paths"); }
    const auto vsCode = Compile(argv[1], "vs_4_0");
    const auto psCode = Compile(argv[2], "ps_4_0");
    ComPtr<ID3D11ShaderReflection> reflection;
    Check(D3DReflect(psCode->GetBufferPointer(), psCode->GetBufferSize(), IID_PPV_ARGS(&reflection)));
    for (const auto& entry : std::array<std::pair<const char*, UINT>, 2>{{ {"PixelShaderSettings",32}, {"RetroShaderSettings",48} }})
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
    bufferDesc.ByteWidth=48;
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
    std::filesystem::path capture;
    auto render=[&](float glow,float scanline,float thickness=1.0f,float gap=1.0f,float dpiScale=1.0f,float tint=0.0f,std::array<float,3> phosphor={0,1,65.0f/255.0f},float white=0,float scheme=0) {
        const float settings[8]{0,dpiScale,size,size,0,0,0,1};
        context->UpdateSubresource(b0.Get(),0,nullptr,settings,0,0);
        const float values[12]{glow,scanline,thickness,gap,phosphor[0],phosphor[1],phosphor[2],tint,white,0,scheme,0};
        context->UpdateSubresource(b1.Get(),0,nullptr,values,0,0);
        context->OMSetRenderTargets(1,rtv.GetAddressOf(),nullptr);
        context->Draw(4,0);
        context->OMSetRenderTargets(0,nullptr,nullptr);
        context->CopyResource(readback.Get(),output.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        Check(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped));
        const auto row=[&](UINT y) { return reinterpret_cast<const float*>(static_cast<const char*>(mapped.pData)+y*mapped.RowPitch)[16*4]; };
        std::array<float,2+size*6> result{};
        result[0]=row(16); result[1]=row(17);
        for(UINT y=0;y<size;++y) {
            const auto pixels=reinterpret_cast<const float*>(static_cast<const char*>(mapped.pData)+y*mapped.RowPitch);
            result[2+y]=pixels[16*4];
            result[2+size+y]=pixels[16*4+3];
            for(UINT channel=0;channel<4;++channel) { result[2+size*2+y*4+channel]=pixels[16*4+channel]; }
        }
        if(!capture.empty()) {
            BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+size*size*4;
            BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=size;info.biHeight=-static_cast<LONG>(size);info.biPlanes=1;info.biBitCount=32;
            std::ofstream file(capture,std::ios::binary);file.write(reinterpret_cast<const char*>(&header),sizeof(header));file.write(reinterpret_cast<const char*>(&info),sizeof(info));
            for(UINT y=0;y<size;++y){const auto captureRow=reinterpret_cast<const float*>(static_cast<const char*>(mapped.pData)+y*mapped.RowPitch);for(UINT x=0;x<size;++x){const auto p=captureRow+x*4;const unsigned char bgra[]{static_cast<unsigned char>(std::lround(std::clamp(p[2],0.0f,1.0f)*255)),static_cast<unsigned char>(std::lround(std::clamp(p[1],0.0f,1.0f)*255)),static_cast<unsigned char>(std::lround(std::clamp(p[0],0.0f,1.0f)*255)),255};file.write(reinterpret_cast<const char*>(bgra),4);}}
            if(!file){throw std::runtime_error("GPU evidence capture write failed");}
        }
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
    auto coverage=[](double y,double thickness,double gap,double scale) {
        const double dark=thickness*scale, clear=gap*scale, period=dark+clear;
        auto integral=[&](double x) { const auto cycles=std::floor(x/period); return cycles*dark+std::clamp(x-cycles*period-clear,0.0,dark); };
        return integral(y+0.5)-integral(y-0.5);
    };
    for(const auto scale:std::array<float,5>{0.75f,1.0f,1.25f,1.5f,2.0f}) {
        for(const auto dimensions:std::array<std::array<float,2>,6>{{{1,1},{0.5f,0.5f},{0.5f,3},{4,1},{8,16},{8,0}}}) {
            const auto patternResult=render(0,0.5f,dimensions[0],dimensions[1],scale);
            for(UINT y=0;y<size;++y) {
                const auto mask=static_cast<float>(coverage(y+0.5,dimensions[0],dimensions[1],scale));
                Near(patternResult[2+y],0.25f*(1.0f-mask*0.5f),"Filtered scanline RGB matches pixel coverage");
                Near(patternResult[2+size+y],1.0f-mask*0.5f,"Filtered scanline retains legacy alpha modulation");
            }
            const auto zero=render(0,0,dimensions[0],dimensions[1],scale);
            for(UINT y=0;y<size;++y) { Near(zero[2+y],0.25f,"Zero intensity is identity for all dimensions/DPI"); }
        }
    }
    for(const auto scale:std::array<float,2>{1.0f,2.0f}) {
        const auto patternResult=render(0,0.5f,1,1,scale);
        for(UINT y=0;y<size;++y) { Near(patternResult[2+y],0.25f*(1.0f-(static_cast<int>((y+0.5f)/scale)%2)*0.5f),"Default integer-DPI pattern preserves legacy output"); }
    }
    const auto fractional=render(0,1,1,1,1.25f);
    Near(fractional[3],0.25f*0.25f,"125 percent DPI integrates partial dark band");
    Near(render(0,0.5f,8,0,1.5f)[0],0.125f,"Zero gap gives uniform dimming");
    auto setInput=[&](const std::array<float,4>& pixel) {
        std::fill(source.begin(),source.end(),pixel);
        context->UpdateSubresource(input.Get(),0,nullptr,source.data(),size*16,0);
    };
    auto linear=[](double c) { return c<=0.04045 ? c/12.92 : std::pow((c+0.055)/1.055,2.4); };
    auto encoded=[](double c) { return c<=0.0031308 ? 12.92*c : 1.055*std::pow(c,1.0/2.4)-0.055; };
    auto oracle=[&](const std::array<float,4>& pixel,float strength,const std::array<float,3>& phosphor) {
        if(strength==0) { return pixel; }
        if(pixel[3]<=0) { return std::array<float,4>{0,0,0,pixel[3]}; }
        std::array<double,3> original{}, chosen{};
        const std::array<double,3> weights{0.2126,0.7152,0.0722};
        double luminance=0,chosenLuminance=0;
        for(UINT c=0;c<3;++c) {
            original[c]=linear(std::clamp(static_cast<double>(pixel[c])/static_cast<double>(pixel[3]),0.0,1.0));
            chosen[c]=linear(phosphor[c]); luminance+=original[c]*weights[c];chosenLuminance+=chosen[c]*weights[c];
        }
        std::array<float,4> expected{0,0,0,pixel[3]};
        for(UINT c=0;c<3;++c) {
            const auto mapped=(chosenLuminance>0 ? chosen[c] : 1.0)*luminance;
            expected[c]=static_cast<float>(encoded(std::clamp(original[c]+(std::clamp(mapped,0.0,1.0)-original[c])*strength,0.0,1.0))*pixel[3]);
        }
        return expected;
    };
    auto sample=[&](const auto& result,UINT row) {
        std::array<float,4> value{};
        for(UINT c=0;c<4;++c) { value[c]=result[2+size*2+row*4+c]; }
        return value;
    };
    unsigned int tintCases=0;
    for(const auto rgb:std::array<std::array<float,3>,10>{{{0,0,0},{1,1,1},{0.02f,0.02f,0.02f},{0.25f,0.25f,0.25f},{0.9f,0.9f,0.9f},{1,0,0},{0,1,0},{0,0,1},{1,1,0},{0.1f,0.2f,0.6f}}}) {
        const std::array<std::array<float,3>,10> phosphors{{{0,1,65.0f/255},{1,176.0f/255,0},{1,0,0},{0,0,1},{180.0f/255,80.0f/255,1},{5.0f/255,31.0f/255,8.0f/255},{230.0f/255,1,242.0f/255},{0.1f,0.1f,0.1f},{0,0,0},{1,1,1}}};
        for(const auto phosphor:phosphors) {
            for(const auto alpha:std::array<float,5>{0,0.00000001f,0.000001f,0.4f,1}) {
                const std::array<float,4> pixel{rgb[0]*alpha,rgb[1]*alpha,rgb[2]*alpha,alpha};
                setInput(pixel);
                for(const auto strength:std::array<float,4>{0,0.25f,0.5f,1}) {
                    const auto actual=sample(render(0,0,1,1,1,strength,phosphor),16);
                    const auto expected=oracle(pixel,strength,phosphor);
                    const auto backdrop=Microsoft::Terminal::Backslash::MapPhosphorRgb(rgb,phosphor,strength);
                    for (UINT c=0;c<3;++c) if (std::abs(backdrop[c]*alpha-expected[c])>.00001f) throw std::runtime_error("Backdrop transfer disagrees with independent double-precision oracle");
                    for(UINT c=0;c<4;++c) {
                        if(!std::isfinite(actual[c]) || std::abs(actual[c]-expected[c])>0.00001f || (strength==0 && actual[c]!=pixel[c]) || (c==3 && actual[c]!=pixel[c])) { throw std::runtime_error("Tint pixel/oracle, exact zero identity, or alpha mismatch"); }
                    }
                    if(alpha>=0.000001f) {
                        double before=0,after=0;
                        for(UINT c=0;c<3;++c) { const auto weight=std::array<double,3>{0.2126,0.7152,0.0722}[c];before+=linear(rgb[c])*weight;after+=linear(actual[c]/alpha)*weight; }
                        double endpointLuminance=0;
                        for(UINT c=0;c<3;++c) { endpointLuminance+=linear(phosphor[c])*std::array<double,3>{0.2126,0.7152,0.0722}[c]; }
                        if(endpointLuminance==0) { endpointLuminance=1; }
                        const auto scale=1-strength+strength*endpointLuminance;
                        if(std::abs(after-before*scale)>0.00002) { throw std::runtime_error("Tint failed uniform relative-luminance scaling"); }
                    }
                    ++tintCases;
                }
            }
        }
    }
    // Independent endpoint and monotonicity requirements: no colored phosphor
    // may exempt white, even at nearly transparent coverage. Black is the only
    // fallback, and intentionally produces grayscale instead of a blank image.
    const std::array<std::array<float,3>,10> endpointPhosphors{{{0,1,65.0f/255},{1,176.0f/255,0},{1,0,0},{0,0,1},{180.0f/255,80.0f/255,1},{5.0f/255,31.0f/255,8.0f/255},{230.0f/255,1,242.0f/255},{0.1f,0.1f,0.1f},{0,0,0},{1,1,1}}};
    for(const auto phosphor:endpointPhosphors) {
        const bool black=phosphor==std::array<float,3>{0,0,0};
        for(const auto alpha:std::array<float,3>{1,0.4f,0.00000001f}) {
            setInput({alpha,alpha,alpha,alpha});
            const auto actual=sample(render(0,0,1,1,1,1,phosphor),16);
            for(UINT c=0;c<3;++c) {
                const auto endpoint=black ? 1.0f : phosphor[c];
                if(std::abs(actual[c]/alpha-endpoint)>0.00001f || actual[3]!=alpha) { throw std::runtime_error("White must map to selected phosphor independently of coverage alpha"); }
            }
        }
        double previous=-1;
        for(unsigned int level=0;level<=16;++level) {
            const auto gray=static_cast<float>(encoded(level/16.0));
            setInput({gray,gray,gray,1});
            const auto actual=sample(render(0,0,1,1,1,1,phosphor),16);
            const auto luminance=linear(actual[0])*.2126+linear(actual[1])*.7152+linear(actual[2])*.0722;
            if(luminance+0.000001<previous) { throw std::runtime_error("Phosphor ramp must preserve brightness ordering"); }
            previous=luminance;
        }
    }
    std::puts("PASS: 10 phosphor white endpoints at opaque/partial/near-zero coverage and 170 monotonic gray levels");
    // Zero strength also returns otherwise invalid hidden RGB at alpha zero as-is.
    setInput({0.25f,0.1f,0.5f,0});
    const auto zeroHidden=sample(render(0,0),16);
    if(zeroHidden!=std::array<float,4>{0.25f,0.1f,0.5f,0}) { throw std::runtime_error("Zero tint must not clamp hidden RGB"); }
    setInput({0.1f,0.05f,0.2f,0.4f});
    for(const auto dpi:std::array<float,3>{1,1.25f,2}) {
        for(const auto glowStrength:std::array<float,3>{0,0.3f,1}) {
            for(const auto scanStrength:std::array<float,3>{0,0.5f,1}) {
                const auto before=render(glowStrength,scanStrength,0.5f,3,dpi);
                const auto after=render(glowStrength,scanStrength,0.5f,3,dpi,0.65f);
                for(UINT row=0;row<size;++row) {
                    const auto original=sample(before,row),actual=sample(after,row);
                    const auto expected=oracle(original,0.65f,{0,1,65.0f/255});
                    for(UINT c=0;c<4;++c) { if(!std::isfinite(actual[c]) || std::abs(actual[c]-expected[c])>0.00002f || (c==3 && actual[c]!=original[c])) { throw std::runtime_error("Tint with glow/filtered scanlines altered alpha or RGB mapping"); } }
                }
            }
        }
    }
    std::printf("PASS: %u tint cases: exact zero, RGB oracle, relative-luminance scaling, opaque/partial/near-zero alpha, colored-white endpoints and combined CRT\n",tintCases);
    unsigned int whiteCases=0;
    for(const auto pixel:std::array<std::array<float,4>,5>{{{0,0,0,1},{0.15f,0.6f,0.3f,1},{0.9f,0.9f,0.9f,1},{0.1f,0.02f,0.2f,0.4f},{0,0,0,0}}}) {
        setInput(pixel);
        for(const auto dpi:std::array<float,5>{0.75f,1,1.25f,1.5f,2}) {
            for(const auto dimensions:std::array<std::array<float,2>,4>{{{1,1},{0.5f,3},{4,4},{8,0}}}) {
                for(const auto strength:std::array<float,4>{0,0.1f,0.5f,1}) {
                    for(const auto tint:std::array<float,2>{0,0.8f}) {
                        const std::array<float,3> phosphor{1,0,1};
                        const auto base=sample(render(0,0,dimensions[0],dimensions[1],dpi,tint,phosphor),16);
                        const auto actual=render(0,strength,dimensions[0],dimensions[1],dpi,tint,phosphor,1);
                        for(UINT y=0;y<size;++y) {
                            const auto mask=static_cast<float>(coverage(y+0.5,dimensions[0],dimensions[1],dpi))*strength;
                            const auto out=sample(actual,y);
                            for(UINT c=0;c<4;++c) {
                                const auto expected=c==3 ? pixel[3] : base[c]+(pixel[3]-base[c])*mask;
                                if(std::abs(out[c]-expected)>0.00002f || (c==3 && out[c]!=pixel[3]) || (strength==0 && out[c]!=base[c])) { throw std::runtime_error("White scanline band/gap, zero, tint ordering or alpha mismatch"); }
                            }
                        }
                        ++whiteCases;
                    }
                }
            }
        }
    }
    // Scheme mode must share exactly the same ramp with its solid backdrop.
    unsigned schemeCases = 0;
    for (const auto pixel : std::array<std::array<float,4>,7>{{{0,0,0,1},{.267f,.267f,.267f,1},{.8f,.8f,.8f,1},{1,0,0,1},{0,.58f,0,1},{.1f,.02f,.2f,.4f},{0,0,0,0}}}) {
        setInput(pixel);
        for (float strength : {0.0f,.5f,1.0f}) {
            const std::array<float,3> orange{1,136.0f/255,0};
            for (float lines : {0.0f,.1f,1.0f}) {
                const auto result = render(0,lines,1,1,1,strength,orange,1,1);
                for (UINT y=0;y<size;++y) {
                    auto expectedSource=pixel;
                    const auto mask=static_cast<float>(coverage(y+.5,1,1,1))*lines;
                    // At zero tint scheme mode leaves the legacy white-band behavior.
                    if (strength>0) for (UINT c=0;c<3;++c) expectedSource[c]+=(pixel[3]-pixel[c])*mask;
                    auto expected=oracle(expectedSource,strength,orange);
                    if (strength==0) for (UINT c=0;c<3;++c) expected[c]+=(pixel[3]-expected[c])*mask;
                    const auto actual=sample(result,y);
                    for (UINT c=0;c<4;++c) if (std::abs(actual[c]-expected[c])>.00002f) throw std::runtime_error("Scheme bands/ramp/alpha oracle mismatch");
                }
                ++schemeCases;
            }
            if (pixel[3]>0) {
                const auto cpu=Microsoft::Terminal::Backslash::MapPhosphorRgb({pixel[0]/pixel[3],pixel[1]/pixel[3],pixel[2]/pixel[3]},orange,strength);
                const auto gpu=sample(render(0,0,1,1,1,strength,orange,0,1),16);
                for (UINT c=0;c<3;++c) if (std::abs(cpu[c]*pixel[3]-gpu[c])>.00002f) throw std::runtime_error("Solid backdrop/production shader mismatch");
            }
        }
    }
    std::printf("PASS: %u scheme phosphor/white-band/alpha cases and CPU backdrop equivalence\n",schemeCases);
    const std::array<float,3> collisionOrange{1,136.0f/255,0};
    const auto greenAtRedY=static_cast<float>(encoded(.2126/.7152));
    const auto redMapped=Microsoft::Terminal::Backslash::MapPhosphorRgb({1,0,0},collisionOrange,1);
    const auto greenMapped=Microsoft::Terminal::Backslash::MapPhosphorRgb({0,greenAtRedY,0},collisionOrange,1);
    for (UINT c=0;c<3;++c) Near(redMapped[c],greenMapped[c],"Equal-luminance hue collision remains explicit in scheme mapping");
    unsigned schemeSweepCases=0;
    for (float alpha : {0.0f,.01f,.5f,.99f,1.0f}) {
        const std::array<float,4> pixel{.8f*alpha,.8f*alpha,.8f*alpha,alpha};
        setInput(pixel);
        for(float glowStrength : {0.0f,.15f,.25f,.5f,.75f,.9f,1.0f})
        for(float intensity : {0.0f,.25f,.5f,.75f,.9f,1.0f})
        for(float whiteChoice : {0.0f,1.0f})
        for(float tintStrength : {0.0f,.5f,1.0f})
        for(float dpi : {1.0f,1.25f,2.0f})
        for(auto geometry : std::array<std::array<float,2>,3>{{{4,4},{.5f,3},{8,0}}}) {
            const std::array<float,3> orange{1,136.0f/255,0};
            const auto actual=render(glowStrength,intensity,geometry[0],geometry[1],dpi,tintStrength,orange,whiteChoice,1);
            // Interior samples exclude the intentional transparent border sampler.
            for(UINT y=8;y<size-8;++y) {
                auto processed=pixel;
                // For a uniform source the normalized finite blur is exactly source.
                for(UINT c=0;c<4;++c) processed[c]+=pixel[c]*glowStrength-processed[c]*pixel[c]*glowStrength;
                const auto mask=static_cast<float>(coverage(y+.5,geometry[0],geometry[1],dpi))*intensity;
                if(whiteChoice>0) for(UINT c=0;c<3;++c) processed[c]+=(processed[3]-processed[c])*mask;
                else for(UINT c=0;c<3;++c) processed[c]*=1-mask;
                const auto expected=oracle(processed,tintStrength,orange);
                const auto observed=sample(actual,y);
                for(UINT c=0;c<4;++c) {
                    if(!std::isfinite(observed[c]) || std::abs(observed[c]-expected[c])>.00003f || observed[c]<-.000001f || observed[c]>observed[3]+.00001f) throw std::runtime_error("Scheme normalized bloom/screen/scanline intensity sweep mismatch");
                }
            }
            ++schemeSweepCases;
        }
    }
    std::printf("PASS: %u scheme rendered sweeps at 0/25/50/75/90/100 percent, opacity0/1/50/99/100, glow, geometry and DPI\n",schemeSweepCases);
    setInput({0,0,0,1});
    const auto whiteBlack=render(0,0.1f,1,1,1,0,{1,0,1},1);
    Near(whiteBlack[0],0,"White scanline leaves true-black gap unchanged");
    Near(whiteBlack[1],0.1f,"White scanline is visible on empty true-black background at default intensity");
    const auto blackBlack=render(0,0.1f,1,1);
    Near(blackBlack[0],0,"Black scanline true-black gap unchanged");
    Near(blackBlack[1],0,"Black scanline true-black band remains black");
    const auto fullWhite=render(0,1,1,1,1,1,{1,0,1},1);
    const auto clear=sample(fullWhite,16),band=sample(fullWhite,17);
    for(UINT c=0;c<3;++c){Near(clear[c],0,"Phosphor black gap unchanged by white choice");Near(band[c],1,"Full white reaches white only in bands after phosphor");}
    std::printf("PASS: %u white scanline cases: black/colored backgrounds, glyph tones, zero/full/subtle intensity, DPI, gap, tint and premultiplied alpha\n",whiteCases);
    if(argc==4) {
        // Diagnostic text-like strokes on true black and magenta cell backgrounds.
        // These captures come from actual shader readback, composited on black.
        const unsigned int letters[3][5]{{6,5,5,5,6},{7,2,2,2,7},{6,5,6,5,5}};
        for(UINT y=0;y<size;++y)for(UINT x=0;x<size;++x)source[y*size+x]=y<16?std::array<float,4>{0,0,0,1}:std::array<float,4>{0.5f,0,0.5f,1};
        for(UINT sampleBand=0;sampleBand<2;++sampleBand)for(UINT letter=0;letter<3;++letter)for(UINT y=0;y<5;++y)for(UINT x=0;x<3;++x)if(letters[letter][y]&(1u<<(2-x)))for(UINT sy=0;sy<2;++sy)for(UINT sx=0;sx<2;++sx)source[(sampleBand*16+2+y*2+sy)*size+2+letter*9+x*2+sx]={0.9f,0.9f,0.9f,1};
        context->UpdateSubresource(input.Get(),0,nullptr,source.data(),size*16,0);
        const auto directory=std::filesystem::path{argv[3]};
        capture=directory/L"scanline-neutral.bmp";render(0,0,4,4);
        capture=directory/L"scanline-black-10.bmp";render(0,0.1f,4,4);
        capture=directory/L"scanline-white-10.bmp";render(0,0.1f,4,4,1,0,{1,0,1},1);
        capture=directory/L"scanline-white-50.bmp";render(0,0.5f,4,4,1,0,{1,0,1},1);
        capture=directory/L"scanline-white-10-phosphor.bmp";render(0,0.1f,4,4,1,0.8f,{1,0,1},1);
        capture.clear();
        std::puts("PASS: actual GPU captures of synthetic foreground strokes, black and magenta backgrounds written");
    }
    auto* timeVar=reflection->GetConstantBufferByName("PixelShaderSettings")->GetVariableByName("time");
    D3D11_SHADER_VARIABLE_DESC timeDesc{}; Check(timeVar->GetDesc(&timeDesc));
    if(timeDesc.uFlags&D3D_SVF_USED) { throw std::runtime_error("Static scanlines must not use Time"); }
    std::puts("PASS: Static scanline shader does not use Time");
    std::puts("WARP retro shader tests passed");
    return 0;
}
catch (const std::exception& e) { std::fprintf(stderr,"FAIL: %s\n",e.what()); return 1; }
