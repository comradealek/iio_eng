src := $(shell echo src/*.c)
srctut := $(shell echo srctut/*.c)
srcglfw := $(shell echo GLFWsrc/*.c)
srcdat := $(shell echo srcdynarrtest/*.c)

objs := $(src:src/%.c=obj/%.o)
tutobjs := $(srctut:srctut/%.c=tutobj/%.o)
glfwobjs := $(srcglfw:GLFWsrc/%.c=glfwobj/%.o)
objdatest := $(srcdat:srcdynarrtest/%.c=objdatest/%.o)

shad := $(shell echo src/shaders/*.glsl)
shadtut := $(shell echo srctut/shaders/*.glsl)

spv := $(shad:src/shaders/%.glsl=src/shaders/%.spv)
spvtut := $(shadtut:srctut/shaders/%.glsl=srctut/shaders/%.spv)

tout := bin/tutorial
mout := bin/main
dout := bin/dynarrtest

libs := -ldl -lm -lrt -lvulkan
includes := -Iinclude
glfwincludes := -IGLFWsrc
cflags := -O3 -g -D_GLFW_WAYLAND

main : $(mout) $(spv)

tutorial : $(tout) $(spvtut)

dynarrtest : $(dout)

$(mout) : $(objs) $(glfwobjs)
	gcc -o $(mout) $(objs) $(glfwobjs) $(libs) $(includes) $(cflags)

$(tout) :  $(tutobjs) $(glfwobjs)
	gcc -o $(tout) $(tutobjs) $(glfwobjs) $(libs) $(includes) $(cflags)

$(dout) : $(objdatest)
	gcc -o $(dout) $(objdatest) $(includes) $(cflags)

obj/%.o : src/%.c
	gcc -c $< -o $@ $(libs) $(includes) $(cflags)

tutobj/%.o : srctut/%.c
	gcc -c $< -o $@ $(libs) $(includes) $(cflags)

glfwobj/%.o : GLFWsrc/%.c
	gcc -c $< -o $@ $(libs) $(glfwincludes) $(cflags)

objdatest/%.o : srcdynarrtest/%.c
	gcc -c $< -o $@ $(includes) $(cflags)

src/shaders/%.spv : src/shaders/%.glsl
	glslc $< -o $@

srctut/shaders/%.spv : srctut/shaders/%.glsl
	glslc $< -o $@

clean : 
	rm $(objs) $(glfwobjs) $(tutobjs) bin/main bin/tutorial $(spvtut)

testreset :
	rm objdatest/*.o bin/dynarrtest