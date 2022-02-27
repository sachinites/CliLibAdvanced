#ifndef __ClientMgmt__
#define __ClientMgmt__

#include <stdint.h>
#include "../cliconst.h"
#include "../gluethread/glthread.h"

typedef struct line_ line_t;
typedef struct line_db_ line_db_t;
typedef struct _param_t_ param_t;
typedef struct cli_exec_control_params_ cli_exec_control_params_t;

typedef struct client_profile_ {

    uint32_t ip_addr; // key
    uint16_t port_no; // key
    int sockfd; // secondary key
    line_t *cline;
    line_db_t *linedb;
    cli_exec_control_params_t *cli_params;
    param_t *cli_root;
    char console_name[TERMINAL_NAME_SIZE];
    glthread_t glue; 
} client_profile_t;
GLTHREAD_TO_STRUCT(glue_to_client_profile, client_profile_t, glue);

client_profile_t *
client_profile_lookup(uint32_t ip_addr, uint16_t portno);

client_profile_t *
client_profile_lookup_by_sockfd(uint16_t sockfd);

void
client_profile_delete(uint32_t ip_addr, uint16_t portno); 

void
client_profile_delete_by_sockfd(uint16_t sockfd);

void
client_profile_create_new(uint32_t ip_addr, uint16_t portno, uint16_t sockfd);

void
client_profile_delete(client_profile_t *client_profile);

#endif
