all: sample2D

sample2D: Sample_GL3_2D.cpp glad.c
#	g++ `pkg-config --cflags glfw3` -o try try.cpp glad.c `pkg-config --static --libs glfw3` 

	g++ -o final bfinal.cpp glad.c -lGL -lglfw -lftgl -lSOIL -ldl -I/usr/local/include -I/usr/local/include/freetype2 -L/usr/local/lib 
#	g++ `pkg-config --cflags glfw3` -o sample2D Sample_GL3_2D.cpp glad.c `pkg-config --static --libs glfw3`
clean:
	rm sample2D
