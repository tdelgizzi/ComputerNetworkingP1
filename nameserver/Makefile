nameserver: nameserver_driver.o nameserver.o
	g++ -std=c++17 -g -o nameserver nameserver_driver.o nameserver.o
nameserver.o: nameserver.cpp nameserver.h
	g++ -std=c++17 -g -c nameserver.cpp
nameserver_driver.o: nameserver_driver.cpp nameserver.h
	g++ -std=c++17 -g -c nameserver_driver.cpp

clean:
	rm -f *.o nameserver
