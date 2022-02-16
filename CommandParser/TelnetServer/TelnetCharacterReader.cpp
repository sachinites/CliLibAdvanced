#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "Sequence.h"
#include "LineMgmt.h"

/* References :
    find escape sequences here :
    https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
 */

unsigned char gbuff[32];
extern line_t line[1];

extern int GL_FD_OUT;

extern void
EnhancedParser(int sockfd, char *cli, uint16_t cli_size);

static bool
clear_screen() {

    if (strncmp((const char *)line[0].lbuf, "cls", 3) == 0 &&
         line[0].n == 3) {
        return true;
    }
    return false;
}

static void
ReadSingleCharMsg(int sockfd, unsigned char *msg) {

    int rc = 0;

    printf("1. %c[%d]\n",
           msg[0], msg[0]);

    switch (msg[0]) {
        
        case BACKSPACE:
            /* Empty line*/
            if (line[0].n == 0) return;
            /* only 1 Char in the line */
            else if (line[0].n == 1 && line[0].cpos == 1) {
                assert(line[0].lpos == 0);
                line[0].cpos = 0;
                line[0].n--;
                line[0].lbuf[line[0].lpos] = '\0';
                esc_seq_move_cur_left(sockfd, 1);
                rc = write(sockfd, " ", 1);
                esc_seq_move_cur_left(sockfd, 1);
                return;
            }
            /* At the end of line */
            else if (line[0].cpos == line[0].lpos + 1) {
                line[0].cpos--;
                line[0].lbuf[line[0].lpos] = '\0';
                line[0].lpos--;
                line[0].n--;
                esc_seq_move_cur_left(sockfd, 1);
                rc = write(sockfd, " ", 1);
                esc_seq_move_cur_left(sockfd, 1);
                return;
            }
            /* In the middle of line */
            else if (line[0].cpos > 0 && line[0].cpos <= line[0].lpos) {
                int rem = line[0].lpos - line[0].cpos + 1;
                print_line(&line[0]);
                line_del_charat(&line[0], line[0].cpos - 1);
                print_line(&line[0]);
                line[0].cpos--;
                esc_seq_move_cur_left(sockfd, 1);
                rc = esc_seq_erase_curr_line_from_cur_end(sockfd);
                rc = write(sockfd, (const char *)line[0].lbuf + line[0].cpos, rem);
                esc_seq_move_cur_to_column(sockfd, line[0].cpos +1);
                return;
            }
            break;
        case TAB_KEY:
            rc = write(sockfd, (const char *)msg, 1);
            break;
        case 'Z':
            print_line(&line[0]);
            break;
        case 'a' ... 'z':
        case 'A' ... 'Y':
        case 48 ... 57:
        case SPACE_KEY:
            /* Tying on empty line */
            if (line_is_empty(&line[0])) {
                line_add_character(&line[0], msg[0]);
                line[0].cpos++;
                rc = write(sockfd, (const char *)msg, 1);
            }
            /* Tying at the end of line */
            else if (line->cpos == line->lpos + 1) {
                line_add_character(&line[0], msg[0]);
                line[0].cpos++;
                rc = write(sockfd, (const char *)msg, 1);
            }
            /* Tying in the middle of line */
            else {
                int rem = line[0].lpos - line[0].cpos + 1;
                print_line(&line[0]);
                line_add_character(&line[0], msg[0]);
                print_line(&line[0]);
                line[0].cpos++;
                 rc = write(sockfd, (const char *)line[0].lbuf + line[0].cpos - 1, rem + 1);
                printf ("Setting cursor to on charc %c at col %d\n", line->lbuf[line[0].cpos], line[0].cpos + 1);
                esc_seq_move_cur_to_column(sockfd, line[0].cpos + 1); // column no starts from 1
            }
            break;
            case  FORWARD_SLASH_KEY:
             if (line[0].cpos == line[0].lpos + 1) {
                line_add_character(&line[0], msg[0]);
                line[0].cpos++;
                EnhancedParser(sockfd, (char *)line[0].lbuf, line[0].n);
                line_reinit(&line[0]);
             }
             break;
            case DOT_KEY:
            case QUESTION_KEY:
                if ( (line[0].cpos == line[0].lpos + 1) ||
                      (line[0].n == 0)) {
                    line_add_character(&line[0], msg[0]);
                    line[0].cpos++;
                    EnhancedParser(sockfd, (char *)line[0].lbuf, line[0].n);
                    line_reinit(&line[0]);
                }
                break;
    }
}

