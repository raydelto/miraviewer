UNAME_S := $(shell uname -s)
LIBS = -L. \
	   -L./common/mingw32/lib \
      -lglew32 \
	  -lglfw3 \
	  -lopengl32 \
	  -lgdi32 \
	  -lassimp-5

INCLUDES = -I./common/includes \
		   -I./headers \
		   -I./common/includes/imgui \
		   -I./common/includes/imgui/backends

OBJ = Texture2D.o \
	ShaderProgram.o \
	Mesh.o \
	Camera.o \
	common/includes/imgui/imgui.o \
	common/includes/imgui/imgui_demo.o \
	common/includes/imgui/imgui_draw.o \
	common/includes/imgui/imgui_tables.o \
	common/includes/imgui/imgui_widgets.o \
	common/includes/imgui/backends/imgui_impl_glfw.o \
	common/includes/imgui/backends/imgui_impl_opengl3.o \
	common/includes/ImGuiFileDialog/ImGuiFileDialog.o

WARNINGS=-w

FLAGS=-std=c++17

ifeq ($(UNAME_S),Darwin)
FRAMEWORKS=-framework OpenGL

LIBS= -L/opt/homebrew/opt/glfw/lib \
	  -L/opt/homebrew/opt/assimp/lib \
	  -lglfw \
	  -lassimp

INCLUDES=-I./headers \
		-I/opt/homebrew/opt/glfw/include \
		-I./common/includes \
		-I/usr/local/include \
		-I./common/includes/imgui \
		-I./common/includes/imgui/backends

OBJ+=glad.o

all: main

glad.o: third-party-source-code/glad.c
	gcc -c third-party-source-code/glad.c $(INCLUDES) $(WARNINGS)

main: src/main.cpp $(OBJ)
	g++ src/main.cpp $(OBJ) $(LIBS) $(INCLUDES) -o main $(WARNINGS) $(FLAGS)

clean:
	rm -f *.o
	rm -f main
else

all: main.exe

main.exe: src/main.cpp $(OBJ)
	g++ src/main.cpp $(OBJ) $(LIBS) $(INCLUDES) -o main.exe $(WARNINGS) $(FLAGS)

clean:
	del *.o
	del main.exe
endif

Texture2D.o: src/Texture2D.cpp headers/Texture2D.h
	g++ -c src/Texture2D.cpp $(INCLUDES) $(WARNINGS) $(FLAGS)

ShaderProgram.o: src/ShaderProgram.cpp headers/ShaderProgram.h
	g++ -c src/ShaderProgram.cpp $(INCLUDES) $(WARNINGS) $(FLAGS)

Mesh.o: src/Mesh.cpp headers/Mesh.h
	g++ -c src/Mesh.cpp $(INCLUDES) $(WARNINGS) $(FLAGS)

Camera.o: src/Camera.cpp headers/Camera.h
	g++ -c src/Camera.cpp $(INCLUDES) $(WARNINGS) $(FLAGS)

%.o: %.cpp
	g++ -c $< -o $@ $(INCLUDES) $(WARNINGS) $(FLAGS)
