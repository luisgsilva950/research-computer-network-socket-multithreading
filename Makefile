CFLAGS = -Wall -g
all:
	gcc $(CFLAGS) equipment.c -o equipment
	gcc $(CFLAGS) server.c -o server
test:
	echo "Testing suite. I need to create a testing suite."
#	@bash run_tests.sh 0
#	sleep 5
#	echo "Testing suite 1"
#	@bash run_tests.sh 1
#	sleep 5
#	echo "Testing suite 2"
#	@bash run_tests.sh 2
#	sleep 5
#	echo "Testing suite 3"
#	@bash run_tests.sh 3
