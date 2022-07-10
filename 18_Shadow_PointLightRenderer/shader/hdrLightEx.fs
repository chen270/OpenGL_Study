

uniform vec4 U_LightPos;
uniform vec3 U_EyePos;
uniform vec4 U_LightDirection;
uniform float U_Cutoff;
uniform float U_DiffuseIntensity;
uniform vec4 U_AmbientLightColor;
uniform vec4 U_AmbientMaterial;
uniform vec4 U_DiffuseLightColor;
uniform vec4 U_DiffuseMaterial;
uniform vec4 U_SpecularLightColor;
uniform vec4 U_SpecularMaterial;

varying vec3 V_Normal;
varying vec3 V_WorldPos;

void main()
{
	float radianCutoff=U_Cutoff*3.14/180.0;
	float cosThta=cos(radianCutoff);
	vec3 spotLightDirection=normalize(U_LightDirection.xyz);

	//ambient
	vec4 ambientColor=U_AmbientLightColor*U_AmbientMaterial;

	//diffuse
	//L vector
	vec3 L=vec3(0.0);
	float distance=0.0;
	float attenuation=1.0;
	//light attribute
	float constantFactor=0.5;
	float linearFactor=0.3;
	float expFactor=0.1;

	if(U_LightPos.w==0.0)
	{
		//direction light
		L=U_LightPos.xyz;
	}
	else
	{
		//model point -> light pos
		//point light / spot light
		L=U_LightPos.xyz-V_WorldPos;
		distance=length(L);
		attenuation=1.0/(constantFactor+linearFactor*distance+expFactor*distance*distance);
	}

	L=normalize(L);

	//N vector
	vec3 n=normalize(V_Normal);
	float diffuseIntensity=0.0;

	if(U_LightPos.w!=0.0f&&U_Cutoff>0.0)
	{
		//spotLight
		float currentCosThta=max(0.0,dot(-L,spotLightDirection));
		if(currentCosThta>cosThta)
		{
			if(dot(L,n)>0.0)
			{
				diffuseIntensity=pow(currentCosThta,U_LightDirection.w);
			}
		}
	}
	else
	{
		//point light or direction light
		diffuseIntensity=max(0.0,dot(L,n));
	}
	vec4 diffuseColor=U_DiffuseLightColor*U_DiffuseMaterial*diffuseIntensity*attenuation*U_DiffuseIntensity;
	
	//specular
	//inverse view direction : object->eye
	vec3 viewDir=U_EyePos-V_WorldPos;
	viewDir=normalize(viewDir);

	//reflection
	vec3 halfVector=L+viewDir;
	halfVector=normalize(halfVector);

	float specularIntensity=0.0;
	if(diffuseIntensity==0.0)
	{
		specularIntensity=0.0;
	}
	else
	{
		//shiness
		specularIntensity=pow(max(0.0,dot(n,halfVector)),128.0);
	}

	vec4 specularColor=U_SpecularLightColor*U_SpecularMaterial*specularIntensity*attenuation;

	vec4 color=ambientColor+diffuseColor+specularColor;
	
//	gl_FragData[1]=color;
//	gl_FragData[0]=color;
	if(color.r>1.0&&color.g>1.0&&color.b>1.0&&color.a>1.0)
	{
		gl_FragData[1]=color;
	}
	else
	{
		gl_FragData[0]=color;
	}
}