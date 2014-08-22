/*
	This file is part of Mandelbrot Explorer.

	Mandelbrot Explorer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Mandelbrot Explorer.  If not, see <http://www.gnu.org/licenses/>.
	Copyright 2014 ProgrammerNerd/ComputerNerd (or whatever screename you know me as)
*/
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "mandelbrot.h"
#include "screen.h"
#include "input.h"
static inline void mandelFull(int64_t* scale,uint16_t z,int deep){
	#ifdef PC
		if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
		mandel((uint16_t *)VRAM_ADDRESS,SCREEN_WIDTH,SCREEN_HEIGHT,deep,z,scale[0],scale[1],0,scale[2],scale[3],0);
		if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	#endif
	#ifdef CASIO_PRIZM
		mandel((uint16_t *)VRAM_ADDRESS,SCREEN_WIDTH,SCREEN_HEIGHT,deep,z,scale[0],scale[1],0,scale[2],scale[3],0);
	#endif
}
#define redrawFull 1
#define redrawUD 2
#define redrawLR 4
#define redrawS 8
static int64_t divXX,divYY;
static int handleScale(int redraw,int deep,int64_t*scaleN,int64_t*scaleO,uint16_t z){
	if(redraw&redrawFull)
		return redrawFull;
	if(redraw&redrawS){
		//scale vram first then if the user does not press any buttons draw it more clear later
		#ifdef CASIO_PRIZM
			DmaWaitNext();
		#endif
		#ifdef PC
			if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
		#endif
		mandelQuater((uint16_t*)VRAM_ADDRESS,SCREEN_WIDTH,SCREEN_HEIGHT,deep,z,scaleN[0],scaleN[1],0,scaleN[2],scaleN[3],0);
		#ifdef PC
			if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
		#endif
		redraw&=~(redrawS|redrawUD|redrawLR);
	}
	return redraw;
}
static int handleUDLR(int redraw,int deep,int64_t*scaleN,int64_t*scaleO,uint16_t z){
	if(redraw&redrawFull)
		return redrawFull;
	if((redraw&redrawUD)||(redraw&redrawLR)){
		#ifdef CASIO_PRIZM
			DmaWaitNext();
		#endif
		#ifdef PC
			if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
		#endif
		uint16_t*vram=(uint16_t*)VRAM_ADDRESS;
		int deltaX;
		deltaX=(int)(((int64_t)scaleN[0]-(int64_t)scaleO[0])*(int64_t)SCREEN_WIDTH/(int64_t)divX);
		int deltaY;
		deltaY=(int)(((int64_t)scaleN[2]-(int64_t)scaleO[2])*(int64_t)SCREEN_HEIGHT/(int64_t)divY);
		if(deltaX>0){
			//move everything left (right was pressed)
			unsigned y;
			int x;
			for(y=0;y<SCREEN_HEIGHT;++y){
				for(x=0;x<SCREEN_WIDTH-deltaX;++x)
					vram[x]=vram[x+deltaX];
				vram+=SCREEN_WIDTH;
			}
			mandel((uint16_t*)VRAM_ADDRESS,SCREEN_WIDTH,SCREEN_HEIGHT,deep,z,scaleO[1],scaleN[1],SCREEN_WIDTH-deltaX,scaleO[2],scaleO[3],0);
		}else if(deltaX<0){
			//move everything to the right (left was pressed)
			deltaX*=-1;
			unsigned y;
			int x;
			for(y=0;y<SCREEN_HEIGHT;++y){
				for(x=SCREEN_WIDTH-deltaX;x>=0;--x)
					vram[x+deltaX]=vram[x];
				vram+=SCREEN_WIDTH;
			}
			mandel((uint16_t*)VRAM_ADDRESS,SCREEN_WIDTH,SCREEN_HEIGHT,deep,z,scaleN[0],scaleN[0]-(scaleN[0]-scaleO[0]),0,scaleO[2],scaleO[3],0);
		}
		vram=(uint16_t*)VRAM_ADDRESS;
		if(deltaY>0){
			//Move everything up (down was pressed)
			memmove(vram,vram+(deltaY*SCREEN_WIDTH),(SCREEN_HEIGHT-deltaY)*SCREEN_WIDTH_2);
			mandel((uint16_t*)VRAM_ADDRESS,SCREEN_WIDTH,SCREEN_HEIGHT,deep,z,scaleN[0],scaleN[1],0,scaleO[3],scaleN[3],SCREEN_HEIGHT-deltaY);
			#ifdef PC
				if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
			#endif
		}else if(deltaY<0){
			//Move image down (up was pressed)
			memmove(vram+(-(deltaY*SCREEN_WIDTH)),vram,(SCREEN_HEIGHT+deltaY)*SCREEN_WIDTH_2);
			mandel((uint16_t*)VRAM_ADDRESS,SCREEN_WIDTH,SCREEN_HEIGHT,deep,z,scaleN[0],scaleN[1],0,scaleN[2],scaleN[2]-(scaleN[2]-scaleO[2]),0);
			#ifdef PC
				if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
			#endif
		}
		redraw&=~(redrawUD|redrawLR);
	}
	return redraw;
}
static void setScale(int64_t * scale,int64_t w,int64_t h){//format minx maxx miny maxy
	divXX=divX;
	divYY=divY;
	divX=absll(scale[0]-scale[1]);
	divY=absll(scale[2]-scale[3]);
	stepX=divX/(int64_t)w;
	stepY=divY/(int64_t)h;
	if(!stepX)
		stepX=1;
	if(!stepY)
		stepY=1;
}
int main(void){
	int64_t scaleN[4] = {(int64_t)-2<<wBits,(int64_t)1<<wBits,(int64_t)-1<<wBits,(int64_t)1<<wBits};
	int64_t scaleO[4] = {(int64_t)-2<<wBits,(int64_t)1<<wBits,(int64_t)-1<<wBits,(int64_t)1<<wBits};
	#ifdef CASIO_PRIZM
		Bdisp_EnableColor(1);
		clearScreen();
		Bdisp_PutDisp_DD();
	#endif
	#ifdef PC
		if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0){
			printf("Could not start SDL %s\n", SDL_GetError());
			return;
		}
		screen=SDL_SetVideoMode(SCREEN_WIDTH,SCREEN_HEIGHT,16,SDL_SWSURFACE);
	#endif
	setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
	divXX=divX;
	divYY=divY;
	uint16_t z=224;//good values for z 28 31 36 41 42 224 is my favorite so far
	calcColorTab(z);
	//mandelFull(scaleN,z,1);
	//FlipScreen();
	#ifdef CASIO_PRIZM
		DmaWaitNext();//Needed before accessing screen
	#endif
	unsigned redraw=redrawFull;
	unsigned deep=1;
	unsigned waiting=0;
	unsigned waitingnum=0;
	for(;;){
		#ifdef PC
			if(checkExit())//Also needed to update keys for SDL
				break;
		#endif
		if(keyPressed(KEY_FOR_EXIT)){
			#ifdef CASIO_PRIZM
				DmaWaitNext();
				int key;
				GetKey(&key);
			#endif
			#ifdef PC
				break;
			#endif
		}
		if(keyPressed(KEY_F1)){
			z=0xFFFF;
			calcColorTab(z);
			redraw=redrawFull;
		}
		if(keyPressed(KEY_F2)){
			z=224;
			calcColorTab(z);
			redraw=redrawFull;
		}
		if(keyPressed(KEY_F3)){
			deep^=1;
			redraw=redrawFull;
		}
		if(keyPressed(KEY_1)){
			--z;
			calcColorTab(z);
			redraw=redrawFull;
		}
		if(keyPressed(KEY_2)){
			++z;
			calcColorTab(z);
			redraw=redrawFull;
		}
		if(keyPressed(KEY_3)){
			z-=10;
			calcColorTab(z);
			redraw=redrawFull;
		}
		if(keyPressed(KEY_4)){
			z+=10;
			calcColorTab(z);
			redraw=redrawFull;
		}
		if(keyPressed(KEY_5)){
			z-=100;
			calcColorTab(z);
			redraw=redrawFull;
		}
		if(keyPressed(KEY_6)){
			z+=100;
			calcColorTab(z);
			redraw=redrawFull;
		}
		memcpy(scaleO,scaleN,4*sizeof(int64_t));
		if(keyPressed(KEY_SHIFT)){
			scaleN[0]+=absll(scaleN[1]-scaleN[0])/64LL;
			scaleN[1]-=absll(scaleN[1]-scaleN[0])/64LL;
			scaleN[2]+=absll(scaleN[3]-scaleN[2])/64LL;
			scaleN[3]-=absll(scaleN[3]-scaleN[2])/64LL;
			setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
			redraw|=redrawS;
		}
		if(keyPressed(KEY_ALPHA)){
			scaleN[0]-=absll(scaleN[1]-scaleN[0])/64LL;
			scaleN[1]+=absll(scaleN[1]-scaleN[0])/64LL;
			scaleN[2]-=absll(scaleN[3]-scaleN[2])/64LL;
			scaleN[3]+=absll(scaleN[3]-scaleN[2])/64LL;
			setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
			redraw|=redrawS;
		}
		if(keyPressed(KEY_UP)){
			scaleN[2]-=absll(scaleN[3]-scaleN[2])/64LL;
			scaleN[3]-=absll(scaleN[3]-scaleN[2])/64LL;
			setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
			redraw|=redrawUD;
		}
		if(keyPressed(KEY_DOWN)){
			scaleN[2]+=(int64_t)absll(scaleN[3]-scaleN[2])/64LL;
			scaleN[3]+=(int64_t)absll(scaleN[3]-scaleN[2])/64LL;
			setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
			redraw|=redrawUD;
		}
		if(keyPressed(KEY_LEFT)){
			scaleN[0]-=(int64_t)absll(scaleN[1]-scaleN[0])/64LL;
			scaleN[1]-=(int64_t)absll(scaleN[1]-scaleN[0])/64LL;
			setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
			redraw|=redrawLR;
		}
		if(keyPressed(KEY_RIGHT)){
			scaleN[0]+=absll(scaleN[1]-scaleN[0])/64LL;
			scaleN[1]+=absll(scaleN[1]-scaleN[0])/64LL;
			setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
			redraw|=redrawLR;
		}
		if(redraw&redrawFull){
			//Full redraw needed
			//mandelFull(scaleN,z,deep);
			unsigned i;
			for(i=0;i<16;++i){
				mandelMask((uint16_t*)VRAM_ADDRESS,SCREEN_WIDTH,SCREEN_HEIGHT,deep,z,scaleN[0],scaleN[1],0,scaleN[2],scaleN[3],0,i&3,i/4,3);
				FlipScreen();
			}
			waiting=0;
			waitingnum=0;
			redraw=0;
		}
		if(waiting&&(!redraw)){
			if(waitingnum>=15)
				waiting=0;
			#ifdef CASIO_PRIZM
				DmaWaitNext();
			#endif
			#ifdef PC
				if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
			#endif
			mandelMask((uint16_t*)VRAM_ADDRESS,SCREEN_WIDTH,SCREEN_HEIGHT,deep,z,scaleN[0],scaleN[1],0,scaleN[2],scaleN[3],0,waitingnum&3,waitingnum/4,3);
			#ifdef PC
				if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
			#endif
			++waitingnum;
			FlipScreen();
		}
		int redrawOld=redraw;
		if(redraw){
			waiting=1;
			if(redraw&redrawS)
				waitingnum=1;
			else
				waitingnum=0;
		}
		redraw=handleScale(redraw,deep,scaleN,scaleO,z);
		redraw=handleUDLR(redraw,deep,scaleN,scaleO,z);
		if((redrawOld!=redraw))
			FlipScreen();
	}
}
