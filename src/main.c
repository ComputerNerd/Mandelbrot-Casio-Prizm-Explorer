/*
	This file is part of Mandelbrot Explorer.

	Mandelbrot Explorer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Mandelbrot Explorer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Mandelbrot Explorer.  If not, see <http://www.gnu.org/licenses/>.
	Copyright 2017 ProgrammerNerd/ComputerNerd (or whatever screename you know me as)
*/
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "screen.h"
#include "input.h"
#include "config.h"
void waitKey(void){
	#ifdef PC
		SDL_Event keyevent;
		do{
			SDL_WaitEvent(&keyevent);
		}while(keyevent.type != SDL_KEYDOWN);
		do{
			SDL_WaitEvent(&keyevent);
		}while(keyevent.type != SDL_KEYUP);
	#endif
	#ifdef CASIO_PRIZM
		int key;
		GetKey(&key);
	#endif
}
int keyPressed(int basic_keycode){
	#ifdef PC
		uint8_t*keystate=SDL_GetKeyState(NULL);
		return keystate[basic_keycode];
	#endif
	#ifdef CASIO_PRIZM
		const volatile unsigned short* keyboard_register = (unsigned short*)0xA44B0000;
		int row, col, word, bit;
		row = basic_keycode%10;
		col = basic_keycode/10-1;
		word = row>>1;
		bit = col + ((row&1)<<3);
		return (0 != (keyboard_register[word] & 1<<bit));
	#endif
}
int checkExit(void){
	#ifdef CASIO_PRIZM
		return keyPressed(KEY_PRGM_MENU);
	#endif
	#ifdef PC
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			if(event.type==SDL_QUIT)
				return 1;
		}
		return 0;
	#endif
}
#ifdef PC
SDL_Surface *screen;
#endif
#ifdef CASIO_PRIZM
#define LCD_GRAM 0x202
#define LCD_BASE	0xB4000000
unsigned short* VRAM_ADDR;
#define SYNCO() __asm__ volatile("SYNCO\n\t":::"memory");
// Module Stop Register 0
#define MSTPCR0	(volatile unsigned *)0xA4150030
// DMA0 operation register
#define DMA0_DMAOR	(volatile unsigned short*)0xFE008060
#define DMA0_SAR_0	(volatile unsigned *)0xFE008020
#define DMA0_DAR_0  (volatile unsigned *)0xFE008024
#define DMA0_TCR_0	(volatile unsigned *)0xFE008028
#define DMA0_CHCR_0	(volatile unsigned *)0xFE00802C
void DmaWaitNext(void){
	while(1){
		if((*DMA0_DMAOR)&4)//Address error has occurred stop looping
			break;
		if((*DMA0_CHCR_0)&2)//Transfer is done
			break;
	}
	SYNCO();
	*DMA0_CHCR_0&=~1;
	*DMA0_DMAOR=0;
}
void FlipScreen(void){
	Bdisp_WriteDDRegister3_bit7(1);
	Bdisp_DefineDMARange(6,389,0,215);
	Bdisp_DDRegisterSelect(LCD_GRAM);

	*MSTPCR0&=~(1<<21);//Clear bit 21
	*DMA0_CHCR_0&=~1;//Disable DMA on channel 0
	*DMA0_DMAOR=0;//Disable all DMA
	*DMA0_SAR_0=((unsigned)VRAM_ADDR)&0x1FFFFFFF;//Source address is VRAM
	*DMA0_DAR_0=LCD_BASE&0x1FFFFFFF;//Destination is LCD
	*DMA0_TCR_0=(216*384)/16;//Transfer count bytes/32
	*DMA0_CHCR_0=0x00101400;
	*DMA0_DMAOR|=1;//Enable DMA on all channels
	*DMA0_DMAOR&=~6;//Clear flags
	*DMA0_CHCR_0|=1;//Enable channel0 DMA
}
#endif
void clearScreen(void){
	#ifdef CASIO_PRIZM
		memset((void *)VRAM_ADDR,0,384*216*2);
	#endif
	#ifdef PC
		if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
		memset(screen->pixels,0,screen->w*screen->h*2);
		if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	#endif
}
static int64_t stepX;
static int64_t stepY;
static int64_t divX;
static int64_t divY;
typedef int fixed_t;
#define preMan 13
#define deepMan 26
#define deepManInt (32-deepMan)
#define difShift (deepMan-preMan)
#define wBits 48
#define WDifpre (wBits-preMan)
#define WDifdeep (wBits-deepMan)
static inline fixed_t absf(fixed_t x){return x>0?x:-x;}
static inline fixed_t square(fixed_t x){return (x*x)>>preMan;}
static inline fixed_t squaredeep(fixed_t x){return (fixed_t)(((int64_t)x*(int64_t)x)>>deepMan);}
static uint16_t colorTab[65536];
static inline int64_t absll(int64_t x){return x>0?x:-x;}
static void calcColorTab(uint16_t maxit){
	uint_fast32_t it;
	for(it=0;it<=maxit;++it)
		colorTab[it]=it*0xFFFF/maxit;
}
static uint16_t ManItDeep(fixed_t c_r,fixed_t c_i,unsigned maxit){//manIt stands for mandelbrot iteration what did you think it stood for?
	//c_r = scaled x coordinate of pixel (must be scaled to lie somewhere in the mandelbrot X scale (-2.5, 1)
	//c_i = scaled y coordinate of pixel (must be scaled to lie somewhere in the mandelbrot Y scale (-1, 1)
	// squre optimization code below originally from http://randomascii.wordpress.com/2011/08/13/faster-fractals-through-algebra/
	//early bailout code from http://locklessinc.com/articles/mandelbrot/
	//changed by me to use fixed point math
	fixed_t ckr,cki;
	unsigned p=0,ptot=8;
	fixed_t z_r = c_r;
	fixed_t z_i = c_i;
	fixed_t zrsqr = squaredeep(z_r);
	fixed_t zisqr = squaredeep(z_i);
	fixed_t q=squaredeep(c_r-((1<<deepMan)/4))+squaredeep(c_i);
	if((((int64_t)q*((int64_t)q+((int64_t)c_r-((1<<deepMan)/4))))>>deepMan) < (squaredeep(c_i)/4))
		return 0;
	//int zrsqr,zisqr;
	do{
		ckr = z_r;
		cki = z_i;
		ptot += ptot;
		if(ptot > maxit) ptot = maxit;
		for(; p < ptot;++p){
			z_i =(squaredeep(z_r+z_i))-zrsqr-zisqr;
			z_i += c_i;
			z_r = zrsqr-zisqr+c_r;
			zrsqr = squaredeep(z_r);
			zisqr = squaredeep(z_i);
			if((zrsqr + zisqr) > (4<<deepMan))
				return colorTab[p];
			if((z_r == ckr) && (z_i == cki))
				return 0;
		}
	} while (ptot != maxit);
	//return (uint32_t)p*(uint32_t)0xFFFF/(uint32_t)maxit;
	return 0;
}
static uint16_t ManIt(fixed_t c_r,fixed_t c_i,unsigned maxit){
	//same credits as above
	fixed_t ckr,cki;
	unsigned p=0,ptot=8;
	fixed_t z_r = c_r;
	fixed_t z_i = c_i;
	fixed_t zrsqr = (z_r * z_r)>>preMan;
	fixed_t zisqr = (z_i * z_i)>>preMan;
	fixed_t q=square(c_r-((1<<preMan)/4))+square(c_i);
	if(((q*(q+(c_r-((1<<preMan)/4))))>>preMan) < (square(c_i)/4))
		return 0;
	do{
		ckr = z_r;
		cki = z_i;
		ptot += ptot;
		if(ptot > maxit) ptot = maxit;
		for(; p < ptot;++p){
			z_i =(square(z_r+z_i))-zrsqr-zisqr;
			z_i += c_i;
			z_r = zrsqr-zisqr+c_r;
			zrsqr = square(z_r);
			zisqr = square(z_i);
			if((zrsqr + zisqr) > (4<<preMan))
				return colorTab[p];
			if((z_r == ckr) && (z_i == cki))
				return 0;
		}
	} while (ptot != maxit);
	return 0;
}
static void mandel(uint16_t*dst,unsigned w,unsigned h,int deep,unsigned maxit,int64_t minX,int64_t maxX,uint16_t poffX,int64_t minY,int64_t maxY,uint16_t poffY){
	int64_t x,y;
	unsigned xx,yy=poffY;
	dst+=(poffY*w);
	if(deep){
		for(y=minY;y<maxY;y+=stepY){
			dst+=poffX;
			xx=poffX;
			if(yy>=h)
				return;
			for(x=minX;x<maxX;x+=stepX){
				if(xx>=w)
					break;
				*dst++=ManItDeep((int64_t)x>>(int64_t)WDifdeep,(int64_t)y>>(int64_t)WDifdeep,maxit);
				++xx;
			}
			++yy;
			dst+=w-xx;
		}
	}else{
		for(y=minY;y<maxY;y+=stepY){
			dst+=poffX;
			xx=poffX;
			if(yy>=h)
				return;
			for(x=minX;x<maxX;x+=stepX){
				if(xx>=w)
					break;
				*dst++=ManIt((int64_t)x>>(int64_t)WDifpre,(int64_t)y>>(int64_t)WDifpre,maxit);
				++xx;
			}
			++yy;
			dst+=w-xx;
		}
	}
}
static void mandelMask(uint16_t*dst,unsigned w,unsigned h,int deep,uint16_t maxit,int64_t minX,int64_t maxX,uint16_t poffX,int64_t minY,int64_t maxY,uint16_t poffY,unsigned skipX,unsigned skipY,unsigned mask){
	int64_t x,y;
	unsigned xx,yy=poffY;
	dst+=(poffY*w);
	if(deep){
		for(y=minY;y<maxY;y+=stepY){
			dst+=poffX;
			xx=poffX;
			if(yy>=h)
				return;
			if((yy&mask)==skipY){
				for(x=minX;x<maxX;x+=stepX){
					if(xx>=w)
						break;
					if((xx&mask)==skipX)
						*dst++=ManItDeep((int64_t)x>>(int64_t)WDifdeep,(int64_t)y>>(int64_t)WDifdeep,maxit);
					else
						++dst;
					++xx;
				}
			}
			++yy;
			dst+=w-xx;
		}
	}else{
		for(y=minY;y<maxY;y+=stepY){
			dst+=poffX;
			xx=poffX;
			if(yy>=h)
				return;
			if((yy&mask)==skipY){
				for(x=minX;x<maxX;x+=stepX){
					if(xx>=w)
						break;
					if((xx&mask)==skipX)
						*dst++=ManIt((int64_t)x>>(int64_t)WDifpre,(int64_t)y>>(int64_t)WDifpre,maxit);
					else
						++dst;
					++xx;
				}
			}
			++yy;
			dst+=w-xx;
		}
	}
}
static void mandelQuater(uint16_t*dst,unsigned w,unsigned h,int deep,uint16_t maxit,int64_t minX,int64_t maxX,uint16_t poffX,int64_t minY,int64_t maxY,uint16_t poffY){
	int64_t x,y;
	unsigned xx,yy=poffY;
	dst+=(poffY*w);
	if(deep){
		for(y=minY;y<maxY;y+=stepY*4){
			dst+=poffX;
			xx=poffX;
			if(yy>=h)
				return;
			for(x=minX;x<maxX;x+=stepX*4){
				if(xx>=w)
					break;
				uint16_t val=ManItDeep((int64_t)x>>(int64_t)WDifdeep,(int64_t)y>>(int64_t)WDifdeep,maxit);
				*dst++=val;
				*dst++=val;
				*dst++=val;
				*dst++=val;
				xx+=4;
			}
			dst+=w-xx;
			__builtin_memcpy(dst,dst-w,w*2);
			dst+=w;
			__builtin_memcpy(dst,dst-w,w*2);
			dst+=w;
			__builtin_memcpy(dst,dst-w,w*2);
			dst+=w;
			yy+=4;
		}
	}else{
		for(y=minY;y<maxY;y+=stepY*4){
			dst+=poffX;
			xx=poffX;
			if(yy>=h)
				return;
			for(x=minX;x<maxX;x+=stepX*4){
				if(xx>=w)
					break;
				uint16_t val=ManIt((int64_t)x>>(int64_t)WDifpre,(int64_t)y>>(int64_t)WDifpre,maxit);
				*dst++=val;
				*dst++=val;
				*dst++=val;
				*dst++=val;
				xx+=4;
			}
			dst+=w-xx;
			__builtin_memcpy(dst,dst-w,w*2);
			dst+=w;
			__builtin_memcpy(dst,dst-w,w*2);
			dst+=w;
			__builtin_memcpy(dst,dst-w,w*2);
			dst+=w;
			yy+=4;
		}
	}
}
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
		int deltaX=(int)(((int64_t)scaleN[0]-(int64_t)scaleO[0])*(int64_t)SCREEN_WIDTH/(int64_t)divX);
		int deltaY=(int)(((int64_t)scaleN[2]-(int64_t)scaleO[2])*(int64_t)SCREEN_HEIGHT/(int64_t)divY);
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
				for(x=SCREEN_WIDTH-1;x>=deltaX;--x)
					vram[x]=vram[x-deltaX];
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
	divX=scale[1]-scale[0];
	divY=scale[3]-scale[2];
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
		VRAM_ADDR = (unsigned short*)GetVRAMAddress();
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
		__builtin_memcpy(scaleO,scaleN,4*sizeof(int64_t));
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
			int64_t moveY=divY/64;
			scaleN[2]-=moveY;
			scaleN[3]-=moveY;
			setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
			redraw|=redrawUD;
		}
		if(keyPressed(KEY_DOWN)){
			int64_t moveY=divY/64;
			scaleN[2]+=moveY;
			scaleN[3]+=moveY;
			setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
			redraw|=redrawUD;
		}
		if(keyPressed(KEY_LEFT)){
			int64_t moveX=divX/128;
			scaleN[0]-=moveX;
			scaleN[1]-=moveX;
			//setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
			redraw|=redrawLR;
		}
		if(keyPressed(KEY_RIGHT)){
			int64_t moveX=divX/128;
			scaleN[0]+=moveX;
			scaleN[1]+=moveX;
			//setScale(scaleN,SCREEN_WIDTH,SCREEN_HEIGHT);
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
