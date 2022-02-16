#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> // for IPPROTO_TCP
#include <unistd.h>
#include <memory.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "TcpClient.h"
#include "TcpClientServiceManager.h"
#include "TcpServer.h"
#include "network_utils.h"

TcpClient::TcpClient(uint32_t ip_addr, uint16_t port_no) {

    this->ip_addr = ip_addr;
    this->port_no = port_no;
    sem_init(&this->wait_for_thread_operation_to_complete, 0, 0);
    this->ref_count = 0;
}

TcpClient::TcpClient() {

    this->ip_addr = 0;
    this->port_no = 0;
    sem_init(&this->wait_for_thread_operation_to_complete, 0, 0);
    this->ref_count = 0;
}

TcpClient::TcpClient(TcpClient *tcp_client) {

    this->comm_fd = tcp_client->comm_fd;
    this->ip_addr = tcp_client->ip_addr;
    this->port_no = tcp_client->port_no;
    sem_init(&this->wait_for_thread_operation_to_complete, 0, 0);
    this->ref_count = 0;
}

TcpClient::~TcpClient() {

    assert(!this->comm_fd);
    assert(!this->client_thread);
    assert(!this->ref_count);
    assert(!this->msgd);
    printf ("Client Deleted : ");
    this->trace();
}

int
TcpClient::SendMsg(char *msg, uint32_t msg_size) const {

    if (this->comm_fd == 0) return -1;

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(this->tcp_server->port_no);
    dest.sin_addr.s_addr = htonl(this->ip_addr);
    int rc = sendto(this->comm_fd, msg, msg_size, 0, 
	     (struct sockaddr *)&dest, sizeof(struct sockaddr));
    if (rc < 0) {
        printf ("sendto failed\n");
    }
    return rc;
}

void
TcpClient::Abort() {

    if (this->client_thread) {
        this->StopThread();
    }
    if (this->comm_fd) {
        close(this->comm_fd);
        this->comm_fd = 0;
    }
    sem_destroy(&this->wait_for_thread_operation_to_complete);
    this->tcp_server = NULL;

    if (this->msgd) {
        this->msgd->Destroy();
        delete this->msgd;
        this->msgd = NULL;
    }

    delete this;
}

void
TcpClient::Dereference() {

    assert(ref_count);
    ref_count--;
}

void
TcpClient::Reference() {

    ref_count++;
}

void
TcpClient::ClientThreadFunction() {

    int rcv_bytes;
    struct sockaddr_in client_addr;

    socklen_t addr_len = sizeof(client_addr);

    this->tcp_server->client_connected(this);

    while (1) {
        pthread_testcancel();

        rcv_bytes = recvfrom(this->comm_fd,
                             this->recv_buffer,
                             sizeof(this->recv_buffer),
                             0,
                             (struct sockaddr *)&client_addr, &addr_len);

        if (rcv_bytes == 0 || rcv_bytes == 65535 || rcv_bytes < 0) {
            this->tcp_server->client_disconnected(this);
            this->tcp_server->RemoveClientFromDB(this);
            free(this->client_thread);
            this->client_thread = NULL;
            this->Dereference();
            this->Abort();
            pthread_exit(0);
        }
        else if (this->msgd) {
            this->msgd->ProcessMsg(this, this->recv_buffer, rcv_bytes);
        }
        else if (this->tcp_server->client_msg_recvd) {
            this->tcp_server->client_msg_recvd(this, this->recv_buffer, rcv_bytes);
        }
     }
}

static void *
client_listening_thread(void *arg) {

    TcpClient *tcp_client = (TcpClient *)arg;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    /* This TcpClient object is being operated by this thread */
    tcp_client->Reference();
    sem_post(&tcp_client->wait_for_thread_operation_to_complete);

    tcp_client->ClientThreadFunction();
    return NULL;
}

void
TcpClient::StartThread() {

    pthread_attr_t attr;

    assert (!this->client_thread );
    this->client_thread = (pthread_t *)calloc(1, sizeof(pthread_t));
    pthread_attr_init(&attr);

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(this->client_thread, &attr, client_listening_thread, (void *)this);
    sem_wait(&this->wait_for_thread_operation_to_complete);
}

void
TcpClient::StopThread() {

    assert(this->client_thread);
    pthread_cancel(*(this->client_thread));
    pthread_join(*(this->client_thread), NULL);
    free(this->client_thread);
    this->client_thread = NULL;
    this->Dereference();
    this->tcp_server->client_disconnected(this);
}

void 
TcpClient::trace() {

    printf ("[%s , %d]\n", 
        network_covert_ip_n_to_p(htonl(this->ip_addr), 0),
        htons(this->port_no));
}

void 
TcpClient::Display() {

    trace();
}

void 
TcpClient::SetTcpMsgDemarcar(TcpMsgDemarcar  *msgd) {
    
    this->msgd = msgd;
}