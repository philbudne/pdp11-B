all:
	make -C tools/apout
	make -C tools/das
	make -C tools/disaout
	make -C source/brt
	make -C source/b711
	make -C source/libb
	make -C source/bilib

hello:	all
	-cd source/b711; ./run.sh hello.b

checklib: all
	./build

clean:
	make -C tools/apout clean
	make -C tools/das clean
	make -C tools/disaout clean
	make -C source/brt clean
	make -C source/b711 clean
	make -C source/libb clean
	make -C source/bilib clean
	rm -rf bilib libb
