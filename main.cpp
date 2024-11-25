#include "mbed.h"
#include <iterator>
#include <stdio.h>
#include <cstdint>
#include "LCD_ST7066U.h"
#include "mRotaryEncoder.h"

LCD lcd(PB_15, PB_14, PB_10, PA_8, PB_2, PB_1); // Instantiated LCD

// Rotary encoder using mRotaryEncoder
mRotaryEncoder encoder(PA_1, PA_4, NC, PullUp, 2000, 1, 1);

enum FanMode {
    OFF,
    ENCDR_C_LOOP,
    ENCDR_O_LOOP,
    AUTO
};

DigitalOut led(LED1);
PwmOut fan(PB_0);                   // PWM control for the fan
InterruptIn fan_tacho(PA_0);        // Tachometer input to count pulses
BufferedSerial mypc(USBTX, USBRX, 19200);
DigitalIn button(BUTTON1);

Timer c_timer;
FanMode current_mode = OFF;

volatile int pulse_count = 0;       // Counts tachometer pulses
volatile int target_rpm = 200;      // Initial target RPM
int last_encoder_value = 0;         // Previous encoder value for detecting changes

float Kp = 0.0003; // Adjusted for improved closed-loop behavior
float Ki = 0.00001;
float Kd = 0.0;

float current_duty_cycle = 0.0;     // Initial duty cycle
float prev_error = 0.0;
float integral = 0.0;

// Define limits for the integral term
const float integral_max = 500.0;
const float integral_min = -500.0;

const int MAX_RPM = 1800; // Maximum RPM corresponding to 100% duty cycle

Mutex rpm_mutex;
Mutex lcd_mutex;

Timer rpm_timer;

// Function to count tachometer pulses
void count_pulse() {
    CriticalSectionLock lock;
    pulse_count++;
    led = !led; // Toggle LED on each pulse
}

// Function to update fan speed based on duty cycle
void update_fan_speed(float duty_cycle) {
    if (duty_cycle < 0.0f) duty_cycle = 0.0f;
    if (duty_cycle > 1.0f) duty_cycle = 1.0f;

    fan.write(duty_cycle);
}

// Function for safely updating the LCD
void safe_lcd_write(const char* text, int line) {
    lcd_mutex.lock();
    lcd.writeLine(text, line);
    lcd_mutex.unlock();
}

// Function to calculate target RPM based on encoder input
void calc_target_rpm() {
    int encoder_value = encoder.Get();
    int encoder_diff = encoder_value - last_encoder_value;

    if (encoder_diff != 0) {
        target_rpm += encoder_diff * 25; // Adjust RPM by 25 per encoder step
        if (target_rpm < 0) target_rpm = 0;
        if (target_rpm > MAX_RPM) target_rpm = MAX_RPM;

        char buffer[16];
        sprintf(buffer, "Target RPM: %d", target_rpm);
        safe_lcd_write(buffer, 0);

        printf("Target RPM: %d\n", target_rpm);
    }

    last_encoder_value = encoder_value;
}

// Ensure tachometer is monitored
void setup_tachometer() {
    fan_tacho.rise(&count_pulse); // Attach interrupt on rising edge
    rpm_timer.start();
}

// Ensure PWM and fan are properly set up
void setup_pwm() {
    fan.period(0.02f); // 20 ms period for PWM
    fan.write(0.0f);   // Start with the fan off
}

// Check tachometer pulse count
void validate_tachometer() {
    if (pulse_count == 0) {
        printf("Tachometer pulse count is zero. Check tachometer wiring.\n");
    } else {
        printf("Tachometer is active. Pulse count: %d\n", pulse_count);
    }
}

