#ifndef TEST_INTERFACE_H
#define TEST_INTERFACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

// Constants
#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define CURSOR_SAVE "\033[s"
#define CURSOR_RESTORE "\033[u"
#define MOVE_TO(row, col) "\033[%d;%dH"

// Struct definitions
struct time
{
    int seconds;
    int minutes;
    int hours;
    int days;
};

// Function declarations
float read_temperature(void);
void setup_terminal(void);
void restore_terminal(void);
void get_terminal_size(void);
void draw_interface(float current_temp, float desired_temp, int is_heating);
void update_values(float current_temp, float desired_temp, int is_heating);
void set_timer(void);
void signal_handler(int signum);
void window_change_handler(int signum);

#endif /* TEST_INTERFACE_H */
