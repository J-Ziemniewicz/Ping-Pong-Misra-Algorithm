all: get build run

get:
	git pull origin main

build:
	mpicc ping_pong.c -o main

run:
	mpirun -n 4 ./main 

publish:
	git add ping_pong.c makefile README.md
	git commit -m "update"
	git push origin main