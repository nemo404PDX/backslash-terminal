// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

// The original retro pixel shader
Texture2D shaderTexture;
SamplerState samplerState;

cbuffer PixelShaderSettings : register(b0)
{
    float time;
    float scale;
    float2 resolution;
    float4 background;
};

cbuffer RetroShaderSettings : register(b1)
{
    float glowIntensity;
    float scanlineIntensity;
    float scanlineThickness;
    float scanlineGap;
    float4 tintColorAndIntensity;
    float4 scanlineColorAndPadding; // x: white bands; y: semantic contrast protection
};

#define SCANLINE_FACTOR scanlineIntensity
#define SCALED_GAUSSIAN_SIGMA (2.0f * scale)

static const float M_PI = 3.14159265f;

float Gaussian2D(float x, float y, float sigma)
{
    return 1 / (sigma * sqrt(2 * M_PI)) * exp(-0.5 * (x * x + y * y) / sigma / sigma);
}

float4 Blur(Texture2D input, float2 tex_coord, float sigma)
{
    float width, height;
    shaderTexture.GetDimensions(width, height);

    float texelWidth = 1.0f / width;
    float texelHeight = 1.0f / height;

    float4 color = { 0, 0, 0, 0 };

    float sampleCount = 13;

    for (float x = 0; x < sampleCount; x++)
    {
        float2 samplePos = { 0, 0 };
        samplePos.x = tex_coord.x + (x - sampleCount / 2.0f) * texelWidth;

        for (float y = 0; y < sampleCount; y++)
        {
            samplePos.y = tex_coord.y + (y - sampleCount / 2.0f) * texelHeight;
            color += input.Sample(samplerState, samplePos) * Gaussian2D(x - sampleCount / 2.0f, y - sampleCount / 2.0f, sigma);
        }
    }

    return color;
}

// Integrate the periodic dark mask over a physical pixel. This preserves the
// bright-first legacy phase at integer DPI and filters fractional/subpixel bands.
float DarkBandIntegral(float x, float period, float gap, float thickness)
{
    float cycles = floor(x / period);
    return cycles * thickness + clamp(x - cycles * period - gap, 0.0f, thickness);
}

float SquareWave(float y)
{
    float thickness = scanlineThickness * scale;
    float gap = scanlineGap * scale;
    float period = thickness + gap;
    // Work in one local period to avoid subtracting large accumulated integrals.
    float start = fmod(y - 0.5f, period);
    float coverage = saturate(DarkBandIntegral(start + 1.0f, period, gap, thickness) - DarkBandIntegral(start, period, gap, thickness));
    return 1.0f - coverage * SCANLINE_FACTOR;
}

float4 Scanline(float4 color, float4 pos)
{
    float wave = SquareWave(pos.y);

    // TODO:GH#3929 make this configurable.
    // Remove the && false to draw scanlines everywhere.
    if (length(color.rgb) < 0.2f && false)
    {
        return color + wave * 0.1f;
    }
    else
    {
        return color * wave;
    }
}

// Terminal surfaces use encoded sRGB with premultiplied alpha. Convert only
// after CRT processing so tint retains that stage's alpha, including its legacy
// glow/scanline behavior. b0 remains the external custom-shader contract.
float3 SrgbToLinear(float3 value)
{
    return lerp(value / 12.92f, pow(max((value + 0.055f) / 1.055f, 0.0f), 2.4f), step(0.04045f, value));
}

float3 LinearToSrgb(float3 value)
{
    return lerp(value * 12.92f, 1.055f * pow(max(value, 0.0f), 1.0f / 2.4f) - 0.055f, step(0.0031308f, value));
}

float4 PhosphorTint(float4 color)
{
    // Branch at zero in main: exact identity, with no conversion/clipping.
    if (color.a <= 0.0f) { return float4(0, 0, 0, color.a); }
    float3 original = SrgbToLinear(saturate(color.rgb / color.a));
    float3 phosphor = SrgbToLinear(tintColorAndIntensity.rgb);
    const float3 weights = float3(0.2126f, 0.7152f, 0.0722f);
    float luminance = saturate(dot(original, weights));
    float phosphorLuminance = dot(phosphor, weights);
    // Keep the selected hue through the brightest samples: white maps to the
    // phosphor endpoint, including unpremultiplied transparent glyph coverage.
    // This uniformly scales linear luminance instead of adding white highlights.
    // Dark/saturated choices can dim the image. Exact black uses grayscale so
    // selecting it cannot blank all terminal content.
    float3 endpoint = phosphorLuminance > 0.0f ? phosphor : 1.0f;
    float3 tinted = endpoint * luminance;
    float3 blended = lerp(original, saturate(tinted), tintColorAndIntensity.a);
    return float4(LinearToSrgb(saturate(blended)) * color.a, color.a);
}

// clang-format off
float4 main(float4 pos : SV_POSITION, float2 tex : TEXCOORD) : SV_TARGET
// clang-format on
{
    float4 color = shaderTexture.Sample(samplerState, tex);
    [branch] if (glowIntensity > 0.0f)
    {
        float4 bloom = Blur(shaderTexture, tex, SCALED_GAUSSIAN_SIGMA) * glowIntensity;
        [branch] if (scanlineColorAndPadding.y > 0.5f && tintColorAndIntensity.a > 0)
        {
            // The legacy additive bloom can produce alpha > 1 and lift dark glyphs
            // into their bright backdrop. Keep physically bounded coverage and
            // limit the added linear light while retaining the blurred halo.
            float alpha = saturate(color.a + bloom.a * (1 - color.a));
            float3 base = SrgbToLinear(saturate(color.rgb / max(color.a, 1e-8f)));
            float3 lit = SrgbToLinear(saturate((color.rgb + bloom.rgb) / max(alpha, 1e-8f)));
            float3 weights = float3(0.2126f, 0.7152f, 0.0722f);
            float limit = min(1, dot(base, weights) + 0.02f);
            float light = dot(lit, weights);
            lit *= min(1, limit / max(light, 1e-8f));
            color = float4(LinearToSrgb(lit) * alpha, alpha);
        }
        else
        {
            color += bloom;
        }
    }
    [branch] if (scanlineIntensity > 0.0f && scanlineColorAndPadding.x < 0.5f)
    {
        color = Scanline(color, pos);
    }

    [branch] if (tintColorAndIntensity.a > 0.0f)
    {
        color = PhosphorTint(color);
    }

    // Light bands follow tint so they remain white without recoloring the gaps.
    // Preserve coverage alpha: a transparent terminal hole stays transparent.
    [branch] if (scanlineIntensity > 0.0f && scanlineColorAndPadding.x >= 0.5f)
    {
        color.rgb = lerp(color.rgb, color.aaa, 1.0f - SquareWave(pos.y));
    }
    return color;
}
