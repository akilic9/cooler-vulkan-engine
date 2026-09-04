struct UniformBufferObject
{
    float4x4 view;
    float4x4 projection;
};

struct CVEGameObjectPushConstant
{
    float4x4 modelMatrix;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> sceneUbo : register(b0, space0);

[[vk::push_constant]] CVEGameObjectPushConstant pushConsts;

struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION0;
    [[vk::location(1)]] float4 colour : COLOR0;
    [[vk::location(2)]] float3 normal : NORMAL0;
    [[vk::location(3)]] float2 texCoord0 : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    [[vk::location(0)]] float4 colour : COLOR0;
    [[vk::location(1)]] float2 texCoord0 : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.colour = input.colour;
    output.texCoord0 = input.texCoord0;
    output.position = mul(sceneUbo.projection, mul(sceneUbo.view, mul(pushConsts.modelMatrix, float4(input.position, 1.0))));
    return output;
}