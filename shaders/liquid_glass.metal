//
//  liquid_glass.metal
//  Reconstructed from libloader (__TEXT,__cstring)
//
//  Custom Frosted Glass / Refractive Backdrop Shader for Mod Menu UI
//

#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

struct LiquidGlassUniforms {
    float2 glassCenter;
    float2 glassSize;
    float2 resolution;
    float cornerRadius;
    float blurRadius;
    float refractStrength;
    float alpha;
    float4 tint;
};

vertex VertexOut liquidGlassVertex(uint vertexID [[vertex_id]],
                                    constant float4* vertices [[buffer(0)]]) {
    VertexOut out;
    float4 vert = vertices[vertexID];
    out.position = float4(vert.xy, 0.0, 1.0);
    out.uv = vert.zw;
    return out;
}

// Signed distance field calculation for rounded rectangle
float roundedBoxSDF(float2 p, float2 size, float radius) {
    float2 d = abs(p) - size + float2(radius);
    return length(max(d, 0.0)) - radius;
}

float roundedBox(float2 uv, float2 center, float2 size, float radius) {
    float2 q = abs(uv - center) - size + radius;
    return length(max(q, 0.0)) - radius;
}

// Computes surface normal along the boundary using gradient of SDF
float2 getNormal(float2 uv, float2 center, float2 size, float radius, float2 resolution) {
    float2 eps = float2(1.0) / resolution * 2.0;
    float2 p = uv - center;

    float dx = (roundedBoxSDF(p + float2(eps.x, 0.0), size, radius) -
                roundedBoxSDF(p - float2(eps.x, 0.0), size, radius)) * 0.5;
    float dy = (roundedBoxSDF(p + float2(0.0, eps.y), size, radius) -
                roundedBoxSDF(p - float2(0.0, eps.y), size, radius)) * 0.5;

    float2 gradient = float2(dx, dy);

    float dxy1 = roundedBoxSDF(p + eps, size, radius);
    float dxy2 = roundedBoxSDF(p - eps, size, radius);
    float2 diag = float2(dxy1 - dxy2);

    gradient = mix(gradient, diag, 0.25);

    if (length(gradient) < 0.001) {
        return float2(0.0);
    }
    return normalize(gradient);
}

// Main fragment shader for refractive frosted glass rendering
fragment float4 liquidGlassFragment(VertexOut in [[stage_in]],
                                     texture2d<float> backgroundTexture [[texture(0)]],
                                     constant LiquidGlassUniforms& uniforms [[buffer(0)]]) {
    constexpr sampler s(filter::linear, address::clamp_to_edge);

    float2 uv = in.uv;
    float2 pixelUV = uv * uniforms.resolution;
    float2 center = uniforms.glassCenter;
    float2 size = uniforms.glassSize * 0.5;

    float2 local = (pixelUV - center) / size;
    local.y *= uniforms.resolution.x / uniforms.resolution.y;

    float radius = uniforms.cornerRadius;
    float dist = roundedBox(pixelUV, center, size, radius);

    if (dist > 1.0) {
        discard_fragment();
    }

    float r = clamp(length(local), 0.0, 1.0);
    float curvature = pow(r, 1.0);
    float2 domeNormal = normalize(local) * curvature;
    float eta = 1.0 / 1.5; // Refractive index ratio (glass to air)
    float2 incident = -domeNormal;
    float2 refractVec = refract(incident, domeNormal, eta);
    float2 curvedRefractUV = uv + refractVec * 0.03;

    float contourFalloff = exp(-abs(dist) * 0.4);
    float2 normal = getNormal(pixelUV, center, size, radius, uniforms.resolution);
    float2 domeNormalContour = normal * pow(contourFalloff, 1.5);
    float2 refractVecContour = refract(float2(0.0), domeNormalContour, eta);
    float2 uvContour = uv + refractVecContour * 0.35 * contourFalloff;

    float edgeWeight = smoothstep(0.0, 1.0, abs(dist));
    float radialWeight = smoothstep(0.5, 1.0, r);
    float combinedWeight = clamp((edgeWeight * 1.0) + (-radialWeight * 0.5), 0.0, 1.0);
    float2 refractUV = mix(curvedRefractUV, uvContour, combinedWeight);

    float3 base = backgroundTexture.sample(s, refractUV).rgb;

    // Apply color absorption / tinting
    float tintStrength = max(uniforms.tint.w, 0.0);
    if (tintStrength > 0.001) {
        float3 tintColor = clamp(uniforms.tint.rgb, 0.001, 1.0);
        float thickness = 0.3 + 0.7 * (1.0 - r);
        float3 absorption = -log(tintColor);
        base *= exp(-absorption * thickness * tintStrength);
    }

    // Edge shadow
    float edgeFalloff = smoothstep(0.01, 0.0, dist);
    float verticalBand = 1.0 - smoothstep(-1.5, -0.2, local.y);
    float topShadow = edgeFalloff * verticalBand;
    float3 shadowColor = float3(0.0);
    base = mix(base, shadowColor, topShadow * 0.1);

    // Rim glow
    float edge = 1.0 - smoothstep(0.0, 0.03, dist * -2.0);
    float3 glow = float3(0.7);
    float3 color = mix(base, glow, edge * 0.5);

    return float4(color, 0.75 * uniforms.alpha);
}

// Separable Gaussian blur compute kernels
kernel void blurHorizontal(texture2d<float, access::read> inTex [[texture(0)]],
                           texture2d<float, access::write> outTex [[texture(1)]],
                           constant float& radius [[buffer(0)]],
                           uint2 gid [[thread_position_in_grid]]) {
    uint w = outTex.get_width();
    uint h = outTex.get_height();
    if (gid.x >= w || gid.y >= h) return;

    float sx = float(inTex.get_width()) / float(w);
    float sy = float(inTex.get_height()) / float(h);
    int cx = int(float(gid.x) * sx);
    int cy = int(float(gid.y) * sy);
    int iw = int(inTex.get_width());

    float weights[9] = { 0.028, 0.066, 0.124, 0.179, 0.206, 0.179, 0.124, 0.066, 0.028 };
    int step = max(1, int(radius * 0.15));

    float4 color = float4(0.0);
    for (int i = -4; i <= 4; i++) {
        int px = clamp(cx + i * step, 0, iw - 1);
        color += inTex.read(uint2(px, cy)) * weights[i + 4];
    }
    outTex.write(color, gid);
}

kernel void blurVertical(texture2d<float, access::read> inTex [[texture(0)]],
                         texture2d<float, access::write> outTex [[texture(1)]],
                         constant float& radius [[buffer(0)]],
                         uint2 gid [[thread_position_in_grid]]) {
    uint w = outTex.get_width();
    uint h = outTex.get_height();
    if (gid.x >= w || gid.y >= h) return;

    int ih = int(inTex.get_height());

    float weights[9] = { 0.028, 0.066, 0.124, 0.179, 0.206, 0.179, 0.124, 0.066, 0.028 };
    int step = max(1, int(radius * 0.15));

    float4 color = float4(0.0);
    for (int i = -4; i <= 4; i++) {
        int py = clamp(int(gid.y) + i * step, 0, ih - 1);
        color += inTex.read(uint2(gid.x, py)) * weights[i + 4];
    }
    outTex.write(color, gid);
}
