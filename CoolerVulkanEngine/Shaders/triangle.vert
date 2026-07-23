struct UniformBufferObject
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> ubo : register(b0, space0);

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION0;
    [[vk::location(1)]] float3 colour : COLOR0;
    [[vk::location(2)]] float3 normal : NORMAL0;
    [[vk::location(3)]] float2 texCoord0 : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    [[vk::location(0)]] float3 colour : COLOR0;
    [[vk::location(1)]] float2 texCoord0 : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.colour = input.colour;
    output.texCoord0 = input.texCoord0;
    output.position = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.position, 1.0))));
    return output;
}