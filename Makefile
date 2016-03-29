
CFLAGS=-fPIC -Iinclude -DLINUX -DLP64 -Wall

all: build_dir/liblogstdio.so build_dir/liblogbuf.so

build_dir/liblogstdio.so: build_dir/liblogdest.so

build_dir/liblogbuf.so: build_dir/log_buf.o
	gcc -z defs -shared $< -o $@

build_dir/liblogdest.so: build_dir/log_fmt.o build_dir/log_arg.o build_dir/log_res.o
	gcc -z defs -shared $^ -o $@

build_dir/liblogstdio.so: build_dir/log_stdio.o
	gcc -z defs -shared $< -o $@ -Lbuild_dir -llogdest

build_dir/%.o: %.c
	@mkdir -p build_dir
	gcc $(CFLAGS) $< -c -o $@
clean:
	rm -rf build_dir
