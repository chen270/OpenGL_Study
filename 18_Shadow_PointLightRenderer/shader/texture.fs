# version 330 core

// uniform vec4 U_AmbientLightColor;
// uniform vec4 U_AmbientMaterial;//材质
uniform sampler2D U_MainTexture;//纹理填充
in vec2 V_Texcoord;
in vec3 V_Normal;
in vec4 V_WorldPos;

void main()
{
    vec3 lightPos = vec3(10.0, 10.0, 0.0);
    vec3 L = lightPos;
    L = normalize(L);
    vec3 n = normalize(V_Normal);

    //amibent 
    vec4 AmbientLightColor = vec4(0.2, 0.2, 0.2, 1.0);
    vec4 AmbientMaterial   = vec4(0.2, 0.2, 0.2, 1.0);//环境光反射的材质
    vec4 ambientColor = AmbientLightColor * AmbientMaterial;//环境光

    //DIFFUSE
    vec4 DiffuseLightColor = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 DiffuseMaterial   = vec4(0.8, 0.8, 0.8, 1.0);//漫反射光 反射的材质
    vec4 diffuseColor = DiffuseLightColor * DiffuseMaterial * max(0.0,dot(L, n));//求得L n 之间的夹角

    //SPECULAR
    vec3 reflectDir = normalize(reflect(-L, n));//光线入射，法线为基准
    //求进入眼睛的光线
    vec3 viewDir = normalize(vec3(0.0) - V_WorldPos.xyz);

    //求得夹角
    vec4 SpecularLightColor = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 SpecularMaterial = vec4(0.8, 0.8, 0.8, 1.0);//镜面反射光 反射的材质
    vec4 specularColor = SpecularLightColor * SpecularMaterial * pow(max(0.0, dot(reflectDir, viewDir)), 128.0);


    if (diffuseColor.r == 0.0) 
    {
        //不在暗面 画光斑
        gl_FragColor = ambientColor + texture2D(U_MainTexture, V_Texcoord) * (diffuseColor);// +specularColor;
    }
    else {
        gl_FragColor = ambientColor + texture2D(U_MainTexture, V_Texcoord) * (diffuseColor);// +specularColor;
    }
    
}