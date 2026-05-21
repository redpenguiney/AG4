#version 460 // TODO: version too high
// #extension GL_ARB_shading_language_include : require

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec4 color;
layout(location=2) in vec2 textureXY;
layout(location=3) in float autoTextureZ;
layout(location=4) in vec3 vertexNormal;
layout(location=5) in vec3 vertexTangent;

layout(location=6) in mat4 modelMatrix;
// locations 6-9 are part of model

layout(location=10) in mat3 normalMatrix;
// locations 10-12 are part of normalMatrix

// 13 and 14 are arbitrary1 and 2
layout(location = 13) in ivec4 boneIDs;
layout(location = 14) in vec4 boneWeights;

// perspective has projection matrix and camera matrix
uniform mat4 perspective;
uniform mat4 modelToLightSpace;

layout(std140, binding = 0) readonly buffer boneSsbo {
    mat4 finalBonesMatrices[];
};
//layout(std140, binding = 3) readonly buffer boneOffsetSsbo {
//    uint boneBufferOffsets[];
//};

uniform bool normalMappingEnabled;
uniform uint maxBones;
uniform int boneOffsetModifier; // TODO: JUST USE MULTIPLE BUFFERING FOR BONES

out vec4 fragmentColor;
out vec3 cameraToFragmentPosition;
out vec3 cameraToFragmentInTangentSpace;
out vec3 fragmentNormal;
out vec3 fragmentTexCoords;
out mat3 TBNmatrix; //TBN matrix is need to make normal mapping work when an object is rotated
// out vec4 lightSpaceCoords;

void main()
{
    uint offset = (gl_BaseInstance + gl_InstanceID) * maxBones;
    ivec4 realBoneIds = boneIDs;
    vec4 totalPosition = vec4(0.0f);
    for(int i = 0 ; i < 4 ; i++)
    {
        if(realBoneIds[i] == -1) { // todo: we could just remove this and rely on excess bones having weight 0? still need to handle i == 0 case somehow tho 
            if (i == 0) {
                totalPosition = vec4(vertexPos,1.0f);
            }
            break; 
        }
        //if(realBoneIds[i] + offset >= finalBonesMatrices.length()) {
        //    totalPosition = vec4(vertexPos,1.0f);
        //    break;
        //}
        vec4 localPosition = finalBonesMatrices[realBoneIds[i] + offset] * vec4(vertexPos,1.0f);
        //mat4 m;
        //m[1] = vec4(1);
        //m[2] = vec4(3);
        //vec4 localPosition = m * vec4(vertexPos, 1.0f);
        totalPosition += localPosition * boneWeights[i];
        // vec3 localNormal = mat3(finalBonesMatrices[realBoneIds[i] + offset]) * norm;
        //totalPosition = vec4(vertexPos, 1.0f);
        
    }
    //totalPosition = vec4(vertexPos, 1.0f);
    vec4 p = modelMatrix * totalPosition;
    cameraToFragmentPosition = p.xyz;
    gl_Position = perspective * p;

    fragmentColor = color;
    fragmentNormal = normalize(normalMatrix * vertexNormal);
    fragmentTexCoords = vec3(textureXY, autoTextureZ);

    vec3 T = normalize(vec3(modelMatrix * vec4(vertexTangent,   0.0)));
    vec3 B = cross(fragmentNormal, T);
    TBNmatrix = mat3(T, B, fragmentNormal);
    cameraToFragmentInTangentSpace = TBNmatrix * (cameraToFragmentPosition);
}          

