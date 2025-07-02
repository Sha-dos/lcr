.PHONY all: build
	cd build && ./lcr ${CONFIG}

.PHONY build:
	cd build && cmake .. && make lcr

.PHONY clean:
	rm -rf build