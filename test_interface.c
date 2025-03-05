#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define CURSOR_SAVE "\033[s"
#define CURSOR_RESTORE "\033[u"
#define MOVE_TO(row, col) "\033[%d;%dH"

// Global variables
float desired_temp = 21.0; // Default temperature
volatile sig_atomic_t shutdown = 0;
static struct termios old_termios, new_termios;
int term_rows, term_cols;
float current_temp = 20.0; // Starting temperature
float last_update_time = 0.0;
int window_changed = 0;
int first_run = 1;

// Mock temperature reading (simulates sensor with realistic temperature changes)
float read_temperature(void)
{
    // Use fixed time delta of 0.5 seconds (matches the usleep in main)
    float delta_time = 0.5;

    // Calculate temperature change
    float temp_difference = desired_temp - current_temp;

    // Simulate heating/cooling rates
    float heating_rate = 0.5; // degrees per second when heating
    float cooling_rate = 0.3; // degrees per second when cooling
    float ambient_loss = 0.1; // degrees per second lost to environment

    // Add some random fluctuation
    float noise = ((float)rand() / RAND_MAX - 0.5) * 0.1;

    if (temp_difference > 0)
    {
        // Need to heat up
        current_temp += (heating_rate * delta_time - ambient_loss * delta_time + noise);
    }
    else
    {
        // Need to cool down (or maintain)
        current_temp += (-cooling_rate * delta_time - ambient_loss * delta_time + noise);
    }

    // Add some ambient temperature influence
    float ambient_temp = 20.0; // Room temperature
    float ambient_influence = (ambient_temp - current_temp) * 0.1 * delta_time;
    current_temp += ambient_influence;

    return current_temp;
}

void setup_terminal(void)
{
    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    printf(HIDE_CURSOR);
}

void restore_terminal(void)
{
    printf(SHOW_CURSOR);
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

void get_terminal_size(void)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    term_rows = w.ws_row;
    term_cols = w.ws_col;
}

void draw_interface(float current_temp, float desired_temp, int is_heating)
{
    get_terminal_size();

    // Use full terminal dimensions with small margin
    int box_width = term_cols - 4;  // Leave 2 character margin on each side
    int box_height = term_rows - 2; // Leave 1 character margin top and bottom
    int start_col = 2;              // Start 2 characters from left
    int start_row = 1;              // Start 1 character from top

    // Clear screen and move to starting position
    printf(CLEAR_SCREEN CURSOR_HOME);

    // Draw top border
    printf(MOVE_TO(% d, % d), start_row, start_col);
    printf("╔");
    for (int i = 0; i < box_width - 2; i++)
        printf("═");
    printf("╗");

    // Draw title box
    printf(MOVE_TO(% d, % d), start_row + 1, start_col);
    printf("║%*s%*s║", box_width - 2, "", 0, "");
    printf(MOVE_TO(% d, % d), start_row + 2, start_col);
    printf("║%*sTEMP CONTROL%*s║",
           (box_width - 12) / 2, "",
           (box_width - 15 + 1) / 2, "");
    printf(MOVE_TO(% d, % d), start_row + 3, start_col);
    printf("║%*s%*s║", box_width - 2, "", 0, "");

    // Draw separator after title
    printf(MOVE_TO(% d, % d), start_row + 4, start_col);
    printf("╠");
    for (int i = 0; i < box_width - 2; i++)
        printf("═");
    printf("╣");

    // Calculate the width needed for the stats block
    int stats_width = 30;                             // Width of the longest stat line
    int stats_margin = (box_width - stats_width) / 2; // Center the stats block

    printf(MOVE_TO(% d, % d), start_row + 5, start_col);
    printf("║%*s%*s║", box_width - 2, "", 0, "");

    printf(MOVE_TO(% d, % d), start_row + 6, start_col);
    printf("║%*sCurrent Temperature: %6.1f°C%*s║",
           stats_margin, "",
           current_temp,
           box_width - (stats_margin + stats_width) - 1, "");

    printf(MOVE_TO(% d, % d), start_row + 7, start_col);
    printf("║%*s%*s║", box_width - 2, "", 0, "");

    printf(MOVE_TO(% d, % d), start_row + 8, start_col);
    printf("║%*sDesired Temperature: %6.1f°C%*s║",
           stats_margin, "",
           desired_temp,
           box_width - (stats_margin + stats_width) - 1, "");

    printf(MOVE_TO(% d, % d), start_row + 9, start_col);
    printf("║%*s%*s║", box_width - 2, "", 0, "");

    printf(MOVE_TO(% d, % d), start_row + 10, start_col);
    printf("║%*sHeater Status: %s%*s║",
           stats_margin, "",
           is_heating ? "ON 🔥" : "OFF ❄️",
           box_width - (stats_margin + stats_width) + 8, "");
    printf(MOVE_TO(% d, % d), start_row + 11, start_col);
    printf("║%*s%*s║", box_width - 2, "", 0, "");

    // Draw separator after stats
    printf(MOVE_TO(% d, % d), start_row + 12, start_col);
    printf("╠");
    for (int i = 0; i < box_width - 2; i++)
        printf("═");
    printf("╣");

    // Fill middle space with empty lines
    for (int i = start_row + 13; i < start_row + box_height - 3; i++)
    {
        printf(MOVE_TO(% d, % d), i, start_col);
        printf("║%*s║", box_width - 2, "");
    }

    // Draw controls at the bottom
    printf(MOVE_TO(% d, % d), start_row + box_height - 3, start_col);
    printf("║%*sPress 'q' to quit%*s║",
           ((box_width) / 2) - 9, "",
           ((box_width) / 2) - 10, "");
    printf(MOVE_TO(% d, % d), start_row + box_height - 2, start_col);
    printf("║%*sPress 's' to set new temperature%*s║",
           (box_width - 31) / 2, "",
           ((box_width) / 2) - 18, "");

    // Draw bottom border
    printf(MOVE_TO(% d, % d), start_row + box_height - 1, start_col);
    printf("╚");
    for (int i = 0; i < box_width - 2; i++)
        printf("═");
    printf("╝");

    fflush(stdout);
}

