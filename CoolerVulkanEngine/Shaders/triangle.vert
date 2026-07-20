struct UniformBufferObject
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

cbuffer ubo : register(b0, space0)
{
    UniformBufferObject ubo;
}

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION0;
    [[vk::location(1)]] float3 colour : COLOR0;
    [[vk::location(2)]] float3 normal : NORMAL0;
    [[vk::location(3)]] float3 texCoord0 : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    [[vk::location(0)]] float3 colour : COLOR0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.colour = input.colour;
    output.position = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.position, 1.0))));
    return output;
}