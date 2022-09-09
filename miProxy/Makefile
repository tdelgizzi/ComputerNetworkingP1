miProxy: miProxy_driver.o miProxy.o
	g++ -std=c++17 -g -o miProxy miProxy_driver.o miProxy.o
miProxy.o: miProxy.cpp miProxy.h
	g++ -std=c++17 -g -c miProxy.cpp
miProxy_driver.o: miProxy_driver.cpp miProxy.h
	g++ -std=c++17 -g -c miProxy_driver.cpp

clean:
	rm -f *.o miProxy
