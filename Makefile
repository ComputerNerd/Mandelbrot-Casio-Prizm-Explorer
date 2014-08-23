CFLAGS=-Wall -Wextra -Isrc -O3 -flto -DPC -fmerge-all-constants -fsingle-precision-constant -s -Wl,--gc-sections -pipe
objects = src/main.o
mandelbort: $(objects)
	cc -Wall -o mandelbrot -lSDL -lm $(CFLAGS) $(objects)

%.o: %.cpp
	cc $(CFLAGS) -c $< -o $@
%.o: %.c
	cc $(CFLAGS) -c $< -o $@
clean:
	rm -f mandelbrot $(objects)
