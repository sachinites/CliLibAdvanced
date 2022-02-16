#ifndef __TcpClientDbManager__
#define __TcpClientDbManager__

#include <list>
#include <semaphore.h>
#include <stdint.h>
#include <pthread.h>
#include <vector>

class TcpClient;
class TcpServer;
class TcpClientDbManager {

    private:
        pthread_rwlock_t rwlock;
        std::list<TcpClient *> tcp_client_db;

    public:
        TcpServer *tcp_server;  /* Back pointer to owning Server */
        TcpClientDbManager(TcpServer *);
        ~TcpClientDbManager();
        
        /* Client DB mgmt functions */
        void Purge();        
        void AddClientToDB(TcpClient *);
        void RemoveClientFromDB(TcpClient *);
        TcpClient * RemoveClientFromDB(uint32_t ip_addr, uint16_t port_no);
        void UpdateClient(TcpClient *);
        TcpClient * LookUpClientDB(uint32_t, uint16_t);
        TcpClient *LookUpClientDB_ThreadSafe(uint32_t ip_addr, uint16_t port_no);
        void DisplayClientDb();
};

#endif 