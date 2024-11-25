//https://chatgpt.com/share/6734cb16-d66c-800f-850d-4885cfd94bdc


#include "mbed.h"
#include <iterator>
#include <stdio.h>

#include <cstdint>
#include "LCD_ST7066U.h"


LCD lcd(PB_15, PB_14, PB_10, PA_8, PB_2, PB_1); // Instantiated 

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

//BufferedSerial mypc(USBTX, USBRX, 19200);
DigitalIn button(BUTTON1);


FanMode current_mode = OFF;

const int max_rpm = 3600;           // Maximum fan RPM for 100% duty cycle (adjusted)
volatile int pulse_count = 0;       // Counts tachometer pulses
volatile int target_rpm = 1740;     // Initial target RPM (50% of max RPM)
int inc1_prev = 0;                  // Previous state of inc1 for edge detection


volatile int calculated_rpm = 0;
Mutex rpm_mutex;
Mutex lcd_mutex;

// Define limits for the integral term
const float integral_max = 500.0; // Adjust this limit based on tuning
const float integral_min = -500.0;

float Kp = 0.00042;
float Ki = 0.00000;
float Kd = 0.0000;

float current_duty_cycle = 0.0;     // Initial duty cycle set to 50%
float prev_error = 0;
float integral = 0;

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
                rpm = (pulse_count / 2) * 60;
                pulse_count = 0; // Reset after reading
            }

            // Update shared variable
            rpm_mutex.lock();
            calculated_rpm = rpm;
            rpm_mutex.unlock();

            //printf("RPM calculated: %d\n", rpm);
        }
        ThisThread::sleep_for(5ms); // Prevent high CPU usage
    }

}


// Function to update fan speed based on target RPM
void update_fan_speed(float duty_cycle) {

    if (duty_cycle < 0.0) duty_cycle = 0.0;
    if (duty_cycle > 1.0) duty_cycle = 1.0;
    
    fan.write(duty_cycle);

    //printf("Current duty cycle: %.2f (Target RPM: %d)\n", duty_cycle, target_rpm);
}

void handle_off_mode(){
    fan.write(0.0); // Turn off the fan
    lcd.writeLine("Off", 0);
}

void safe_lcd_write(const char* text, int line) {
    lcd_mutex.lock();
    lcd.writeLine(text, line);
    lcd_mutex.unlock();
}


void calc_target_rpm(){

    // Rotary ecoder logic
    if (inc1.read() != inc1_prev) {
        if (inc1_prev == 0 && inc1.read() == 1) 
        {    // Rising edge of inc1
            if (inc2.read() == 0) {
                // Clockwise rotation: increase target RPM
                target_rpm += 100;  // Increase target RPM by 100
            } else {
                // Counterclockwise rotation: decrease target RPM
                target_rpm -= 100;  // Decrease target RPM by 100
            }

            if (target_rpm > max_rpm) target_rpm = max_rpm;
            if (target_rpm < 0) target_rpm = 0;

            // Writing the target RPM to the LCD.
            char buffer[16];
            sprintf(buffer, "Target RPM: %d", target_rpm);
            safe_lcd_write(buffer,0 );

        }
    }

    inc1_prev = inc1.read(); // Update previous state

    // Debounce
    wait_us(700);

}

// The closed loop mode logic is housed here. 
void handle_closed_loop_ctrl(){

    int rpm;
    rpm_mutex.lock();
    rpm = calculated_rpm;
    rpm_mutex.unlock();

    //Function to update the target_rpm global variable.
    calc_target_rpm();
    // Writing the target RPM to the LCD.

    float delta_t = 0.1f;
    int error = target_rpm - rpm;
    integral += error * delta_t;
    int derivative = error - prev_error;
    // Apply clamping to prevent windup
    if (integral > integral_max) integral = integral_max;
    if (integral < integral_min) integral = integral_min;

    float pid_output = (Kp * error) + (Ki * 0.0f) + (Kd * 0.00f);

    //current_duty_cycle += pid_output;
    //update_fan_speed(current_duty_cycle);

    //printf("Fan RPM: %d, Target RPM: %d, calculated PID Output is %.2f \nIntegral: %.2f, Derivative: %d, Error: %d\n", rpm, target_rpm, pid_output, integral, derivative, error);
    //printf("Fan RPM: %d, Target RPM: %d\n", rpm, target_rpm);
    prev_error = error;
    
}

void handle_open_loop_ctrl() {

    calc_target_rpm();

    // Calculate and set PWM duty cycle based on target RPM
    // Map target RPM from 0 to 3600 RPM to a duty cycle from 0% to 100%
    current_duty_cycle = static_cast<float>(target_rpm) / max_rpm;

    // Set the adjusted duty cycle to the fan
    fan.write(current_duty_cycle);

    // Output the adjusted duty cycle
    char open_buffer[16];
    sprintf(open_buffer, "Duty Cycle: %.2f\n", current_duty_cycle);
    safe_lcd_write(open_buffer,1);
}

// Function to handle AUTO mode
void handle_auto_mode() {
    // Placeholder for auto mode logic
    lcd.clear();
    lcd.writeLine("Auto", 0);
    // Add auto control logic here
}


// Function to update the current state based on button presses
void update_state() {
    static int last_button_state = 1;
    static Timer debounce_timer;
    debounce_timer.start();

    int button_state = button.read();
    if (button_state == 0 && last_button_state == 1 && debounce_timer.elapsed_time().count() > 100000) {
        debounce_timer.reset();

        // Cycle through the states
        current_mode = static_cast<FanMode>((current_mode + 1) % 4);

        // Display current mode on the LCD
        switch (current_mode) {
            case OFF: lcd.writeLine("Mode: Off", 0); break;
            case ENCDR_C_LOOP: lcd.writeLine("Mode: Closed Loop", 0); break;
            case ENCDR_O_LOOP: lcd.writeLine("Mode: Open Loop", 0); break;
            case AUTO: lcd.writeLine("Mode: Auto", 0); break;
        }

    }
    last_button_state = button_state;

}

// Interrupt service routine to count tachometer pulses
void count_pulse() {
    CriticalSectionLock lock;
    pulse_count++;
    led = !led;
}

// Main program
int main() {
    button.mode(PullUp);
    //mypc.set_format(8, BufferedSerial::None, 1); // Set serial format
    printf("Starting fan control with encoder\n");

    // Initialize PWM for the fan
    fan.period(0.02f);   

    // Attach interrupt for the tachometer
    fan_tacho.rise(&count_pulse); // Count rising edges

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
        printf("Current Mode is %d\n", current_mode);
        
    }

}

