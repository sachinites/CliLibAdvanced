CC=g++
CFLAGS=-g
TARGET:testapp.exe tcp_client.exe libnw.a
LIBS=-lpthread -lrt
OBJS=TcpClient.o \
			TcpClientDbManager.o  \
			TcpClientServiceManager.o \
			TcpNewConnectionAcceptor.o \
			TcpServer.o \
			network_utils.o	\
			TcpMsgDemarcar.o \
			TcpMsgFixedSizeDemarcar.o \
			ByteCircularBuffer.o \
			TcpMsgVariableSizeDemarcar.o

testapp.exe:testapp.o ${OBJS}
	${CC} ${CFLAGS} ${OBJS} testapp.o -o testapp.exe ${LIBS}

libnw.a:${OBJS}
	ar rs libnw.a ${OBJS}

tcp_client.exe:tcp_client.o
	${CC} ${CFLAGS} tcp_client.o -o tcp_client.exe
	
TcpClient.o:TcpClient.cpp
	${CC} ${CFLAGS} -c TcpClient.cpp -o TcpClient.o

tcp_client.o:tcp_client.cpp
	${CC} ${CFLAGS} -c tcp_client.cpp -o tcp_client.o

testapp.o:testapp.cpp
	${CC} ${CFLAGS} -c testapp.cpp -o testapp.o

TcpClientDbManager.o:TcpClientDbManager.cpp
	${CC} ${CFLAGS} -c TcpClientDbManager.cpp -o TcpClientDbManager.o

TcpClientServiceManager.o:TcpClientServiceManager.cpp
	${CC} ${CFLAGS} -c TcpClientServiceManager.cpp -o TcpClientServiceManager.o

TcpNewConnectionAcceptor.o:TcpNewConnectionAcceptor.cpp
	${CC} ${CFLAGS} -c TcpNewConnectionAcceptor.cpp -o TcpNewConnectionAcceptor.o

TcpServer.o:TcpServer.cpp
	${CC} ${CFLAGS} -c TcpServer.cpp -o TcpServer.o

network_utils.o:network_utils.cpp
	${CC} ${CFLAGS} -c network_utils.cpp -o network_utils.o

TcpMsgFixedSizeDemarcar.o:TcpMsgFixedSizeDemarcar.cpp
	${CC} ${CFLAGS} -c TcpMsgFixedSizeDemarcar.cpp -o TcpMsgFixedSizeDemarcar.o

TcpMsgDemarcar.o:TcpMsgDemarcar.cpp
	${CC} ${CFLAGS} -c TcpMsgDemarcar.cpp -o TcpMsgDemarcar.o

ByteCircularBuffer.o:ByteCircularBuffer.cpp
	${CC} ${CFLAGS} -c ByteCircularBuffer.cpp -o ByteCircularBuffer.o

TcpMsgVariableSizeDemarcar.o:TcpMsgVariableSizeDemarcar.cpp
	${CC} ${CFLAGS} -c TcpMsgVariableSizeDemarcar.cpp -o TcpMsgVariableSizeDemarcar.o
	
clean:
	rm -f *.o
	rm -f *exe
	rm -f libnw.a
