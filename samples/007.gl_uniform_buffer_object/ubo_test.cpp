#include "platform.h"
#include "wrapper_gl.hpp"
#include <shader_gl.hpp>
#include "transforms.h"

/*
simple example showing sharing values across two shaders
*/

static const float vertices[] = {
	//0
	 0,0,0,			//P
	 1.0f,0.3f,0.3f,//C
	 //1
	 1.0f,0,0,		//P
	 1.0f,0.3f,0.3f,//C
	 //2
	 0,0,0,			//P
	 0.3f,1.0f,0.3f,//C
	 //3
	 0,1.0f,0,		//P
	 0.3f,1.0f,0.3f,//C
	 //4
	 0,0,0,			//P
	 0.3f,0.3f,1.0f,//C
	 //5
	 0,0,1.0f,		//P
	 0.3f,0.3f,1.0f,//C
};

static unsigned char indices[] = {	0, 1, 2, 3, 4, 5 };


static const char* pVertexShader = "\
#version 420 core\n\
layout(location = 0) in vec3 inVertex;\n\
layout(location = 1) in vec3 inColour;\n\
layout(std140, binding=1) uniform Transforms\n\
{\n\
uniform mat4		mvp;\n\
uniform mat4		proj;\n\
uniform mat4		mv;\n\
uniform mat4		nrmn;\n\
}trans;\n\
out vec3 colour;\n\
void main()\n\
{\n\
	colour = inColour;\n\
	vec4 finalPosition = trans.mvp * vec4(inVertex,1) ;\n\
	gl_Position = finalPosition;\n\
	gl_PointSize = 10.0f;\n\
}\n\
";

static const char* pVertexShader2 = "\
#version 420 core\n\
layout(location = 0) in vec3 inVertex;\n\
layout(location = 1) in vec3 inColour;\n\
layout(std140, binding=1) uniform Transforms\n\
{\n\
uniform mat4		mvp;\n\
uniform mat4		proj;\n\
uniform mat4		mv;\n\
uniform mat4		nrmn;\n\
}trans;\n\
out vec3 colour;\n\
void main()\n\
{\n\
	colour = inColour;\n\
	vec4 finalPosition = trans.mvp * vec4(inVertex+vec3(0.5,0.5,0),1) ;\n\
	gl_Position = finalPosition;\n\
	gl_PointSize = 20.0f;\n\
}\n\
";

static const char* pFragmentShader = "\
#version 420 core\n\
in vec3 colour;\n\
out vec4 fragColour;\n\
void main()\n\
{\n\
float d = (1.0 - gl_FragCoord.z);\n\
fragColour = vec4(colour.xyz*d, 1);\n\
}\n\
";

typedef struct {
	CMatrix44 mvp;
	CMatrix44 proj; 
	CMatrix44 mv;
	float nrm[16]; //3x3 matrix	
} Transforms;

//static variables 
CPlatform* pPlatform = 0;
unsigned int vao=0, ab=0, eab, ubo;	
Transform transform(10);
CShaderProgram program[2];
float latitude = 0.0f, longitude = 0.0f;
CVec3df cameraPosition(0, 0.0f, -2.0f);
Transforms transforms;

//functions
void Setup(CPlatform * const  pPlatform);
void MainLoop(CPlatform * const pPlatform);
void CleanUp(void);


int PlatformMain( void ){
	pPlatform = GetPlatform();	
	pPlatform->ShowDebugConsole();
	pPlatform->Create(L"uniform buffer object example, phasersonkill.com", 4, 2, 640, 640, 24, 8, 24, 8, CPlatform::MS_0);
	Setup(pPlatform);

	while(!pPlatform->IsQuitting())
		MainLoop(pPlatform);
	CleanUp();
	return 0;
}

