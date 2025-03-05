#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define CURSOR_SAVE "\033[s"
#define CURSOR_RESTORE "\033[u"
#define MOVE_TO(row, col) "\033[%d;%dH"

extern float desired_temp;
extern float read_temperature(void);

static struct termios old_termios, new_termios;

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

void draw_interface(float current_temp, float desired_temp, int is_heating)
{
    // Initial full draw
    printf(CLEAR_SCREEN CURSOR_HOME);
    printf("╔════════════════════════════════════════╗\n");
    printf("║        Temperature Control System       ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║ Current Temperature: %6.1f°C          ║\n", current_temp);
    printf("║ Desired Temperature: %6.1f°C          ║\n", desired_temp);
    printf("║ Heater Status: %-20s ║\n", is_heating ? "ON 🔥" : "OFF ❄️");
    printf("╠════════════════════════════════════════╣\n");
    printf("║ Press 'q' to quit                      ║\n");
    printf("║ Press 's' to set new temperature       ║\n");
    printf("╚════════════════════════════════════════╝\n");
}

void update_values(float current_temp, float desired_temp, int is_heating)
{
    // Update current temperature (row 4, column 22)
    printf(MOVE_TO(4, 22), 4, 22);
    printf("%6.1f", current_temp);

    // Update desired temperature (row 5, column 22)
    printf(MOVE_TO(5, 22), 5, 22);
    printf("%6.1f", desired_temp);

    // Update heater status (row 6, column 16)
    printf(MOVE_TO(6, 16), 6, 16);
    printf("%-20s", is_heating ? "ON 🔥" : "OFF ❄️");

    // Flush stdout to ensure immediate update
    fflush(stdout);
}

void set_new_temperature(void)
{
    printf(CLEAR_SCREEN CURSOR_HOME);
    printf("Enter new desired temperature (°C): ");
    printf(SHOW_CURSOR);

    char input[32];
    float new_temp;

    // Temporarily restore canonical mode for input
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        if (sscanf(input, "%f", &new_temp) == 1)
        {
            desired_temp = new_temp;
        }
    }

    // Restore non-canonical mode
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    printf(HIDE_CURSOR);
}

int main(void)
{
    setup_terminal();
    atexit(restore_terminal);

    float last_current_temp = -1;
    float last_desired_temp = -1;
    int last_heating_state = -1;
    int first_run = 1;

    while (1)
    {
        float current_temp = read_temperature();
        int is_heating = (current_temp < desired_temp);

        // Only redraw full screen on first run
        if (first_run)
        {
            draw_interface(current_temp, desired_temp, is_heating);
            first_run = 0;
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

        // Check for input
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1)
        {
            if (c == 'q')
            {
                break;
            }
            else if (c == 's')
            {
                set_new_temperature();
                first_run = 1; // Redraw full screen after temperature input
            }
        }

        usleep(500000); // Update every 0.5 seconds
    }

    return 0;
}
