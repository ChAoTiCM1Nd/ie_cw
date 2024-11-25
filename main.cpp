#include "mbed.h"
#include <iterator>
#include <stdio.h>
#include <cstdint>
#include "LCD_ST7066U.h"

LCD lcd(PB_15, PB_14, PB_10, PA_8, PB_2, PB_1); // Instantiated LCD

enum FanMode{
    OFF,
    ENCDR_C_LOOP,
    ENCDR_O_LOOP,
    AUTO
}; 

DigitalOut led(LED1);
DigitalOut led_ext(PC_0);           // External LED for counterclockwise indication
PwmOut fan(PB_0);                   // PWM control for the fan
InterruptIn fan_tacho(PA_0);        // Tachometer input to count pulses
DigitalIn inc1(PA_1);               // Rotary encoder channel A
DigitalIn inc2(PA_4);               // Rotary encoder channel B

BufferedSerial mypc(USBTX, USBRX, 19200);
DigitalIn button(BUTTON1);

Timer c_timer;

FanMode current_mode = OFF;

volatile int pulse_count = 0;       // Counts tachometer pulses
volatile int target_rpm = 200;     // Initial target RPM (50% of max RPM)
int inc1_prev = 0;                  // Previous state of inc1 for edge detection

volatile int calculated_rpm = 0;
Mutex rpm_mutex;
Mutex lcd_mutex;

// Define limits for the integral term
const float integral_max = 500.0; // Adjust this limit based on tuning
const float integral_min = -500.0;

float Kp = 0.000142;
float Ki = 0.00000;
float Kd = 0.0000;

float current_duty_cycle = 0.0;     // Initial duty cycle set to 50%
float prev_error = 0;
float integral = 0;

// Previous states for change detection
bool last_button_state = 1; // Button was initially not pressed (active-low)
int last_calculated_rpm = 0;
int last_target_rpm = target_rpm;

void count_pulse() {
    CriticalSectionLock lock;
    pulse_count++;
    led = !led;
}

void rpm_calc_thread(){
    Timer timer;
    timer.start();

    while (true) {
        if (timer.elapsed_time().count() >= 1000000) { // 1-second interval
            timer.reset();

            // Calculate RPM: (pulse_count / 2) * 60 for 2-pulse/rev fans
            int rpm;
            {
                CriticalSectionLock lock; // Atomic access to `pulse_count`
                rpm = pulse_count;
                pulse_count = 0; // Reset after reading
            }

            // Update shared variable
            rpm_mutex.lock();
            calculated_rpm = rpm;
            rpm_mutex.unlock();
        }
        ThisThread::sleep_for(5ms); // Prevent high CPU usage
    }
}

// Function to update fan speed based on target RPM
void update_fan_speed(float duty_cycle) {
    if (duty_cycle < 0.0) duty_cycle = 0.0;
    if (duty_cycle > 1.0) duty_cycle = 1.0;
    
    fan.write(duty_cycle);
}

void handle_off_mode(){
    fan.write(0.0); // Turn off the fan
    lcd.writeLine("Off", 0);
    if (last_button_state == 0) {
        printf("Fan is OFF\n");  // Print state to serial monitor only on change
    }
}

void safe_lcd_write(const char* text, int line) {
    lcd_mutex.lock();
    lcd.writeLine(text, line);
    lcd_mutex.unlock();
}

void calc_target_rpm(){
    // Rotary encoder logic
    if (inc1.read() != inc1_prev) {
        if (inc1_prev == 0 && inc1.read() == 1) {    // Rising edge of inc1
            if (inc2.read() == 0) {
                // Clockwise rotation: increase target RPM
                target_rpm += 25;  // Increase target RPM by 25
            } else {
                // Counterclockwise rotation: decrease target RPM
                target_rpm -= 25;  // Decrease target RPM by 25
            }

            if (target_rpm < 0) target_rpm = 0;

            // Writing the target RPM to the LCD.
            char buffer[16];
            sprintf(buffer, "Target RPM: %d", target_rpm);
            safe_lcd_write(buffer, 0);

            if (target_rpm != last_target_rpm) {
                printf("Target RPM changed: %d\n", target_rpm);  // Print state to serial monitor only on change
            }

            last_target_rpm = target_rpm;
        }
    }
    inc1_prev = inc1.read(); // Update previous state
    wait_us(700); // Debounce
}

