all: clean soft_body


soft_body: soft_body.cpp
	g++ soft_body.cpp -lSDL2 -Wall -Wextra

clean:
	rm -f a.out
