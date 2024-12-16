objects := yyjson.o main.o
libs :=
includes := -Iinclude
cflags := -O3 -g

main : $(objects)
	gcc -o bin/main $(objects) $(libs) $(includes) $(cflags)

$(objects) : %.o : src/%.c
	gcc -c $^ $(libs) $(includes) $(cflags)
