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

static void
ProcessNormalKeyPress(int sockfd, line_t *line, char c) {

    if (line_is_empty(line))
    {
        line_add_character(line, c);
        line->cpos++;
        write(sockfd, (const char *)&c, 1);
    }
    /* Tying at the end of line */
    else if (line->cpos == line->lpos + 1)
    {
        line_add_character(line, c);
        line->cpos++;
        write(sockfd, (const char *)&c, 1);
    }
    /* Tying in the middle of line */
    else
    {
        int rem = line->lpos - line->cpos + 1;
        print_line(line);
        line_add_character(line, c);
        print_line(line);
        line->cpos++;
        write(sockfd, (const char *)line->lbuf + line->cpos - 1, rem + 1);
        printf("Setting cursor to on charc %c at col %d\n", line->lbuf[line->cpos], line->cpos + 1);
        esc_seq_move_cur_left(sockfd, rem);
    }
}

static void
ReadSingleCharMsg(int sockfd, unsigned char *msg) {

    int rc = 0;

    printf("1. %c[%d]\n",
           msg[0], msg[0]);

    switch (msg[0]) {
        
        case LINE_FEED_KEY:
        case CARRIAGE_KEY:
            if (line[0].n) {
                rc = write(sockfd, "\r\n", 2);
            }
            EnhancedParser(sockfd, (char *)line[0].lbuf, line[0].n);
            if (line[0].n) line_reinit(&line[0]);
            break;
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
            esc_seq_move_cur_right(sockfd, TAB_SIZE);
            line_save_cpos(&line[0]);
            line[0].lbuf[line[0].cpos] = '\0';
            if (line_is_empty(&line[0])) {
                line[0].lpos += TAB_SIZE -1;
            }
            else{
                line[0].lpos += TAB_SIZE;
            }
            line[0].n += TAB_SIZE;
            line[0].cpos += TAB_SIZE;
            /*Save Tab in Line, because \0 will create problem as it is non-writable character*/
            strncpy((char *)&line[0].lbuf[line[0].saved_pos], TAB_SPACE,  TAB_SIZE);
            break;
        case 'Z':
            print_line(&line[0]);
            break;
        case 'a' ... 'z':
        case 'A' ... 'Y':
        case 48 ... 57:
        case SPACE_KEY:
        case '_':
             ProcessNormalKeyPress(sockfd, &line[0], msg[0]);
             break;
            case DOT_KEY:
            case QUESTION_KEY:
                if (((line[0].cpos == line[0].lpos + 1) && 
                        (line[0].lbuf[line[0].lpos] == ' ')) ||
                        (line[0].n == 0)) {
                    line_add_character(&line[0], msg[0]);
                    line[0].cpos++;
                    EnhancedParser(sockfd, (char *)line[0].lbuf, line[0].n);
                    line[0].cpos--;
                    line[0].lbuf[line[0].lpos] = '\0';
                    if (line[0].lpos) line[0].lpos--;  
                    line[0].n--;
                    line_rewrite(sockfd, &line[0]);
                }
                else {
                    /* Treat ? and . as normal keys */
                    ProcessNormalKeyPress(sockfd, &line[0], msg[0]);
                }
                break;
            case CTRL_L_KEY:
               line_reinit(&line[0]);
               strncpy((char *)&line[0].lbuf[line[0].cpos] , "cls", 3);
               line[0].lpos = 2;
               line[0].cpos = 3;
               line[0].n = 3;
               EnhancedParser(sockfd, (char *)line[0].lbuf, line[0].n);
               line[0].lbuf[0] = '\0'; line[0].lbuf[1] = '\0'; line[0].lbuf[2] = '\0';
               line[0].lpos = 0; line[0].cpos = 0; line[0].n = 0;
               break;
            case SINGLE_UPPER_COMMA_KEY:
                if (line_is_empty(&line[0])) return;
                /* Already at the end of line */
                if (line[0].cpos == line[0].lpos + 1) return;
                rc = esc_seq_erase_curr_line_from_cur_end(sockfd);
                line[0].n -= line[0].lpos - line[0].cpos +1;
                /* Do not update lpos if already at the beginning of line */
                line[0].lpos =  line[0].cpos ?  line[0].cpos - 1 : 0;
                break;
            case FORWARD_SLASH_KEY:
                if (((line[0].cpos == line[0].lpos + 1) && 
                        (line[0].lbuf[line[0].lpos] == ' ') &&
                        line[0].n)) {
                    line_add_character(&line[0], msg[0]);
                    line[0].cpos++;
                    EnhancedParser(sockfd, (char *)line[0].lbuf, line[0].n);
                    line_reinit(&line[0]);
                }
                else {
                    /* Treat ? and . as normal keys */
                    ProcessNormalKeyPress(sockfd, &line[0], msg[0]);
                }
                break;
            default:;
    }
}