void update_values(float current_temp, float desired_temp, int is_heating)
{
    get_terminal_size();

    int box_width = term_cols - 4;                    // Leave 2 character margin on each side
    int box_height = term_rows - 2;                   // Leave 1 character margin top and bottom
    int start_col = 2;                                // Start 2 characters from left
    int start_row = 1;                                // Start 1 character from top
    int stats_width = 30;                             // Width of the longest stat line
    int stats_margin = (box_width - stats_width) / 2; // Center the stats block

    // Update current temperature
    printf(MOVE_TO(% d, % d), start_row + 6, start_col);
    printf("║%*sCurrent Temperature: %6.1f°C%*s║",
           stats_margin, "",
           current_temp,
           box_width - (stats_margin + stats_width) - 1, "");

    printf(MOVE_TO(% d, % d), start_row + 7, start_col);
    printf("║%*s%*s║", box_width - 2, "", 0, "");

    printf(MOVE_TO(% d, % d), start_row + 8, start_col);
    printf("║%*sDesired Temperature: %6.1f°C%*s║",
           stats_margin, "",
           desired_temp,
           box_width - (stats_margin + stats_width) - 1, "");

    printf(MOVE_TO(% d, % d), start_row + 9, start_col);
    printf("║%*s%*s║", box_width - 2, "", 0, "");

    printf(MOVE_TO(% d, % d), start_row + 10, start_col);
    printf("║%*sHeater Status: %s%*s║",
           stats_margin, "",
           is_heating ? "ON 🔥" : "OFF ❄️",
           is_heating ? (box_width - (stats_margin + stats_width) + 8) : (box_width - (stats_margin + stats_width) + 7), "");

    fflush(stdout);
}

void set_new_temperature(void)
{
    // Temporarily restore canonical mode for input and make stdin blocking again
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);

    // Remove non-blocking flag from stdin
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    printf(CLEAR_SCREEN CURSOR_HOME);
    printf("Enter new desired temperature (°C): ");
    printf(SHOW_CURSOR);

    char input[32];
    float new_temp;

    // Read the new temperature
    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        if (sscanf(input, "%f", &new_temp) == 1)
        {
            desired_temp = new_temp;
        }
    }

    // Restore non-canonical mode
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    // Set stdin back to non-blocking
    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    printf(HIDE_CURSOR);
}

// Signal handler for Ctrl+C
void signal_handler(int signum)
{
    shutdown = 1;
}

// Add new signal handler for window changes
void window_change_handler(int signum)
{
    window_changed = 1;
}

int main(void)
{
    signal(SIGINT, signal_handler);
    signal(SIGWINCH, window_change_handler); // Add window change signal handler

    // Initialize interface
    setup_terminal();
    atexit(restore_terminal);

    // Make stdin non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    float last_current_temp = -1;
    float last_desired_temp = -1;
    int last_heating_state = -1;

    while (!shutdown)
    {
        float current_temp = read_temperature();
        int is_heating = (current_temp < desired_temp);

        // Redraw full screen on first run or window size change
        if (first_run || window_changed)
        {
            draw_interface(current_temp, desired_temp, is_heating);
            first_run = 0;
            window_changed = 0;
        }
        // Update values only if they changed
        else if (current_temp != last_current_temp ||
                 desired_temp != last_desired_temp ||
                 is_heating != last_heating_state)
        {
            update_values(current_temp, desired_temp, is_heating);
        }

        last_current_temp = current_temp;
        last_desired_temp = desired_temp;
        last_heating_state = is_heating;

        // Check for input (non-blocking)
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0)
        {
            if (c == 'q' || c == 'Q')
            {
                printf(CLEAR_SCREEN);
                break;
            }
            else if (c == 's' || c == 'S')
            {
                set_new_temperature();
                first_run = 1; // Redraw full screen after temperature input
            }
        }

        usleep(500000); // Update every 0.5 seconds
    }

    return 0;
}