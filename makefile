all: get build run

get:
	git pull origin main

build:
	mpicc hello_world_mpi.c -o main

run:
	mpirun -n 4 ./main

publish:
	git add hello_world_mpi.c makefile README.md
	git commit -m "update"
	git push origin main