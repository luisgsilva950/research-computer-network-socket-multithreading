CFLAGS = -Wall -g
all:
	gcc $(CFLAGS) client.c -o client
	gcc $(CFLAGS) server.c -o server
test:
	echo "Testing suite 0"
	@bash run_tests.sh 0
	sleep 5
	echo "Testing suite 1"
	@bash run_tests.sh 1
	sleep 5
	echo "Testing suite 2"
	@bash run_tests.sh 2
	sleep 5
	echo "Testing suite 3"
	@bash run_tests.sh 3
