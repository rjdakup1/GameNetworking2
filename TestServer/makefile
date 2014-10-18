all: main.cpp 
	g++ *.cpp -g -O2 src/*.cpp src/*.c `sdl-config --cflags --libs` -lSDL_image -lSDL_mixer -lSDL_ttf -lSDL_net -Iincludes -o main

clean:
	rm main

run:
	./main

new: clean all