// Updated closed-loop control logic
void handle_closed_loop_ctrl() {
    calc_target_rpm();

    if (rpm_timer.elapsed_time().count() >= 1000000) { // 1-second interval
        rpm_timer.reset();

        int rpm;
        {
            CriticalSectionLock lock; // Atomic access to pulse_count
            rpm = (pulse_count * 60); // 2 pulses per revolution: RPM = pulse_count * 30
            pulse_count = 0;          // Reset after calculation
        }

        validate_tachometer(); // Debug tachometer activity

        int error = target_rpm - rpm;

        // PID calculations
        float delta_t = 1.0f; // 1-second intervals
        integral += error * delta_t;

        // Integral windup protection
        if (integral > integral_max) integral = integral_max;
        if (integral < integral_min) integral = integral_min;

        float derivative = (error - prev_error) / delta_t;
        float pid_output = (Kp * error) + (Ki * integral) + (Kd * derivative);

        current_duty_cycle += pid_output;
        if (current_duty_cycle > 1.0f) current_duty_cycle = 1.0f;
        if (current_duty_cycle < 0.0f) current_duty_cycle = 0.0f;

        update_fan_speed(current_duty_cycle);

        printf("Closed Loop: Target RPM: %d, Measured RPM: %d, Duty Cycle: %.2f, Error: %d\n",
               target_rpm, rpm, current_duty_cycle, error);

        prev_error = error;
    }
}

// Function for open-loop control
void handle_open_loop_ctrl() {
    calc_target_rpm();

    // Calculate duty cycle based on target RPM
    current_duty_cycle = (float)target_rpm / MAX_RPM;

    // Constrain duty cycle
    if (current_duty_cycle > 1.0f) current_duty_cycle = 1.0f;
    if (current_duty_cycle < 0.0f) current_duty_cycle = 0.0f;

    update_fan_speed(current_duty_cycle);

    // Display duty cycle on LCD
    char buffer[16];
    sprintf(buffer, "Duty: %.2f", current_duty_cycle);
    safe_lcd_write(buffer, 1);

    printf("Open Loop: Target RPM: %d, Duty Cycle: %.2f\n", target_rpm, current_duty_cycle);
}

// Function to handle OFF mode
void handle_off_mode() {
    fan.write(0.0f); // Turn off the fan
    lcd.writeLine("Off", 0);
    printf("Fan is OFF\n");
}

// Function to update the state machine based on button input
void update_state() {
    static Timer debounce_timer;
    debounce_timer.start();

    static int last_button_state = 1;
    int button_state = button.read();

    if (button_state == 0 && last_button_state == 1 && debounce_timer.elapsed_time().count() > 100000) {
        debounce_timer.reset();

        // Cycle through the states
        current_mode = static_cast<FanMode>((current_mode + 1) % 4);

        // Update LCD based on mode
        switch (current_mode) {
            case OFF:
                lcd.writeLine("Mode: Off", 0);
                printf("Fan is OFF\n");
                break;
            case ENCDR_C_LOOP:
                lcd.writeLine("Mode: Closed Loop", 0);
                printf("Fan is in Closed Loop mode\n");
                break;
            case ENCDR_O_LOOP:
                lcd.writeLine("Mode: Open Loop", 0);
                printf("Fan is in Open Loop mode\n");
                break;
            case AUTO:
                lcd.writeLine("Mode: Auto", 0);
                printf("Fan is in AUTO mode\n");
                break;
        }
    }

    last_button_state = button_state;
}

// Main program
int main() {
    button.mode(PullUp);
    printf("Starting fan control with encoder\n");

    setup_pwm();
    setup_tachometer();
    lcd.writeLine("Initializing...", 0);

    while (true) {
        update_state();

        switch (current_mode) {
            case OFF:
                handle_off_mode();
                break;

            case ENCDR_C_LOOP:
                handle_closed_loop_ctrl();
                break;

            case ENCDR_O_LOOP:
                handle_open_loop_ctrl();
                break;

            case AUTO:
                // Placeholder for AUTO mode logic
                break;
        }

        ThisThread::sleep_for(5ms);
    }
}
