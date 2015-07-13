LIBS = -L  -lm -lpthread
OBJS = ../source/game.o 
SRCS = ../source/game.cpp 
all:game
game: ${OBJS}
	g++ -o ../target/game ${OBJS}  ${LIBS} 
game.o: ${SRCS}
	g++ -c ${SRCS} 

clean:
	rm -r ../source/*.o ../target/game
