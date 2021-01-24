# Ping-Pong Misra Algorithm
Implementation of Ping-Pong algorithm (Misra 1983) for mutal exclusion within ring topology.

Pull newest project version from repository, build and run
``` C
make all
```

Build Ping-Pong Misra Algorithm project
``` C
make build
```
Run project on 3 processes
``` C
make run
```

Push newest version of project
``` C
make publish
```

To run Ping-Pong Algorithm in debug mode set flag -d in Makefile 
``` C
run:
	 mpirun -n 4 ./main -d 
```

To test Ping-Pong Algorithm there are two option. Starting host can fail to send PONG token (-t pong) or can fail to send PING token (-t ping) when sending first tokens.

Test ping regeneration:
``` C
run:
    mpirun -n 4 ./main -d -t ping
```    

Test pong regeneration:
``` C
run:
    mpirun -n 4 ./main -d -t pong
```

## Function description
Function Regenerate is not used in implementation, because when one of the tokens is lost and is regenerate, than host holds both of PING and PONG tokens, which should invoke incarnation function. Instead of using regenerate and later incrementing regenerated tokens values, we use incarnation function to regenerate and increment tokens.

### Regenerate

``` C
void regenerate(int val) {
 	ping = abs(val);
 	pong = -ping;
}
```

### Incarnation

``` C
void incarnation(int val) {
 	ping = abs(val) +1;
 	pong = -ping;
}
```
