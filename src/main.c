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

#define RSDK_SIGNATURE_MDL (0x4C444D)

//this will contain the data read from SDMC
u8* buffer;

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
    char bytes[4];
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

void readMDL(){
	int offsetCur = 0;
	printf("EJJKREKOEOE\n");
	int sig = ReadInt32(&offsetCur);
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
	if (flags & MODEL_USECOLOURS){
		union Color *colors = malloc(sizeof(union Color)*vertCount);
		for (int v = 0; v < vertCount; ++v) {
                colors[v].color = ReadInt32(&offsetCur);
				//printf("RGBA:%03i %03i %03i %03i",colors[v].bytes[0],colors[v].bytes[1],colors[v].bytes[2],colors[v].bytes[3]);
            }
		}
	short indexCount = ReadInt16(&offsetCur);
	printf("%i\n",offsetCur);
	short *indices = malloc(sizeof(short)*indexCount);
	 for (int i = 0; i < indexCount; ++i) {
		indices[i] = ReadInt16(&offsetCur);
	 }
	 printf("%i\n",offsetCur);
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
					//printf("%i",offsetCur);
                }
            }
        }
	printf("%f\n",vertices[0].nx);
	


}

int main(int argc, char** argv)
{

	gfxInitDefault(); //makes displaying to screen easier

	FILE *file = fopen("UFOChase.bin","rb");
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
	off_t bytesRead = fread(buffer,1,size,file);

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

		//render rainbow
		renderEffect();

		//copy buffer to lower screen (don't have to do it every frame)
		memcpy(gfxGetFramebuffer(GFX_TOP, GFX_TOP, NULL, NULL), buffer, size);

		//wait & swap
		gfxSwapBuffersGpu();
		gspWaitForEvent(GSPGPU_EVENT_VBlank0, false);
	}

	//cleanup and return
	//returning from main() returns to hbmenu when run under ninjhax
	exit:

	//closing all services even more so
	gfxExit();
	return 0;
}