void handle_closed_loop_ctrl(){
    fan_tacho.rise(&count_pulse); // Count rising edges
    calc_target_rpm();

    c_timer.start();

    if (c_timer.elapsed_time().count() >= 1000000) { // 1-second interval
        c_timer.reset();

        int rpm;
        rpm_mutex.lock();
        rpm = calculated_rpm;
        rpm_mutex.unlock();

        float delta_t = 0.1f;
        int error = target_rpm - rpm;
        integral += error * delta_t;
        int derivative = error - prev_error;

        float pid_output = (Kp * error) + (Ki * 0.0f) + (Kd * 0.00f);

        current_duty_cycle += pid_output;
        update_fan_speed(current_duty_cycle);

        if (rpm != last_calculated_rpm) {
            printf("Fan RPM: %d, Target RPM: %d, duty cycle: %.2f\n", rpm, target_rpm, current_duty_cycle);  // Print state to serial monitor only on change
            last_calculated_rpm = rpm;
        }

        prev_error = error;
    }
}

void handle_open_loop_ctrl() {
    calc_target_rpm();

    c_timer.start();

    if (c_timer.elapsed_time().count() >= 1000000) { // 1-second interval
        c_timer.reset();

        // Calculate and set PWM duty cycle based on target RPM
        // Use the equation: y = 653.38 * target_rpm - 94.626
        current_duty_cycle = 653.38 * target_rpm - 94.626;

        // Map the duty cycle to a valid range (0.0 to 1.0)
        if (current_duty_cycle < 0.0) current_duty_cycle = 0.0;
        if (current_duty_cycle > 1.0) current_duty_cycle = 1.0;

        // Set the adjusted duty cycle to the fan
        fan.write(current_duty_cycle);

        // Display duty cycle on LCD
        char open_buffer[16];
        sprintf(open_buffer, "Duty Cycle: %.2f", current_duty_cycle);
        safe_lcd_write(open_buffer, 1);

        if (current_duty_cycle != last_calculated_rpm) {
            printf("Duty Cycle changed: %.2f\n", current_duty_cycle);  // Print state to serial monitor only on change
            last_calculated_rpm = current_duty_cycle;
        }

        update_fan_speed(current_duty_cycle);
    }
}

// Function to handle AUTO mode
void handle_auto_mode() {
    lcd.clear();
    lcd.writeLine("Auto", 0);
    printf("Fan is in AUTO mode\n");  // Print state to serial monitor only on change
}

void update_state() {
    static Timer debounce_timer;
    debounce_timer.start();

    int button_state = button.read();  // Read the current button state
    if (button_state == 0 && last_button_state == 1 && debounce_timer.elapsed_time().count() > 100000) {
        debounce_timer.reset();

        // Cycle through the states
        current_mode = static_cast<FanMode>((current_mode + 1) % 4);

        // Print state change only if mode changes
        switch (current_mode) {
            case OFF:
                lcd.writeLine("Mode: Off", 0);
                if (last_button_state == 0) {
                    printf("Fan is OFF\n");  // Print state to serial monitor only on change
                }
                break;
            case ENCDR_C_LOOP:
                lcd.writeLine("Mode: Closed Loop", 0);
                printf("Fan is in Closed Loop mode\n");  // Print state to serial monitor only on change
                break;
            case ENCDR_O_LOOP:
                lcd.writeLine("Mode: Open Loop", 0);
                printf("Fan is in Open Loop mode\n");  // Print state to serial monitor only on change
                break;
            case AUTO:
                lcd.writeLine("Mode: Auto", 0);
                printf("Fan is in AUTO mode\n");  // Print state to serial monitor only on change
                break;
        }
    }
    last_button_state = button_state;  // Update the previous state of the button
}

// Main program
int main() {
    button.mode(PullUp);
    printf("Starting fan control with encoder\n");

    // Initialize PWM for the fan
    fan.period(0.02f);   

    lcd.writeLine("Initializing...", 0); // Write text to the second line

    rtos::Thread rpm_thread;
    rpm_thread.start(rpm_calc_thread);

    // Main Finite State Machine logic here. 
    while (true) {
        update_state();

        // Execute logic for the current mode
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
                handle_auto_mode();
                break;
        }
        // Add a short delay to reduce CPU usage
        ThisThread::sleep_for(5ms);
    }
}
