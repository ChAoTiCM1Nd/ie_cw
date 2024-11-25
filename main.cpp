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

// Rotary encoder pins and instance
mRotaryEncoder encoder(PA_1, PA_4);

// Constants and variables
const int max_rpm = 3600;           // Maximum fan RPM
volatile int pulse_count = 0;       // Counts tachometer pulses
volatile int target_rpm = 1200;     // Initial target RPM (50% of max RPM)
float Kp = 0.00042, Ki = 0.0, Kd = 0.0;
float current_duty_cycle = 0.0;     // Initial duty cycle
float prev_error = 0, integral = 0;
const float integral_max = 500.0, integral_min = -500.0; // Integral limits

// Function to update fan speed based on target RPM
void update_fan_speed(float duty_cycle) {
    fan.write(duty_cycle);
}

// Interrupt service routine to count tachometer pulses
void count_pulse() {
    pulse_count++;
}

int main() {
    mypc.set_format(8, BufferedSerial::None, 1); // Set serial format
    printf("Starting fan control with rotary encoder\n");

    // Initialize PWM for the fan
    fan.period(0.2f);
    fan_tacho.rise(&count_pulse); // Attach tachometer interrupt

    Timer rpm_timer;
    rpm_timer.start();
    lcd.writeLine("Initializing...", 0); // Write to the first line

    while (true) {
        // Read rotary encoder value and adjust target RPM
        int encoder_change = encoder.getDiff(); // Get rotation difference
        if (encoder_change != 0) {
            target_rpm += encoder_change * 100; // Change RPM in increments of 100
            if (target_rpm > max_rpm) target_rpm = max_rpm;
            if (target_rpm < 0) target_rpm = 0;

            char buffer[16];
            sprintf(buffer, "Target RPM: %d", target_rpm);
            lcd.writeLine(buffer, 0);
        }

        // Calculate RPM and apply PID control every second
        if (rpm_timer.elapsed_time().count() >= 1000000) { // 1 second elapsed
            rpm_timer.reset();
            int rpm;
            {
                CriticalSectionLock lock; // Ensure atomic access
                rpm = (pulse_count / 2) * 60; // Calculate RPM
                pulse_count = 0;            // Reset pulse count
            }

            float error = target_rpm - rpm;
            integral += error * 0.1f; // delta_t is 0.1 seconds
            float derivative = error - prev_error;

            // Clamp integral to avoid windup
            if (integral > integral_max) integral = integral_max;
            if (integral < integral_min) integral = integral_min;

            // PID computation
            float pid_output = (Kp * error) + (Ki * integral) + (Kd * derivative);
            current_duty_cycle += pid_output;

            // Clamp duty cycle
            if (current_duty_cycle <= 0.0f) current_duty_cycle = 0.0f;
            if (current_duty_cycle >= 1.0f) current_duty_cycle = 1.0f;

            update_fan_speed(current_duty_cycle);

            // Display RPM on the second line
            char buffer_rpm[16];
            sprintf(buffer_rpm, "RPM: %d", rpm);
            lcd.writeLine(buffer_rpm, 1);

            printf("Calculated duty cycle: %.2f, Fan RPM: %d, Target RPM: %d\n", current_duty_cycle, rpm, target_rpm);

            prev_error = error;
        }
    }
}
