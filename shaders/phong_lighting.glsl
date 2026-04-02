uniform vec3 envLightDirection;
uniform vec3 envLightColor;
uniform float envLightDiffuse;
uniform float envLightAmbient;
uniform float envLightSpecular;

struct Light {
    vec4 relPosAndIntensity; // w-coord is intensity/range, xyz is pos
    vec4 colorAndAngle; // xyz is color, w-coord is spotlight angle
    vec4 directionAndLightType; // xyz is direction, w-coord is  0.0f (no more lights, stop iterating), 1.0f (pointlight), 2.0f (spotlight), or 3.0f (environmental light)
};

layout (std140) uniform lights {
    Light Lights[1024];
};

vec3 CalculateEnvLightInfluence( float specularStrength, vec3 normal) {
    float diff = max(dot(normal, envLightDirection), 0.0);
    vec3 diffuse = diff * envLightDiffuse * envLightColor;

    vec3 viewDir = normalize(-cameraToFragmentPosition);
    // vec3 reflectDir = reflect(-lightDir, normal); // replace reflectDir with halfwayDir for blinn-phong lighting, which is better than phong lighting
    vec3 halfwayDir = normalize(envLightDirection + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec * envLightSpecular * envLightColor;  

    vec3 ambient = envLightColor * envLightAmbient;

    return ambient + diffuse + specular;
}

vec3 CalculateSpotlightInfluence(vec3 lightColor, vec3 rel_pos, float range, float innerAngle, float outerAngle, vec3 lightDirection, float specularStrength, vec3 normal) {
    float distance = length(rel_pos - cameraToFragmentPosition);
    vec3 lightDir = normalize(rel_pos - cameraToFragmentPosition); 
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(-cameraToFragmentPosition);
    // vec3 reflectDir = reflect(-lightDir, normal); // replace reflectDir with halfwayDir for blinn-phong lighting, which is better than phong lighting
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    float theta = dot(lightDir, lightDirection);
    float spotlightStrength = range/pow(distance, 2) * max(0, (theta - outerAngle)/(innerAngle - outerAngle));
    
    //return vec3(theta, theta, theta);
    return spotlightStrength * (diffuse + specular);
}

vec3 CalculateLightInfluence(vec3 lightColor, vec3 rel_pos, float range, float specularStrength, vec3 normal) {
    
    float distance = length(rel_pos - cameraToFragmentPosition);
    vec3 lightDir = normalize(rel_pos - cameraToFragmentPosition); 
    // float d = ;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(-cameraToFragmentPosition);
    // vec3 reflectDir = reflect(-lightDir, normal); // replace reflectDir with halfwayDir for blinn-phong lighting, which is better than phong lighting
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  

    vec3 ambient = lightColor * 0.1;

    float strength = range/pow(distance, 2);
    
    return strength * (ambient + diffuse + specular);
};

vec3 CalculateLighting(float specularStrength, vec3 normal) {
    vec3 light = vec3(0, 0, 0);
    for (uint i = 0; i < 1024; i++) {
        if (Lights[i].directionAndLightType.w == 0.0f) return light;
        else if (Lights[i].directionAndLightType.w == 1.0f) {
            light += CalculateLightInfluence(Lights[i].colorAndAngle.xyz, Lights[i].relPosAndIntensity.xyz, Lights[i].relPosAndIntensity.w, specularStrength, normal);
        }
    }

    return light;
}