static void
ReadDoubleCharMsg(int sockfd, unsigned char *msg) {

    int rc = 0;

    printf ("2. %c[%d] %c[%d]\n", 
        msg[0], msg[0], msg[1], msg[1]);

    switch (msg[0]) {
        case ENTER_KEY:

            /* If the user has typed cls<enter> then send back clear screen
            instruction code to remote client. Dont bother to feed to Parser */
            if (clear_screen()) {
                rc = esc_seq_clear_screen(sockfd);
                rc += write(sockfd, "Press Enter To Continue...", 
                            strlen("Press Enter To Continue.."));
                rc += esc_seq_move_cur_beginning_of_line(sockfd, 1);
                line_reinit(&line[0]);
                return;
            }
            if (line[0].n == 0) {
            /* If only enter is hit, no need to send new line reset,
                The Parser will do it because we need re-write the line
                header*/
            }
            else {
                rc = write(sockfd, "\r\n", 2);
            }
            EnhancedParser(sockfd, (char *)line[0].lbuf, line[0].n);
            line_reinit(&line[0]);
            break;
        default: ;
    }
}

static void
 ReadThreeCharMsg(int sockfd, unsigned char *msg) {

    int rc = 0;

        printf ("3. %c[%d] %c[%d] %c[%d]\n", 
            msg[0], msg[0], msg[1], msg[1], msg[2], msg[2]);

        switch (msg[0]) {

             case '\033':
                switch (msg[1]) {

                    case '[':
                        switch (msg[2]) {
                            case 'A': /* UP Arrow */
                                rc = write(sockfd, (const char *)msg, 3);
                                //rc = write(sockfd, "\033[A", 3);
                                break;
                            case 'B': /* DOWN Arrow */
                                 rc = write(sockfd, (const char *)msg, 3);
                                 //rc = write(sockfd, "\033[B", 3);
                                break;
                            case 'C': /* RIGHT Arrow */
                                if (line[0].cpos != line[0].lpos + 1) {
                                    line[0].cpos++;
                                    //rc = write(sockfd, (const char *)msg, 3);
                                    rc = esc_seq_move_cur_right(sockfd, 1);
                                }
                                 //rc = write(sockfd, "\033[C", 3);
                                break;
                            case 'D': /* LEFT Arrow */
                                if (line[0].cpos > 0) {
                                 line[0].cpos--;
                                 rc = esc_seq_move_cur_left(sockfd, 1);
                                 //rc = write(sockfd, (const char *)msg, 3);
                                }
                                 //rc = write(sockfd, "\033[D", 3);
                                break;
                            case 'H': /* Home Key */
                                rc = esc_seq_move_cur_beginning_of_line(sockfd, 0);
                                line[0].cpos = 0;
                            break;
                            case 'F': /* END key */
                                rc = esc_seq_move_cur_to_column(sockfd, line[0].lpos + 2);
                                line[0].cpos = line[0].lpos + 1;
                            break;
                        }
                }
            default:;
        }
 }

static void
 ReadFourCharMsg(int sockfd, unsigned char *msg) {

    int rc = 0;

    printf("4. %c[%d] %c[%d] %c[%d] %c[%d]\n",
           msg[0], msg[0], msg[1], msg[1], msg[2], msg[2], msg[3], msg[4]);

    switch(msg[0]) {
        case '\033':
            switch(msg[1]) {
                case '[':
                    switch(msg[2]){
                        case '3':
                            switch(msg[3]){
                                case '~':
                                    /* DELETE KEY */
                                    /* Empty line */
                                    if (line[0].n == 0) return;
                                    /* At the end of line */
                                    else if (line[0].lpos + 1  == line[0].cpos) return;
                                     /* In the middle of line */
                                     else if (line[0].cpos >= 0 && line[0].cpos <= line[0].lpos) {
                                        int rem = line[0].lpos - line[0].cpos + 1;
                                        line_del_charat(&line[0], line[0].cpos);
                                        rc = esc_seq_erase_curr_line_from_cur_end(sockfd);
                                        rc = write(sockfd, (const char *)line[0].lbuf + line[0].cpos, rem -1);
                                        esc_seq_move_cur_to_column(sockfd, line[0].cpos +1);
                                     }
                                    break;
                            }
                        case '4':
                            break;
                    }
            }
    }
 }

static void
 ReadLongerCharMsg(int sockfd, unsigned char *msg, uint16_t msg_size) {

     int i;

     for(i = 0; i < msg_size; i++) {
         printf("%d : %c[%d]\n", i, msg[i], msg[i]);
     }

    if (msg[msg_size -1] == 'R') {
        // ^[ [ # ; # R   where # represents row and col number
        int row = 0, col = 0;
        esc_seq_read_cur_pos(msg, msg_size, &row, &col);
        printf ("Row = %d, col = %d\n", row, col);
    }

 }

void
InputCliFromRemoteClient(int sockfd, unsigned char *msg, uint16_t msg_size) {

    switch(msg_size) {
        case 1 :
            ReadSingleCharMsg(sockfd, msg);
            break;
        case 2 :
            ReadDoubleCharMsg(sockfd, msg);
            break;
        case 3 :
            ReadThreeCharMsg(sockfd, msg);
            break;
        case 4 :
            ReadFourCharMsg(sockfd, msg);
            break;
        default:
            ReadLongerCharMsg(sockfd, msg, msg_size);
           ;
    }
    GL_FD_OUT = sockfd;
}
