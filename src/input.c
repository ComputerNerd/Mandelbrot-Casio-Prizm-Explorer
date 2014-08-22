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
#include "input.h"
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
		const unsigned short* keyboard_register = (unsigned short*)0xA44B0000;
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

