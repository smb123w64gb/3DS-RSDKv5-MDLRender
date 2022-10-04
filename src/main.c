///////////////////////////////////////
//           SDMC example            //
///////////////////////////////////////

//this example shows you how to load a binary image file from the SD card and display it on the lower screen
//for this to work you should copy test.bin to same folder as your .3dsx
//this file was generated with GIMP by saving a 240x320 image to raw RGB
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include <3ds.h>
#include <citro3d.h>
#include "vshader_shbin.h"
static float angleX = 0.0, angleY = 0.0;
static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static void* vbo_data;
static int uLoc_projection, uLoc_modelView;
static C3D_Mtx projection;
#define RSDK_SIGNATURE_MDL (0x4C444D)
#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))


//this will contain the data read from SDMC
u8* buffer;
//this will have the vertex stride for each frame
int* vStride;
int vStrideCount;
int vStrideCurrent = 0;
int vertexSize;

//3DS has VFPs so we could just use cos
//but we're old school so LUT4life
s32 pcCos(u16 v)
{
	return 1;
}
union u8ToFloat{
	u8 bytes[4];
	float value;
};
float ReadSingle(int* v){
	union u8ToFloat unValue;
	for(int x = 0;x<4;x++){
		unValue.bytes[x] = (char)buffer[x+*v];
	}
	float value = unValue.value;
	*v=*v + 4;
	return value;
}
int ReadInt32(int *v){
	int value = (int)buffer[0+*v]|((int)buffer[1+*v]<< 8)|((int)buffer[2+*v]<< 16)|((int)buffer[3+*v]<< 24);
	*v=*v + 4;
	return value;
	}
short ReadInt16(int *v){
	short value = (short)buffer[0+*v]|((short)buffer[1+*v]<< 8);
	*v=*v + 2;
	return value;
}
char ReadInt8(int *v){
	char value = (char)buffer[0+*v];
	*v=*v+1;
	return value;
}
char * ReadHeader(int *v){
	char *str = malloc(4);
	for(int x = 0;x<4;x++){
		str[x] = (char)buffer[x+*v];
	}
	*v=*v + 4;
	return str;
}
void renderEffect()
{
	static int cnt;
	u8* bufAdr=gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

	int i, j;
	for(i=1;i<400;i++)
	{
		for(j=1;j<240;j++)
		{
			u32 v=(j+i*240)*3;
			bufAdr[v]=(pcCos(i+cnt)+4096)/32;
			bufAdr[v+1]=(pcCos(j-256+cnt)+4096)/64;
			bufAdr[v+2]=(pcCos(i+128-cnt)+4096)/32;
		}
	}

	cnt++;
}

union Color {
    unsigned char bytes[4];
    uint color;
};
struct ModelVertex {
    float x;
    float y;
    float z;

    float nx;
    float ny;
    float nz;
};
struct TexCoord {
    float x;
    float y;
};
enum ModelFlags {
    MODEL_NOFLAGS     = 0,
    MODEL_USENORMALS  = 1 << 0,
    MODEL_USETEXTURES = 1 << 1,
    MODEL_USECOLOURS  = 1 << 2,
};

struct gpuVertex { float position[3]; float normal[3]; float color[4];};

