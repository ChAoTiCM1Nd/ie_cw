#include "mbed.h"
#include "LCD_ST7066U.h"
#include "mRotaryEncoder.h"

// Initialize components
LCD lcd(PB_15, PB_14, PB_10, PA_8, PB_2, PB_1); // Instantiated LCD
DigitalOut led(LED1);
DigitalOut led_ext(PC_0);           // External LED for counterclockwise indication
PwmOut fan(PB_0);                   // PWM control for the fan
InterruptIn fan_tacho(PA_0);        // Tachometer input to count pulses
BufferedSerial mypc(USBTX, USBRX, 115200);

// Constants and variables
const int max_rpm = 3600;           // Maximum fan RPM
volatile int pulse_count = 0;       // Counts tachometer pulses
volatile int target_rpm = 1200;     // Initial target RPM (50% of max RPM)
float current_duty_cycle = 0.0;     // Initial duty cycle
float prev_error = 0, integral = 0;
const float integral_max = 500.0, integral_min = -500.0; // Integral limits

// Array to store duty cycle to RPM results
float duty_cycles[21];  // Array to store duty cycle values
int rpm_results[21];    // Array to store corresponding RPM values
int index = 0;          // Current index for storing results

// Function to update fan speed based on duty cycle
void update_fan_speed(float duty_cycle) {
    fan.write(duty_cycle);
}

// Interrupt service routine to count tachometer pulses
void count_pulse() {
    pulse_count++;
}

int main() {
    mypc.set_format(8, BufferedSerial::None, 1); // Set serial format
    printf("Starting fan control with duty cycle ramp\n");

    // Initialize PWM for the fan
    fan.period(0.2f);  // PWM period of 0.2 seconds
    fan_tacho.rise(&count_pulse); // Attach tachometer interrupt

    Timer rpm_timer;
    rpm_timer.start();
    lcd.writeLine("Initializing...", 0); // Write to the first line

    Timer duty_timer;    // Timer to increase duty cycle every 5 seconds
    duty_timer.start();  // Start the duty timer

    // Start the duty cycle ramp from 0 to 100%
    while (current_duty_cycle < 1.0) {
        // Increase duty cycle every 5 seconds
        if (duty_timer.read() >= 5.0) {
            duty_timer.reset();  // Reset the timer after 5 seconds

            // Increase the duty cycle by 0.05 (5%) each time
            if (current_duty_cycle < 1.0) {
                current_duty_cycle += 0.05;
                if (current_duty_cycle > 1.0) {
                    current_duty_cycle = 1.0;  // Ensure we do not exceed 100%
                }
            }

            // Update fan speed and display duty cycle
            update_fan_speed(current_duty_cycle);
            char buffer[16];
            sprintf(buffer, "Duty: %.2f", current_duty_cycle);
            lcd.writeLine(buffer, 0);  // Display duty cycle on the LCD

            // Calculate RPM only during the active period of the duty cycle
            int rpm = 0;

            // Wait for the duration of the high period (active duty cycle)
            Timer duty_timer_active;
            duty_timer_active.start();
            while (duty_timer_active.read() < (current_duty_cycle * 0.2)) {
                // Wait for the duration of the high period (active duty cycle)
                ThisThread::sleep_for(100ms); // Sleep for 100ms between checks
            }

            // Stop measuring RPM after the high period ends
            duty_timer_active.stop();
            
            {
                CriticalSectionLock lock; // Ensure atomic access
                rpm = pulse_count;
                pulse_count = 0;            // Reset pulse count
            }

            // Store results
            duty_cycles[index] = current_duty_cycle;
            rpm_results[index] = rpm;
            index++;

            printf("Duty Cycle: %.2f, RPM: %d\n", current_duty_cycle, rpm);
        }

        // Sleep for a short duration to avoid unnecessary CPU usage
        ThisThread::sleep_for(100ms);
    }

    // Print all results at the end of the test
    printf("\nDuty Cycle to RPM Results:\n");
    for (int i = 0; i < index; i++) {
        printf("Duty Cycle: %.2f, RPM: %d\n", duty_cycles[i], rpm_results[i]);
    }

    lcd.writeLine("Test Complete", 0);
    while (true) {
        // Keep the program running
        ThisThread::sleep_for(1000ms);
    }
}
