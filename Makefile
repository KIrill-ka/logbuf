OFLAGS ?= -O2
CFLAGS=-fPIC -Iinclude -DLINUX -DLP64 -Wall $(OFLAGS)

all: build_dir/liblogstdio.so build_dir/liblogbuf.so

tclext: build_dir/tcllogbuf.so

build_dir/liblogstdio.so: build_dir/liblogdest.so

build_dir/tcllogbuf.so: build_dir/logbuf_tcl.o build_dir/liblogdest.so
	gcc -shared $(LFLAGS) -o $@ $< -Lbuild_dir -llogdest

build_dir/logbuf_tcl.o: tcl/logbuf_tcl.c
	gcc -I /usr/include/tcl8.6 $(CFLAGS) -c $< -o $@

build_dir/liblogbuf.so: build_dir/log_buf.o
	gcc -z defs -shared $(LFLAGS) $< -o $@

build_dir/liblogdest.so: build_dir/log_fmt.o build_dir/log_arg.o build_dir/log_res.o
	gcc -z defs -shared $(LFLAGS) $^ -o $@

build_dir/liblogstdio.so: build_dir/log_stdio.o
	gcc -z defs -shared $(LFLAGS) $< -o $@ -Lbuild_dir -llogdest

build_dir/%.o: %.c
	@mkdir -p build_dir
	gcc $(CFLAGS) $< -c -o $@
clean:
	rm -rf build_dir