void Setup(CPlatform * const  pPlatform)
{
	CShader vertexShader(CShader::VERT, &pVertexShader, 1);
	CShader vertexShader2(CShader::VERT, &pVertexShader2, 1);
	CShader fragmentShader(CShader::FRAG, &pFragmentShader, 1);
	

	// - - - - - - - - - - 
	//setup the shaders	
	program[0].Initialise();
	program[0].AddShader(&vertexShader);
	program[0].AddShader(&fragmentShader);
	program[0].Link();

	program[1].Initialise();
	program[1].AddShader(&vertexShader2);
	program[1].AddShader(&fragmentShader);
	program[1].Link();

	int blockAlign = CShaderProgram::GetUniformBlockAlignment();
	int blockLoc = program[0].GetUniformBlockBinding(program[0].GetUniformBlockLocation("Transforms")); //should be 5 :)
	int blockSiz = program[0].GetUniformBlockSize(program[0].GetUniformBlockLocation("Transforms")); //should be 5 :)
	int namelength = program[0].GetUniformBlockInfo(program[0].GetUniformBlockLocation("Transforms"),CShaderProgram::NAME_LENGTH);
	int activeUniforms = program[0].GetUniformBlockInfo(program[0].GetUniformBlockLocation("Transforms"),CShaderProgram::BLK_ACTIVE_UNIFORMS);
	int uniformindices[100];
	program[0].GetUniformBlockInfo(program[0].GetUniformBlockLocation("Transforms"),CShaderProgram::BLK_ACTIVE_UNIFORMS_INDICES, uniformindices);

	// - - - - - - - - - - 
	// setup vertex buffers etc
	//glGenVertexArrays(1,&vao);	//vertex array object	
	vao = WRender::CreateVertexArrayObject();
	eab = WRender::CreateBuffer(WRender::ELEMENTS, WRender::STATIC, sizeof(indices), indices);
	ab = WRender::CreateBuffer(WRender::ARRAY, WRender::STATIC, sizeof(vertices), vertices);
	ubo = WRender::CreateBuffer(WRender::UNIFORM, WRender::DYNAMIC, sizeof(Transforms), &transform);
	WRender::BindBufferToIndex(WRender::UNIFORM, ubo, 1);
	
	WRender::BindVertexArrayObject(vao);
	WRender::BindBuffer(WRender::ELEMENTS, eab);
	WRender::VertexAttribute va[2] = {
		{ab, 0, 3, WRender::FLOAT, 0, sizeof(float)*6, 0, 0},
		{ab, 1, 3, WRender::FLOAT, 0, sizeof(float)*6, sizeof(float)*3, 0},		
	};
	WRender::SetAttributeFormat( va, 2, 0);	
	
	Transform::CreateProjectionMatrix(transforms.proj, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);	
	WRender::SetClearColour(0,0,0,0);
}

void MainLoop(CPlatform * const  pPlatform)
{	
	//update the main application
	pPlatform->Tick();	
	WRender::ClearScreenBuffer(COLOR_BIT | DEPTH_BIT); //not really needed 		
	WRender::EnableDepthTest(true);
	transform.Push();			
	{
		CMatrix44 rotLat,rotLong;
		rotLat.Rotate(latitude, 1, 0, 0);
		rotLong.Rotate(longitude, 0, 1, 0);

		transform.Translate(cameraPosition);
		transform.ApplyTransform(rotLat);
		transform.ApplyTransform(rotLong);			

		transform.Push();					
		{
			transforms.mvp = transforms.proj * transform.GetCurrentMatrix();
			//program.SetMtx44(uMVP, transforms.mvp.data);
			WRender::UpdateBuffer(WRender::UNIFORM, WRender::DYNAMIC, ubo, sizeof(Transforms), (void*)&transforms, 0);				
				
			program[0].Start();
			WRender::Draw(WRender::LINES, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);
			WRender::Draw(WRender::POINTS, WRender::U_BYTE, 1, 5);

			program[1].Start();				
			WRender::Draw(WRender::LINES, WRender::U_BYTE, sizeof(indices)/sizeof(unsigned char), 0);
			WRender::Draw(WRender::POINTS, WRender::U_BYTE, 1, 5);
		}
		transform.Pop();
	}
	transform.Pop();		
	pPlatform->UpdateBuffers();
	if(pPlatform->GetKeyboard().keys[KB_UP].IsPressed())
		latitude += 90.0f * pPlatform->GetDT();
	if(pPlatform->GetKeyboard().keys[KB_DOWN].IsPressed())
		latitude -= 90.0f * pPlatform->GetDT();
		
	if(pPlatform->GetKeyboard().keys[KB_LEFT].IsPressed())
		longitude += 90.0f * pPlatform->GetDT();
	if(pPlatform->GetKeyboard().keys[KB_RIGHT].IsPressed())
		longitude -= 90.0f * pPlatform->GetDT();
}

void CleanUp(void)
{	
	WRender::DeleteVertexArrayObject(vao);
	WRender::DeleteBuffer(ab);
	WRender::DeleteBuffer(eab);
	WRender::DeleteBuffer(ubo);
	//shaders were sorted when they go out of scope
	program[0].CleanUp();
	program[1].CleanUp();	
	//GLenum a = glGetError();
}