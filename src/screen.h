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
#ifdef PC
#include <SDL/SDL.h>
#endif
#ifdef PC
#define FlipScreen() SDL_Flip(screen);
#define VRAM_ADDRESS screen->pixels
#endif
#ifdef CASIO_PRIZM
#define VRAM_ADDRESS 0xA8000000
void DmaWaitNext(void);
void FlipScreen(void);
#endif
