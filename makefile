objects := yyjson.o main.o model_loader.o
glfw := cocoa_time.o context.o egl_context.o glx_context.o init.o input.o linux_joystick.o monitor.o null_init.o null_joystick.o null_monitor.o null_window.o osmesa_context.o platform.o posix_module.o posix_poll.o posix_thread.o posix_time.o vulkan.o wgl_context.o win32_init.o win32_joystick.o win32_module.o win32_monitor.o win32_thread.o win32_time.o win32_window.o window.o wl_init.o wl_monitor.o wl_window.o x11_init.o x11_monitor.o x11_window.o xkb_unicode.o
libs :=
includes := -Iinclude
glfwincludes := -IGLFWsrc
cflags := -O3 -g -lm

main : $(objects) $(glfw)
	gcc -o bin/main $(objects) $(glfw) $(libs) $(includes) $(cflags)

$(objects) : %.o : src/%.c
	gcc -c $^ $(libs) $(includes) $(cflags)


$(glfw) : %.o : GLFWsrc/%.c
	gcc -c $^ $(libs) $(glfwincludes) $(cflags) -D_GLFW_WAYLAND