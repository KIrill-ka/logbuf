
CFLAGS= -g -I../include -L../build_dir -Wall -Wl,--rpath=../build_dir

#tests=build_dir/logtest build_dir/logdest-res build_dir/logtest-fmt build_dir/logtest-pref
tests=build_dir/logtest build_dir/logtest-pref

all: $(tests)
build_dir/logtest: logtest.c
	@mkdir -p build_dir
	gcc $(CFLAGS) $< -o $@ -llogbuf -llogdest

build_dir/logtest-pref: logtest-pref.c
	@mkdir -p build_dir
	gcc $(CFLAGS) $< -o $@ -llogbuf -llogdest

clean:
	rm -rf build_dir
