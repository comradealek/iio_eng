src := $(shell echo src/*.c)
srctut := $(shell echo srctut/*.c)
srcglfw := $(shell echo GLFWsrc/*.c)

objs := $(src:src/%.c=obj/%.o)
tutobjs := $(srctut:srctut/%.c=tutobj/%.o)
glfwobjs := $(srcglfw:GLFWsrc/%.c=glfwobj/%.o)

tout := bin/tutorial
mout := bin/main

libs := -ldl
includes := -Iinclude
glfwincludes := -IGLFWsrc
cflags := -O0 -g -lm -D_GLFW_WAYLAND

main : $(mout)

tutorial : $(tout)

$(mout) : $(objs) $(glfwobjs)
	gcc -o $(mout) $(objs) $(glfwobjs) $(libs) $(includes) $(cflags)

$(tout) :  $(tutobjs) $(glfwobjs)
	gcc -o $(tout) $(tutobjs) $(glfwobjs) $(libs) $(includes) $(cflags)

obj/%.o : src/%.c
	gcc -c $< -o $@ $(libs) $(includes) $(cflags)

tutobj/%.o : srctut/%.c
	gcc -c $< -o $@ $(libs) $(includes) $(cflags)

glfwobj/%.o : GLFWsrc/%.c
	gcc -c $< -o $@ $(libs) $(glfwincludes) $(cflags)

clean : 
	rm $(objs) $(glfwobjs) $(tutobjs) bin/main bin/tutorial