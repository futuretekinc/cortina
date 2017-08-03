/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2009 by Cortina Systems Incorporated.                 */
/***********************************************************************/

#include "serial_cs752x.h"
#include "common.h"

#define CYGNUM_IROSBOOT_CMD_LINE_EDITING 0

#define CYGPKG_IROSBOOT_MAX_CMD_LINE 128

//bool const console_echo = true;
//bool const cmd_history = true;
#define console_echo  1


static void
do_idle(unsigned int is_idle)
{
        // do nothing
        return;
}

static void 
mon_write_char(char c)
{
    serial_putc(c);
}



static void 
mon_read_char(char *c)
{
    *c = (char) serial_getc();
}


static int
mon_read_char_with_timeout(char *c)
{
        return serial_getc_with_timeout(c);
}


//
// Read a line of input from the user
// Return:
//        _GETS_OK: 'n' valid characters received
//       _GETS_GDB: '$' (GDB lead-in)
//   _GETS_TIMEOUT: No input before timeout
//     _GETS_CTRLC: ^C typed
//
static int
_rb_gets_preloaded(char *buf, int buflen, int timeout)
{
    char *ip = buf;   // Insertion point
    char *eol = buf;  // End of line
    char c;
    unsigned int res = 0;
    static char last_ch = '\0';
    int _timeout;

#if 0
    // Display current buffer data
    while (*eol) {
        mon_write_char(*eol++);
    }
#endif

    ip = eol;

    while (1) {
        //if ((timeout > 0) && (eol == buf)) {
        if (0) {
#define MIN_TIMEOUT 50

            while (timeout > 0) {

                res = mon_read_char_with_timeout(&c);
                if (res) {
                    // Got a character
                    do_idle(0);
                    break;
                }
                timeout -= _timeout;
            }
            if (res == 0) {
                do_idle(1);
                return _GETS_TIMEOUT;  // Input timed out
            }
        } else {
            mon_read_char(&c);
        }

        *eol = '\0';
        switch (c) {
#define CTRL(c) ((c)&0x1F)
            case CTRL('C'): // ^C
                // Abort current input
                serial_puts("^C\n");
                *buf = '\0';  // Nothing useful in buffer
                return _GETS_CTRLC;
            case '\n':
            case '\r':
                // If previous character was the "other" end-of-line, ignore this one
                if (((c == '\n') && (last_ch == '\r')) ||
                    ((c == '\r') && (last_ch == '\n'))) {
                    c = '\0';
                    break;
                }
                // End of line
                if (console_echo) {
                    mon_write_char('\r');
                    mon_write_char('\n');
                }
                last_ch = c;
                return _GETS_OK;
            case '\b':
            case 0x7F:  // DEL
                if (ip != buf) {
                    if (console_echo) {
                        mon_write_char('\b');
                        mon_write_char(' ');
                        mon_write_char('\b');
                    }
                    ip--;
                    eol--;

                }
                break;
            default:
                if (console_echo) {
                    mon_write_char(c);
                }
                if (ip == eol) {
                    // Advance both pointers
                    *ip++ = c;
                    eol = ip;
                }
        }
        last_ch = c;
        if (ip == buf + buflen - 1) { // Buffer full
            *ip = '\0';
            return buflen;
        }
    }
}

int
_rb_gets(char *buf, int buflen, int timeout)
{
        *buf = '\0';  // Empty buffer
        return _rb_gets_preloaded(buf, buflen, timeout);
}

