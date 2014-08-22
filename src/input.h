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
void waitKey(void);
int keyPressed(int basic_keycode);
int checkExit(void);
#ifdef CASIO_PRIZM
	#include <fxcg/keyboard.h>
	#define KEY_UP KEY_PRGM_UP
	#define KEY_DOWN KEY_PRGM_DOWN
	#define KEY_LEFT KEY_PRGM_LEFT
	#define KEY_RIGHT KEY_PRGM_RIGHT
	#define KEY_0 KEY_PRGM_0
	#define KEY_1 KEY_PRGM_1
	#define KEY_2 KEY_PRGM_2
	#define KEY_3 KEY_PRGM_3
	#define KEY_4 KEY_PRGM_4
	#define KEY_5 KEY_PRGM_5
	#define KEY_6 KEY_PRGM_6
	#define KEY_7 KEY_PRGM_7
	#define KEY_8 KEY_PRGM_8
	#define KEY_9 KEY_PRGM_9
	#define KEY_F1 KEY_PRGM_F1
	#define KEY_F2 KEY_PRGM_F2
	#define KEY_F3 KEY_PRGM_F3
	#define KEY_F4 KEY_PRGM_F4
	#define KEY_F5 KEY_PRGM_F5
	#define KEY_F6 KEY_PRGM_F6
	#define KEY_FOR_EXIT KEY_PRGM_MENU
	#define KEY_SHIFT KEY_PRGM_SHIFT
	#define KEY_ALPHA KEY_PRGM_ALPHA
#endif
#ifdef PC
	#include <SDL/SDL.h>
	#define KEY_UP SDLK_UP
	#define KEY_DOWN SDLK_DOWN
	#define KEY_LEFT SDLK_LEFT
	#define KEY_RIGHT SDLK_RIGHT
	#define KEY_0 SDLK_0
	#define KEY_1 SDLK_1
	#define KEY_2 SDLK_2
	#define KEY_3 SDLK_3
	#define KEY_4 SDLK_4
	#define KEY_5 SDLK_5
	#define KEY_6 SDLK_6
	#define KEY_7 SDLK_7
	#define KEY_8 SDLK_8
	#define KEY_9 SDLK_9
	#define KEY_F1 SDLK_F1
	#define KEY_F2 SDLK_F2
	#define KEY_F3 SDLK_F3
	#define KEY_F4 SDLK_F4
	#define KEY_F5 SDLK_F5
	#define KEY_F6 SDLK_F6
	#define KEY_FOR_EXIT SDLK_ESCAPE
	#define KEY_SHIFT SDLK_LSHIFT
	#define KEY_ALPHA SDLK_LALT //No such key as alpha is present on keyboards alt is the closest word
#endif
