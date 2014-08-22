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
#include <stdio.h>
#include <stdint.h>
#include "mandelbrot.h"
int64_t stepX;
int64_t stepY;
int64_t divX;
int64_t divY;
static inline fixed_t absf(fixed_t x){return x>0?x:-x;}
static inline fixed_t square(fixed_t x){return (x*x)>>preMan;}
static inline fixed_t squaredeep(fixed_t x){return (fixed_t)(((int64_t)x*(int64_t)x)>>deepMan);}
static uint16_t colorTab[65536];
void calcColorTab(uint16_t maxit){
	uint_fast32_t it;
	for(it=0;it<=maxit;++it)
		colorTab[it]=it*0xFFFF/maxit;
}
static uint16_t ManItDeep(fixed_t c_r,fixed_t c_i,uint16_t maxit){//manIt stands for mandelbrot iteration what did you think it stood for?
	//c_r = scaled x coordinate of pixel (must be scaled to lie somewhere in the mandelbrot X scale (-2.5, 1)
	//c_i = scaled y coordinate of pixel (must be scaled to lie somewhere in the mandelbrot Y scale (-1, 1)
	// squre optimaztion code below orgionally from http://randomascii.wordpress.com/2011/08/13/faster-fractals-through-algebra/
	//early bailout code from http://locklessinc.com/articles/mandelbrot/
	//changed by me to use fixed point math
	fixed_t ckr,cki;
	unsigned p=0,ptot=8;
	fixed_t z_r = c_r;
	fixed_t z_i = c_i;
	fixed_t zrsqr = squaredeep(z_r);
	fixed_t zisqr = squaredeep(z_i);
	fixed_t q=squaredeep(c_r-((1<<deepMan)/4))+squaredeep(c_i);
	if ((((int64_t)q*((int64_t)q+((int64_t)c_r-((1<<deepMan)/4))))>>deepMan) < (squaredeep(c_i)/4))
		return 0;
	//int zrsqr,zisqr;
	do{
		ckr = z_r;
		cki = z_i;
		ptot += ptot;
		if (ptot > maxit) ptot = maxit;
		for (; p < ptot;++p){
			z_i =(squaredeep(z_r+z_i))-zrsqr-zisqr;
			z_i += c_i;
			z_r = zrsqr-zisqr+c_r;
			zrsqr = squaredeep(z_r);
			zisqr = squaredeep(z_i);
			if ((zrsqr + zisqr) > (4<<deepMan))
				return colorTab[p];
			if ((z_r == ckr) && (z_i == cki))
				return 0;
		}
	} while (ptot != maxit);
	//return (uint32_t)p*(uint32_t)0xFFFF/(uint32_t)maxit;
	return 0;
}
static uint16_t ManIt(fixed_t c_r,fixed_t c_i,uint16_t maxit){
	//same credits as above
	fixed_t ckr,cki;
	unsigned p=0,ptot=8;
	fixed_t z_r = c_r;
	fixed_t z_i = c_i;
	fixed_t zrsqr = (z_r * z_r)>>preMan;
	fixed_t zisqr = (z_i * z_i)>>preMan;
	fixed_t q=square(c_r-((1<<preMan)/4))+square(c_i);
	if (((q*(q+(c_r-((1<<preMan)/4))))>>preMan) < (square(c_i)/4))
		return 0;
	do{
		ckr = z_r;
		cki = z_i;
		ptot += ptot;
		if (ptot > maxit) ptot = maxit;
		for (; p < ptot;++p){
			z_i =(square(z_r+z_i))-zrsqr-zisqr;
			z_i += c_i;
			z_r = zrsqr-zisqr+c_r;
			zrsqr = square(z_r);
			zisqr = square(z_i);
			if ((zrsqr + zisqr) > (4<<preMan))
				return colorTab[p];
			if ((z_r == ckr) && (z_i == cki))
				return 0;
		}
	} while (ptot != maxit);
	return 0;
}
void mandel(uint16_t*dst,unsigned w,unsigned h,int deep,uint16_t maxit,int64_t minX,int64_t maxX,uint16_t poffX,int64_t minY,int64_t maxY,uint16_t poffY){
	int64_t x,y;
	unsigned xx,yy=poffY;
	dst+=(poffY*w);
	if(deep){
		for (y=minY;y<maxY;y+=stepY){
			dst+=poffX;
			xx=poffX;
			if(yy>=h)
				return;
			for (x=minX;x<maxX;x+=stepX){
				if(xx>=w)
					break;
				*dst++=ManItDeep((int64_t)x>>(int64_t)WDifdeep,(int64_t)y>>(int64_t)WDifdeep,maxit);
				++xx;
			}
			++yy;
			dst+=w-xx;
		}
	}else{
		for (y=minY;y<maxY;y+=stepY){
			dst+=poffX;
			xx=poffX;
			if(yy>=h)
				return;
			for (x=minX;x<maxX;x+=stepX){
				if (xx>=w)
					break;
				*dst++=ManIt((int64_t)x>>(int64_t)WDifpre,(int64_t)y>>(int64_t)WDifpre,maxit);
				++xx;
			}
			++yy;
			dst+=w-xx;
		}
	}
}
void mandelMask(uint16_t*dst,unsigned w,unsigned h,int deep,uint16_t maxit,int64_t minX,int64_t maxX,uint16_t poffX,int64_t minY,int64_t maxY,uint16_t poffY,unsigned skipX,unsigned skipY,unsigned mask){
	int64_t x,y;
	unsigned xx,yy=poffY;
	dst+=(poffY*w);
	if(deep){
		for (y=minY;y<maxY;y+=stepY){
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
		for (y=minY;y<maxY;y+=stepY){
			dst+=poffX;
			xx=poffX;
			if(yy>=h)
				return;
			if((yy&mask)==skipY){
				for (x=minX;x<maxX;x+=stepX){
					if (xx>=w)
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
void mandelQuater(uint16_t*dst,unsigned w,unsigned h,int deep,uint16_t maxit,int64_t minX,int64_t maxX,uint16_t poffX,int64_t minY,int64_t maxY,uint16_t poffY){
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
			memcpy(dst,dst-w,w*2);
			dst+=w;
			memcpy(dst,dst-w,w*2);
			dst+=w;
			memcpy(dst,dst-w,w*2);
			dst+=w;
			yy+=4;
		}
	}else{
		for (y=minY;y<maxY;y+=stepY*4){
			dst+=poffX;
			xx=poffX;
			if(yy>=h)
				return;
			for (x=minX;x<maxX;x+=stepX*4){
				if (xx>=w)
					break;
				uint16_t val=ManIt((int64_t)x>>(int64_t)WDifpre,(int64_t)y>>(int64_t)WDifpre,maxit);
				*dst++=val;
				*dst++=val;
				*dst++=val;
				*dst++=val;
				xx+=4;
			}
			dst+=w-xx;
			memcpy(dst,dst-w,w*2);
			dst+=w;
			memcpy(dst,dst-w,w*2);
			dst+=w;
			memcpy(dst,dst-w,w*2);
			dst+=w;
			yy+=4;
		}
	}
}
