.PHONY run:
	cd build && cmake .. && make lcr && ./lcr ${CONFIG}