cbuffer cbPerObject : register(b0) 
{
    float4x4 mmodel;
    uint colorIndex;
};

cbuffer cbPerPass : register(b1)
{
    float4x4 mview;
    float4x4 mproj;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

PSInput VSMain(float3 position : POSITION)
{
    // default color sequence of the cube for each face in the direction
    float3 colorSamples[7] =
    {
        float3(1.0, 0.215, 0.115), // Front              
        float3(0.0, 1.0, .498), // Right
        float3(1.0, 1.0, 0.3), // Top
        float3(0.313, 0.325, .98), // Left
        float3(1.0, 0.481, 0.12), // Back
        float3(1.0, 1.0, 1.0), // Bottom
        float3(0.0, 0.0, 0.0) // Base
    };
    
    
    float4x4 wvp = mul(mul(mmodel, mview), mproj);

    PSInput psinput;
    psinput.position = mul(float4(position, 1.0), wvp);
    psinput.color = colorSamples[colorIndex];
    
    
    return psinput;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return float4(input.color, 1.0);
}