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
typedef int fixed_t;
#define preMan 13
#define deepMan 26
#define deepManInt (32-deepMan)
#define difShift (deepMan-preMan)
#define wBits 48
#define WDifpre (wBits-preMan)
#define WDifdeep (wBits-deepMan)
static inline int64_t absll(int64_t x){return x>0?x:-x;}
void calcColorTab(uint16_t maxit);
void mandel(uint16_t*dst,unsigned w,unsigned h,int deep,uint16_t maxit,int64_t minX,int64_t maxX,uint16_t poffX,int64_t minY,int64_t maxY,uint16_t poffY);
void mandelMask(uint16_t*dst,unsigned w,unsigned h,int deep,uint16_t maxit,int64_t minX,int64_t maxX,uint16_t poffX,int64_t minY,int64_t maxY,uint16_t poffY,unsigned skipX,unsigned skipY,unsigned mask);
void mandelQuater(uint16_t*dst,unsigned w,unsigned h,int deep,uint16_t maxit,int64_t minX,int64_t maxX,uint16_t poffX,int64_t minY,int64_t maxY,uint16_t poffY);
extern int64_t stepX;
extern int64_t stepY;
extern int64_t divX;
extern int64_t divY;
