struct LightColor{    float4      diffuse;    float4      ambient;    float4      specular;};struct LightInfo{    LightColor  color;    float4	    position;    float4	    direction;     int		    lightType;    float	    range;    float	    angle;    int  	    padding;};cbuffer GLOBAL_PARAMS : register(b0){    int         g_lightCount;    float3      g_lightPadding;    LightInfo   g_light[50];}cbuffer TRANSFORM_PARAMS : register(b1){    row_major matrix g_matWorld;    row_major matrix g_matView;    row_major matrix g_matProjection;    row_major matrix g_matWV;    row_major matrix g_matWVP;};cbuffer MATERIAL_PARAMS : register(b2){    int int_0;    int int_1;    int int_2;    int int_3;    int int_4;    float float_0;    float float_1;    float float_2;    float float_3;    float float_4;};Texture2D tex_0 : register(t0);Texture2D tex_1 : register(t1);Texture2D tex_2 : register(t2);Texture2D tex_3 : register(t3);Texture2D tex_4 : register(t4);SamplerState sam_0 : register(s0);struct VS_IN{    float3 pos : POSITION;    float2 uv : TEXCOORD;};struct VS_OUT{    float4 pos : SV_Position;    float2 uv : TEXCOORD;};VS_OUT VS_Main(VS_IN input){    VS_OUT output = (VS_OUT)0;    output.pos = mul(float4(input.pos, 1.f), g_matWVP);    output.uv = input.uv;    return output;}float4 PS_Main(VS_OUT input) : SV_Target{    float4 color = tex_0.Sample(sam_0, input.uv);    return color;}