void readMDL(){
	int offsetCur = 0;
	offsetCur = 4;
	char flags = ReadInt8(&offsetCur);

	char faceVertCount = ReadInt8(&offsetCur);

	short vertCount = ReadInt16(&offsetCur);
	short frameCount = ReadInt16(&offsetCur);
	printf("Verts Per Face:%i\nVert Count:%i\nFrame Count:%i\n",faceVertCount,vertCount,frameCount);
	struct ModelVertex *vertices = malloc(sizeof(struct ModelVertex)*vertCount*frameCount);
	
	if (flags & MODEL_USETEXTURES){
		struct TexCoord *texCoords = malloc(sizeof(struct TexCoord)*vertCount);
		for (int v = 0; v < vertCount; ++v) {
                texCoords[v].x = ReadSingle(&offsetCur);
                texCoords[v].y = ReadSingle(&offsetCur);
            }
		}
	union Color *colors = malloc(sizeof(union Color)*vertCount);
	if (flags & MODEL_USECOLOURS){
		for (int v = 0; v < vertCount; ++v) {
                colors[v].color = ReadInt32(&offsetCur);
				//printf("RGBA:%03i %03i %03i %03i",colors[v].bytes[0],colors[v].bytes[1],colors[v].bytes[2],colors[v].bytes[3]);
            }
		}
	short indexCount = ReadInt16(&offsetCur);
	printf("IndexCount:%i\n",indexCount);
	short *indices = malloc(sizeof(short)*indexCount);
	 for (int i = 0; i < indexCount; i++) {
		indices[i] = ReadInt16(&offsetCur);
	 }
	 
	 for (int f = 0; f < frameCount; ++f) {
            for (int v = 0; v < vertCount; ++v) {
                vertices[(f * vertCount) + v].x = ReadSingle(&offsetCur);
                vertices[(f * vertCount) + v].y = ReadSingle(&offsetCur);
                vertices[(f * vertCount) + v].z = ReadSingle(&offsetCur);

                vertices[(f * vertCount) + v].nx = 0;
                vertices[(f * vertCount) + v].ny = 0;
                vertices[(f * vertCount) + v].nz = 0;
                if (flags & MODEL_USENORMALS) {
                    vertices[(f * vertCount) + v].nx = ReadSingle(&offsetCur);
                    vertices[(f * vertCount) + v].ny = ReadSingle(&offsetCur);
                    vertices[(f * vertCount) + v].nz = ReadSingle(&offsetCur);
                }
            }
        }
	printf("%i\n",offsetCur);
	struct gpuVertex *Verts = malloc(sizeof(struct gpuVertex)*indexCount*frameCount);
	vStride = malloc(frameCount*sizeof(int));
	vStrideCount = frameCount;
	vertexSize = (int)indexCount;
	int y = 0;
	for(int f = 0;f<frameCount;++f){
		vStride[f] = f*indexCount;
		for(int x = 0;x<vertexSize; ++x){
			Verts[y].position[0] = vertices[(f * vertCount) + indices[x]].x;
			Verts[y].position[1] = vertices[(f * vertCount) + indices[x]].y;
			Verts[y].position[2] = vertices[(f * vertCount) + indices[x]].z;

			Verts[y].normal[0] = vertices[(f * vertCount) + indices[x]].nx;
			Verts[y].normal[1] = vertices[(f * vertCount) + indices[x]].ny;
			Verts[y].normal[2] = vertices[(f * vertCount) + indices[x]].nz;
			//printf("F:%i\tI:%i\tVert:%i\n",f,indices[x],(f * vertCount) + indices[x]);
			if (flags & MODEL_USECOLOURS){
			Verts[y].color[0] =  (float)colors[indices[x]].bytes[2]/255.0f;
			Verts[y].color[1] =  (float)colors[indices[x]].bytes[1]/255.0f;
			Verts[y].color[2] =  (float)colors[indices[x]].bytes[0]/255.0f;
			Verts[y].color[3] =  (float)colors[indices[x]].bytes[3]/255.0f;
			}else{
				Verts[y].color[0] = 1.0f;
				Verts[y].color[1] = 1.0f;
				Verts[y].color[2] = 1.0f;
				Verts[y].color[3] = 1.0f;
			}
			y++;
		}
	}
	// Load the vertex shader, create a shader program and bind it
	vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);

	// Get the location of the uniforms
	uLoc_projection   = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
	uLoc_modelView    = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");

	// Configure attributes for use with the vertex shader
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 3); // v1=normal
	AttrInfo_AddLoader(attrInfo, 2,  GPU_FLOAT, 4); // v2=color
	// Create the VBO (vertex buffer object)
	int allocation = sizeof(struct gpuVertex)*indexCount*frameCount;
	vbo_data = linearAlloc(allocation);
	
	memcpy(vbo_data, Verts, allocation);
	free(Verts);
	//printf("%f",Verts[0].position[1]);
	//printf("%f",Verts[0].position[2]);
	// Configure buffers
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo_data, sizeof(struct gpuVertex), 3, 0x210);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

printf("%i\n",vertexSize);
	
}

static void sceneRender(float iod)
{
	C3D_CullFace(GPU_CULL_NONE);
	Mtx_PerspStereoTilt(&projection, C3D_AngleFromDegrees(40.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, iod, 2.0f, false);
	// Calculate the modelView matrix
	C3D_Mtx modelView;
	Mtx_Identity(&modelView);
	Mtx_Translate(&modelView, 0.0, -35.0f, -175.0f, true);
	Mtx_RotateY(&modelView, angleY, true);

	// Rotate the cube each frame
	angleX += M_PI / 180;
	angleY += M_PI / 360;

	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView,  &modelView);

	// Draw the VBO
	
	//printf("%i\n",vertexSize/3);
	C3D_DrawArrays(GPU_TRIANGLES, vStride[vStrideCurrent], vertexSize);
}

int main(int argc, char** argv)
{

	gfxInitDefault(); //makes displaying to screen easier
	gfxSet3D(true);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	C3D_RenderTarget* targetLeft  = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTarget* targetRight = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(targetLeft,  GFX_TOP, GFX_LEFT,  DISPLAY_TRANSFER_FLAGS);
	C3D_RenderTargetSetOutput(targetRight, GFX_TOP, GFX_RIGHT, DISPLAY_TRANSFER_FLAGS);

	FILE *file = fopen("SonicJog.bin","rb");
	if (file == NULL) goto exit;

	// seek to end of file
	fseek(file,0,SEEK_END);

	// file pointer tells us the size
	off_t size = ftell(file);

	// seek back to start
	fseek(file,0,SEEK_SET);

	//allocate a buffer
	buffer=malloc(size);
	if(!buffer)goto exit;

	//read contents !
	fread(buffer,1,size,file);

	//close the file because we like being nice and tidy
	fclose(file);

	//This is where the fun begins
	//Also within RSDKv5 there are native functions to read data so this is not final to it.
	consoleInit(GFX_BOTTOM, NULL);

	readMDL();
	while(aptMainLoop())
	{
		//exit when user hits B
		hidScanInput();
		if(keysHeld()&KEY_B)break;

		float slider = osGet3DSliderState();
		float iod = slider/3;


		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		{

			C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, 0x68B0D8FF, 0);
			C3D_FrameDrawOn(targetLeft);
			sceneRender(-iod);

			if (iod > 0.0f)
			{
				C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, 0x68B0D8FF, 0);
				C3D_FrameDrawOn(targetRight);
				sceneRender(iod);
			}
		}
		C3D_FrameEnd(0);
		vStrideCurrent++;
		if(vStrideCurrent>=vStrideCount)vStrideCurrent=0;
		//gspWaitForEvent(GSPGPU_EVENT_VBlank0, false);
	}

	//cleanup and return
	//returning from main() returns to hbmenu when run under ninjhax
	exit:

	//closing all services even more so
	linearFree(vbo_data);
	free(buffer);
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
	C3D_Fini();
	gfxExit();
	return 0;
}

