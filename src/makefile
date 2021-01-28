CXX := g++ -std=c++11 -g
LIBS := -lpthread
MV := mv 

target := huhu_server
sources := ${wildcard *.cpp}
headers := ../include/${wildcard *.h}
objects := ${patsubst %.cpp, %.o, ${sources}}


${target}: ${objects}
	${CXX} -o $@ $^ ${LIBS} 


.PHONY: clean cleanall

cheanall:
	${MV} ${target} ${objects} ../test

clean:
	${MV} ${objects} ../test

test:
	echo ${objects}