.PHONY all: build
	cd build && ./lcr ../config.json

.PHONY build:
	cd build && cmake .. && make lcr

.PHONY clean:
	rm -rf build