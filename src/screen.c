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
#include "config.h"
#ifdef PC
SDL_Surface *screen;
#endif
#ifdef CASIO_PRIZM
#define LCD_GRAM 0x202
#define LCD_BASE	0xB4000000
#define VRAM_ADDR 0xA8000000
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
		if((*DMA0_DMAOR)&4)//Address error has occured stop looping
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
	*DMA0_SAR_0=VRAM_ADDR&0x1FFFFFFF;//Source address is VRAM
	*DMA0_DAR_0=LCD_BASE&0x1FFFFFFF;//Desination is LCD
	*DMA0_TCR_0=(216*384)/16;//Transfer count bytes/32
	*DMA0_CHCR_0=0x00101400;
	*DMA0_DMAOR|=1;//Enable DMA on all channels
	*DMA0_DMAOR&=~6;//Clear flags
	*DMA0_CHCR_0|=1;//Enable channel0 DMA
}
#endif
void clearScreen(void){
	#ifdef CASIO_PRIZM
		memset((void *)0xA8000000,0,384*216*2);
	#endif
	#ifdef PC
		if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
		memset(screen->pixels,0,screen->w*screen->h*2);
		if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	#endif
}
void toScrTop(unsigned w,unsigned h,uint16_t * c){
	uint16_t * s;
	#ifdef PC
		if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
		s=screen->pixels;
	#endif
	#ifdef CASIO_PRIZM
		s=(uint16_t*)0xA8000000;
	#endif
	unsigned w2=w*2;
	while(h--){
		memcpy(s,c,w2);
		s+=SCREEN_WIDTH;
		c+=w;
	}
	#ifdef PC
		if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	#endif
}
void fillRectScr(unsigned x,unsigned y,uint16_t col,unsigned wr,unsigned hr){
	uint16_t*s;
	#ifdef PC
		if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
		s=screen->pixels;
	#endif
	#ifdef CASIO_PRIZM
		s=(uint16_t*)0xA8000000;
	#endif
	s+=(y*SCREEN_WIDTH)+x;
	while(hr--){
		unsigned w2=wr;
		while(w2--)
			*s++=col;
		s+=SCREEN_WIDTH-wr;
	}
	#ifdef PC
		if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	#endif
}