/* New line Character 
 On unix Systems , New line character is \n ( 1 B ) i.e. 10
 On windows, new line is \rnull (2B) i.e. 13 and 0
 Also, on some systems, or test file, new line character is \r\n (2B) i.e. 13 and 10
*/
static void
ReadDoubleCharMsg(int sockfd, unsigned char *msg) {

    int rc = 0;

    printf ("2. %c[%d] %c[%d]\n", 
        msg[0], msg[0], msg[1], msg[1]);

    switch(msg[0]) {
        case CARRIAGE_KEY:
            switch(msg[1]){
                case NULL_KEY:
                case LINE_FEED_KEY:
                    if (line[0].n) {
                        rc = write(sockfd, "\r\n", 2);
                    }
                    EnhancedParser(sockfd, (char *)line[0].lbuf, line[0].n);
                    if (line[0].n) line_reinit(&line[0]);
                    break;
            }
            break;
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
                                break;
                            case 'B': /* DOWN Arrow */
                                 rc = write(sockfd, (const char *)msg, 3);
                                break;
                            case 'C': /* RIGHT Arrow */
                                if (line_is_empty(&line[0])) return;
                                if (line[0].cpos != line[0].lpos + 1) {
                                    line[0].cpos++;
                                    rc = esc_seq_move_cur_right(sockfd, 1);
                                }
                                break;
                            case 'D': /* LEFT Arrow */
                                if (line[0].cpos > 0) {
                                 line[0].cpos--;
                                 rc = esc_seq_move_cur_left(sockfd, 1);
                                }
                                break;
                            case 'H': /* Home Key */
                                if (!line[0].cpos)
                                    return;
                                rc = esc_seq_move_cur_left(sockfd, line[0].cpos);
                                line[0].cpos = 0;
                                break;
                            case 'F': /* End key */
                                if (line_is_empty(&line[0]))
                                    return;
                                if (line[0].cpos == line[0].lpos + 1)
                                    return;
                                rc = esc_seq_move_cur_right(sockfd, line[0].n - line[0].cpos);
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
                                    /* Deleting the last char */
                                    else if (line[0].lpos   == line[0].cpos) {
                                        line[0].lbuf[line[0].cpos] = '\0';
                                        if (line[0].lpos) line[0].lpos--;
                                        line[0].n--;
                                        rc = write(sockfd, (const char *)" ", 1);
                                        esc_seq_move_cur_left(sockfd, 1);
                                    }
                                     /* In the middle of line */
                                     else if (line[0].cpos >= 0 && line[0].cpos <= line[0].lpos) {
                                        int rem = line[0].lpos - line[0].cpos + 1;
                                        line_del_charat(&line[0], line[0].cpos);
                                        esc_seq_erase_curr_line_from_cur_end(sockfd);
                                        rc = write(sockfd, (const char *)line[0].lbuf + line[0].cpos, rem -1);
                                        printf ("rc = %d, rem = %d\n", rc, rem);
                                        esc_seq_move_cur_left(sockfd, rem - 1);
                                     }
                                     break;
                            }
                            break;
                            case '1': /* Home Key */
                                switch(msg[3]) {
                                    case '~':
                                        if (!line[0].cpos) return;
                                        rc = esc_seq_move_cur_left(sockfd, line[0].cpos);
                                        line[0].cpos = 0;
                                        break;
                                }
                            break;
                            case '4': /* END key */
                                switch (msg[3]) {
                                case '~':
                                    if (line_is_empty(&line[0])) return;
                                    if (line[0].cpos == line[0].lpos + 1) return;
                                    rc = esc_seq_move_cur_right(sockfd, line[0].n - line[0].cpos);
                                    line[0].cpos = line[0].lpos + 1;
                                    break;
                                }
                            break;
                    }
            }
    }
 }

static void
 ReadLongerCharMsg(int sockfd, unsigned char *msg, uint16_t msg_size) {

     uint16_t i;

     static bool initialized = false;

     if (!initialized) {
         initialized = true;
         return;
     } 

     for (i = 0 ; i < msg_size ; i++) {
    
         printf("%c[%d]\n", msg[i], msg[i]);
         switch(msg[i]) {
             case LINE_FEED_KEY:
             case CARRIAGE_KEY:
             case NULL_KEY:
                if (line[0].n) {
                    write(sockfd, "\n\r", 2);
                }
                EnhancedParser(sockfd, (char *)line[0].lbuf, line[0].n);
                line_reinit(&line[0]);
                 break;
            default:
                line_add_character(&line[0], msg[i]);
                line[0].cpos++;
                write(sockfd, (char *)&msg[i], 1);
                break;
         }
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
            break;
           ;
    }
}
