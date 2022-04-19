WindowsServer.exe : WindowsServer.o
	g++ -o WindowsServer.exe WindowsServer.o -lws2_32 -o WindowsServer.exe
WindowsServer.o : WindowsServer.cpp
	g++ -c -O3 -Wall WindowsServer.cpp