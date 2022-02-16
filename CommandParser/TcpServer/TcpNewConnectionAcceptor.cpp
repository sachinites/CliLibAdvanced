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

#include "TcpServer.h"
#include "TcpNewConnectionAcceptor.h"
#include "TcpClientDbManager.h"
#include "TcpClient.h"
#include "network_utils.h"

TcpNewConnectionAcceptor::TcpNewConnectionAcceptor(TcpServer *TcpServer) {

    this->accept_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (this->accept_fd < 0) {
        printf("Error : Could not create Accept FD\n");
        exit(0);
    }

    this->accept_new_conn_thread = (pthread_t *)calloc(1, sizeof(pthread_t));
    sem_init(&this->wait_for_thread_operation_to_complete, 0, 0);
    pthread_rwlock_init(&this->rwlock, NULL);
    this->accept_new_conn = true;
    this->tcp_server = TcpServer;
}

 TcpNewConnectionAcceptor::~TcpNewConnectionAcceptor() {

        assert (this->accept_fd == 0);
        assert(!this->accept_new_conn_thread);
 }

void
TcpNewConnectionAcceptor::StartTcpNewConnectionAcceptorThreadInternal() {

    int opt = 1;
    bool accept_new_conn;
    bool create_multi_threaded_client;
    struct sockaddr_in server_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(this->tcp_server->port_no);
    server_addr.sin_addr.s_addr = htonl(this->tcp_server->ip_addr);

    if (setsockopt(this->accept_fd, SOL_SOCKET,
                   SO_REUSEADDR, (char *)&opt, sizeof(opt))<0) {
        printf("setsockopt Failed\n");
        exit(0);
    }

    if (setsockopt(this->accept_fd, SOL_SOCKET,
                   SO_REUSEPORT, (char *)&opt, sizeof(opt))<0) {
        printf("setsockopt Failed\n");
       exit(0);
    }

    if (bind(this->accept_fd, (struct sockaddr *)&server_addr,
                sizeof(struct sockaddr)) == -1) {
        printf("Error : Acceptor socket bind failed [%s(0x%x), %d], error = %d\n", 
            network_covert_ip_n_to_p(tcp_server->ip_addr, 0),
            tcp_server->ip_addr,
            tcp_server->port_no, errno);
        exit(0);
    }

    if (listen(this->accept_fd, 5) < 0 ) {

        printf("listen failed\n");
        exit(0);
    }
    
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int comm_socket_fd;

    sem_post (&this->wait_for_thread_operation_to_complete);

    while (true) {

        pthread_testcancel();
        comm_socket_fd =  accept (this->accept_fd,
                                                        (struct sockaddr *)&client_addr, &addr_len);

        if (comm_socket_fd < 0) {
            printf ("Error in Accepting New Connections\n");
            continue;
        }

        pthread_rwlock_rdlock(&this->rwlock);
        accept_new_conn = this->accept_new_conn;
        pthread_rwlock_unlock(&this->rwlock);

        if (!accept_new_conn) {
            close(comm_socket_fd);
            continue;
        }

        /* Send Welcome msg to client */
        TcpClient *tcp_client = new TcpClient();
        tcp_client->comm_fd = comm_socket_fd;
        tcp_client->ip_addr = client_addr.sin_addr.s_addr;
        tcp_client->port_no = client_addr.sin_port;
        tcp_client->tcp_server = this->tcp_server;
        tcp_client->SendMsg((char *)"Remote Login : Welcome\n",
                    strlen("Remote Login : Welcome\n"));
        tcp_client->SetTcpMsgDemarcar(
            TcpMsgDemarcar::InstantiateTcpMsgDemarcar(
                this->tcp_server->msgd_type, 8, 0, 0, 0, 0));
        this->tcp_server->ProcessNewClient (tcp_client);
    }
}

static void *
tcp_listen_for_new_connections(void *arg) {

    TcpNewConnectionAcceptor *tcp_new_conn_acc = 
            (TcpNewConnectionAcceptor *)arg;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, NULL);

    tcp_new_conn_acc->StartTcpNewConnectionAcceptorThreadInternal();
    return NULL;
}

  void
  TcpNewConnectionAcceptor::StartTcpNewConnectionAcceptorThread() {

      pthread_attr_t attr;

      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

      if (pthread_create(this->accept_new_conn_thread, &attr,           tcp_listen_for_new_connections, (void *)this)) {
          printf ("%s() Thread Creation failed, error = %d\n", __FUNCTION__, errno);
          exit(0);
      }
      sem_wait (&this->wait_for_thread_operation_to_complete);
      printf ("Service started : TcpNewConnectionAcceptorThread\n");
  }

void
TcpNewConnectionAcceptor::SetAcceptNewConnectionStatus(bool status) {

    pthread_rwlock_wrlock(&this->rwlock);
    this->accept_new_conn = status;
    pthread_rwlock_unlock(&this->rwlock);
}

void
TcpNewConnectionAcceptor::Stop() {

    this->StopTcpNewConnectionAcceptorThread();
    close(this->accept_fd);
    this->accept_fd = 0;
    sem_destroy(&this->wait_for_thread_operation_to_complete);
    pthread_rwlock_destroy(&this->rwlock);
    delete this;
}

void
TcpNewConnectionAcceptor::StopTcpNewConnectionAcceptorThread() {

    pthread_cancel(*this->accept_new_conn_thread);
    pthread_join(*this->accept_new_conn_thread, NULL);
    free(this->accept_new_conn_thread);
    this->accept_new_conn_thread = NULL;
}