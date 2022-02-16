#ifndef __TCP_CLIENT__
#define __TCP_CLIENT__

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_CLIENT_BUFFER_SIZE 1024

class TcpClientServiceManager;
class TcpServer;
class TcpMsgDemarcar;

class TcpClient {

    private:
        pthread_rwlock_t rwlock;
    public :
        uint32_t ip_addr;
        uint16_t port_no;
        int comm_fd;
        int ref_count;
        unsigned char recv_buffer[MAX_CLIENT_BUFFER_SIZE];
        pthread_t *client_thread;
        TcpServer *tcp_server;
        sem_t wait_for_thread_operation_to_complete;
        TcpMsgDemarcar *msgd;
        
        TcpClient(uint32_t, uint16_t);
        TcpClient();
        TcpClient (TcpClient *);
        ~TcpClient();
	    int SendMsg(char *, uint32_t) const;
        void StartThread();
        void StopThread();
        void ClientThreadFunction();
        void trace();
        void Dereference();
        void Reference();
        void Abort();
        void Display();
        void SetTcpMsgDemarcar(TcpMsgDemarcar  *);
};

#endif
