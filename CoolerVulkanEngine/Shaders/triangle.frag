[[vk::binding(1, 0)]]
Texture2D texture : register(t1, space0);

[[vk::binding(1, 0)]]
SamplerState textureSampler : register(s1, space0);

struct PSInput
{
    [[vk::location(0)]] float3 colour : COLOR0;
    [[vk::location(1)]] float2 texCoord0 : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target
{
     return texture.Sample(textureSampler, input.texCoord0);
}