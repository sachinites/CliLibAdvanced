#include <assert.h>
#include "TcpMsgDemarcar.h"
#include "TcpClient.h"
#include "TcpServer.h"
#include "ByteCircularBuffer.h"
#include "TcpMsgVariableSizeDemarcar.h"

TcpMsgVariableSizeDemarcar::TcpMsgVariableSizeDemarcar () :

    TcpMsgDemarcar (DEFAULT_CBC_SIZE) {
}

TcpMsgVariableSizeDemarcar::~TcpMsgVariableSizeDemarcar() {

}

void
TcpMsgVariableSizeDemarcar::Destroy() {

}

bool 
TcpMsgVariableSizeDemarcar::IsBufferReadyToflush() {

    uint16_t msg_size;
   ByteCircularBuffer_t *bcb = this->TcpMsgDemarcar::bcb;
   
   if (bcb->current_size <= sizeof(uint16_t)) return false;

    BCBRead(bcb, (unsigned char *)&msg_size, sizeof(uint16_t), false);

    if (msg_size <= bcb->current_size) return true;
    
    return false;
}

void
TcpMsgVariableSizeDemarcar::NotifyMsgToClient(TcpClient *tcp_client) {

    uint16_t msg_size;
    ByteCircularBuffer_t *bcb = this->TcpMsgDemarcar::bcb;

    while (this->IsBufferReadyToflush()) {

        BCBRead(bcb, (unsigned char *)&msg_size, sizeof(uint16_t), false);
        assert(msg_size == BCBRead(bcb, this->TcpMsgDemarcar::buffer, msg_size, true));

        tcp_client->tcp_server->client_msg_recvd(tcp_client,  this->TcpMsgDemarcar::buffer, msg_size);
    }
}