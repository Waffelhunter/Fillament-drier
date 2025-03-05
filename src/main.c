#include <stdio.h>
#include <pigpio.h>
#include <time.h>
#include <signal.h>

#define HEAT_SENSOR_PIN 4    // GPIO4 temperature sensor
#define TRANSISTOR 17        // GPIO17
#define DEFAULT_TEMP 0.0     // Default desired temperature in Celsius
#define TEMP_TOLERANCE 2.0   // Temperature tolerance range (+/-)
#define SAMPLE_INTERVAL 5000 // Sample interval in milliseconds
#define MAX_TEMP 100.0
#define TEMP_READ_RETRIES 3
#define MIN_VALID_VOLTAGE 0.2
#define MAX_VALID_VOLTAGE 3.0
#define SENSOR_READ_INTERVAL_MS 100 // Time between retry attempts

// Global variables
float desired_temp = DEFAULT_TEMP;
time_t temp_change_start = 0;
int temp_change_duration = 0;
volatile sig_atomic_t shutdown = 0;

void signal_handler(int sig)
{
    shutdown = 1;
}

float read_temperature(void)
{
    float total_valid_readings = 0;
    int valid_readings_count = 0;

    // Try multiple readings to ensure validity
    for (int i = 0; i < TEMP_READ_RETRIES; i++)
    {
        // Read raw value from temperature sensor
        int raw_value = gpioRead(HEAT_SENSOR_PIN);

        // Convert to voltage
        float voltage = raw_value * (3.3 / 1024.0);

        // Validate voltage reading
        if (voltage < MIN_VALID_VOLTAGE || voltage > MAX_VALID_VOLTAGE)
        {
            fprintf(stderr, "Warning: Invalid voltage reading: %.2fV\n", voltage);
            time_sleep(SENSOR_READ_INTERVAL_MS / 1000.0);
            continue;
        }

        // Convert to temperature
        float temperature = (voltage - 0.5) * 100.0;

        // Validate temperature bounds
        if (temperature < 0.0 || temperature > MAX_TEMP)
        {
            fprintf(stderr, "Warning: Temperature out of range: %.1f°C, shutting down for safety\n", temperature);
            gpioWrite(TRANSISTOR, 0);
            return -1;
        }

        total_valid_readings += temperature;
        valid_readings_count++;

        time_sleep(SENSOR_READ_INTERVAL_MS / 1000.0);
    }

    // Check if we got any valid readings
    if (valid_readings_count == 0)
    {
        fprintf(stderr, "Error: Failed to get valid temperature reading after %d attempts\n",
                TEMP_READ_RETRIES);
        return -1;
    }

    // Return average of valid readings
    return total_valid_readings / valid_readings_count;
}

void control_heater(float current_temp)
{
    // If we got an error reading temperature, turn off heater for safety
    if (current_temp < 0)
    {
        gpioWrite(TRANSISTOR, 0);
        return;
    }

    if (current_temp < (desired_temp - TEMP_TOLERANCE))
    {
        // Turn heater ON if temperature is below desired range
        if (desired_temp < MAX_TEMP)
        {
            gpioWrite(TRANSISTOR, 1);
        }
    }
    else if (current_temp > (desired_temp + TEMP_TOLERANCE))
    {
        // Turn heater OFF if temperature is above desired range
        gpioWrite(TRANSISTOR, 0);
    }
}

int main()
{
    signal(SIGINT, signal_handler);
    if (gpioInitialise() < 0)
    {
        fprintf(stderr, "Failed to initialize pigpio\n");
        return 1;
    }

    // Setup pins
    gpioSetMode(HEAT_SENSOR_PIN, PI_INPUT);
    gpioSetMode(TRANSISTOR, PI_OUTPUT);

    printf("Temperature control system started.\n");
    printf("currently set to temperature: %.1f°C\n", desired_temp);

    while (!shutdown)
    {
        float current_temp = read_temperature();
        if ()

            // Check if temporary temperature change has expired
            if (temp_change_duration > 0)
            {
                time_t current_time = time(NULL);
                if (current_time - temp_change_start >= temp_change_duration)
                {
                    desired_temp = DEFAULT_TEMP;
                    temp_change_duration = 0;
                    printf("Reverting to default temperature: %.1f°C\n", desired_temp);
                }
            }

        // Control heater based on current temperature
        control_heater(current_temp);

        // Print status
        printf("Current: %.1f°C, Desired: %.1f°C\n", current_temp, desired_temp);

        // Check for temperature change input
        // This is a simplified example - implement your input method
        char input[32];
        if (fgets(input, sizeof(input), stdin) != NULL)
        {
            float new_temp;
            int duration;
            if (sscanf(input, "%f %d", &new_temp, &duration) == 2)
            {
                desired_temp = new_temp;
                temp_change_duration = duration;
                temp_change_start = time(NULL);
                printf("Temperature temporarily changed to %.1f°C for %d seconds\n",
                       desired_temp, temp_change_duration);
            }
        }

        // Wait before next reading
        time_sleep(SAMPLE_INTERVAL / 1000.0);
    }
    gpioWrite(TRANSISTOR, 0);
    gpioTerminate();
    return 0;
}
