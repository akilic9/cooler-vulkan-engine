struct PSInput
{
    float3 colour : COLOR0;
};

float4 main(PSInput input) : SV_Target
{
     return float4(input.colour, 1.0);
}