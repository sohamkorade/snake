all:
	g++ main.cpp -o snake -lncurses -O2

clean:
	rm -f snake