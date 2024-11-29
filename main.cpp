#include "mbed.h"
#include "LCD_ST7066U.h"
#include "mRotaryEncoder.h"

// Global variables and constants
volatile int pulse_count = 0;       // Counts tachometer pulses
volatile int pulse_per_second = 0;  // Pulses in the last second
int last_encoder_value = 0;         // Last known encoder position
volatile float current_duty_cycle = 0.0f;    // Initial duty cycle

volatile bool encoder_flag = false;

// PID control parameters
float Kp = 0.000041;
float Ki = 0.0000;
float Kd = 0.0000;

float filtered_rpm = 0.0f;
float prev_error = 0.0;
float integral = 0.0;

const float integral_max = 500.0;
const float integral_min = -500.0;
const int MAX_RPM = 1850; // Maximum RPM corresponding to 100% duty cycle
const float MIN_DUTY_CYCLE = 0.01f;

volatile float open_duty_cycle = 0;
volatile int global_rpm = 0; // Global variable for the RPM of the fan

Mutex lcd_mutex;

// Custom clamp function
template <typename T>
T clamp(T value, T min_val, T max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

LCD lcd(PB_15, PB_14, PB_10, PA_8, PB_2, PB_1); // Instantiated LCD
mRotaryEncoder encoder(PA_1, PA_4, NC, PullUp, 2000, 1, 1); // Rotary encoder setup

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

Timer rpm_timer, print_timer;
FanMode current_mode = OFF;

volatile float global_dc = 0;

void encoder_interrupt_handler() {
    encoder_flag = true; // Set the flag
    // Perform minimal operations here (e.g., debouncing logic)
}

void count_pulse() {
    static uint32_t last_fall_time = 0;  // Time of the last falling edge
    uint32_t current_time = osKernelGetTickCount();  // Current kernel tick count (in ms)

    // Calculate the time difference between the current and last falling edges
    uint32_t elapsed_time = current_time - last_fall_time;

    // Restart the timer (update the last falling edge time)
    last_fall_time = current_time;

    // Only count a pulse if the elapsed time is greater than 15 ms
    if (elapsed_time > 15) {
        pulse_count++;  // Increment pulse count
    }
}



int calculate_rpm() {
    static uint32_t last_calc_time = 0;
    static int last_rpm = 0;

    uint32_t current_time1 = osKernelGetTickCount(); // Current time in ms
    uint32_t time_diff = current_time1 - last_calc_time; // Time difference in ms

    if (time_diff >= 1000) { // Calculate RPM every second (1000 ms)
        int rpm;
        {
            CriticalSectionLock lock; // Prevent interrupt interference
            rpm = (pulse_count * 30); // Calculate RPM
            pulse_count = 0; // Reset pulse count for next calculation
        }
        last_calc_time = current_time1; // Update the last calculation time

        return rpm > 50 ? rpm : 0; // Filter out low RPM values (less than 50)
    }

    return last_rpm; // Return the last valid RPM if a second hasn't passed
}


// Update fan speed based on duty cycle
void update_fan_speed(float duty_cycle) {
    fan.write(duty_cycle);
}

// Function to safely write to the LCD
void safe_lcd_write(const char* text, int line) {
    static char last_text[2][17] = { "", "" }; // Adjust size for two lines, 16 chars + null terminator

    lcd_mutex.lock();
    if (strncmp(last_text[line], text, 16) != 0) { // Compare with last written text
        strncpy(last_text[line], text, 16);      // Update the stored text
        last_text[line][16] = '\0';              // Ensure null termination

        char padded_text[17] = { ' ' };          // Create a blank-padded string
        strncpy(padded_text, text, 16);          // Copy the text to the padded string
        padded_text[16] = '\0';                  // Ensure null termination

        lcd.writeLine(padded_text, line);        // Write the padded string to the LCD
    }
    lcd_mutex.unlock();
}

// Update rotary encoder target RPM
int calc_target_rpm() {
    int encoder_value = encoder.Get();          // Read encoder position
    int encoder_diff = encoder_value - last_encoder_value; // Calculate change
    static int local_target_rpm = 0;

    if (encoder_diff != 0) {
        local_target_rpm += encoder_diff * 10; // Adjust RPM by 25 per encoder step
        local_target_rpm = clamp(local_target_rpm, 0, MAX_RPM);

        // Update LCD and log
        char buffer[16];
        sprintf(buffer, "Target RPM: %d", local_target_rpm);
        safe_lcd_write(buffer, 0);
    }

    last_encoder_value = encoder_value; // Update last position
    return local_target_rpm;
}

// Closed-loop control logic
void handle_closed_loop_ctrl() {

    static uint32_t last_iteration = 0;  // Time of the last falling edge
    uint32_t current_time2 = osKernelGetTickCount();  // Current kernel tick count (in ms)

    // Calculate the time difference between the current and last falling edges
    uint32_t elapsed_time = current_time2 - last_iteration;

    fan_tacho.fall(&count_pulse); // Set tachometer interrupt

    static int valid_rpm = 0;
    //fan_tacho.fall(&count_pulse); // Set tachometer interrupt
    static int c_target_rpm = 800;

    c_target_rpm = calc_target_rpm(); // Update target RPM

    

    if (elapsed_time >= 1000)
    {

        int rpm = calculate_rpm();

        if (rpm > 0)
        {
            valid_rpm = rpm;
        }
        int error = c_target_rpm - valid_rpm;
        if (error < 40) error = 0;

        // PID calculations
        float pid_output = (Kp * error) + (Ki * 0.0f) + (Kd * 0.0f);
        

        current_duty_cycle += pid_output;

        update_fan_speed(current_duty_cycle);


        
        
        prev_error = error;

        printf("Current pid output: %.2f, current duty cycle: %.2f, error: %d, rpm: %d\n", pid_output, current_duty_cycle, error, valid_rpm);
        last_iteration = current_time2;
    }
    
    
}

#include <cmath> // For exp function

void handle_open_loop_ctrl() {
    fan_tacho.fall(&count_pulse); // Set tachometer interrupt

    static int t_rpm = 0;
    static int last_pulse_count = 0;

    t_rpm = calc_target_rpm(); // Update target RPM

    // Apply the new equation: y = 0.0931 * e^(0.0013 * x)
    float duty_cycle = 0.0931f * exp(0.0013f * t_rpm);

    // Clamp the duty cycle to a reasonable range (0.0 to 1.0 or within any hardware limits)
    open_duty_cycle = clamp(duty_cycle, MIN_DUTY_CYCLE, 1.0f);
    global_dc = open_duty_cycle; // Update the global duty cycle variable

    update_fan_speed(open_duty_cycle); // Update fan speed with the new duty cycle
    global_rpm = calculate_rpm(); // Calculate and update the global RPM variable

    // Calculate pulses per second
    int pulse_diff = pulse_count - last_pulse_count;
    pulse_per_second = pulse_diff; // Update pulses per second

    last_pulse_count = pulse_count; // Store the current pulse count for the next cycle

    if (global_rpm > 0) {
        printf("Target RPM: %d, Duty Cycle: %.2f, Current RPM: %d, Pulses/Second: %d\n", 
               t_rpm, open_duty_cycle, global_rpm, pulse_per_second);
    }

    ThisThread::sleep_for(1ms); // Add a small delay to avoid overloading the system
}



void handle_auto_ctrl() {
    handle_closed_loop_ctrl();
}

void handle_off_ctrl() {
    fan.write(0.0f);
}

// Button press handler to switch between modes
void update_mode() {
    static Timer debounce_timer;
    debounce_timer.start();

    static int last_button_state = 1;  // Last button state
    int button_state = button.read();  // Read button state

    if (button_state == 0 && last_button_state == 1 && debounce_timer.elapsed_time().count() > 100000) {
        debounce_timer.reset();
        current_mode = static_cast<FanMode>((current_mode + 1) % 4);

        // Update LCD mode display
        switch (current_mode) {
            case OFF:
                safe_lcd_write("M: OFF", 0);
                break;
            case ENCDR_C_LOOP:
                safe_lcd_write("M: Closed Loop", 0);
                break;
            case ENCDR_O_LOOP:
                safe_lcd_write("M: Open Loop", 0);
                break;
            case AUTO:
                safe_lcd_write("M: AUTO", 0);
                break;
        }
    }

    last_button_state = button_state;  // Update last button state
}

int main() {
    fan.period(0.005f);  // Set PWM period 200Hz frequency
    fan.write(0.0f);     // Start with fan off

    safe_lcd_write("M: OFF", 0); // Display initial mode
    rpm_timer.start();  // Start RPM timer
    print_timer.start(); // Start serial print timer

    fan_tacho.fall(&count_pulse); // Set tachometer interrupt
    

    while (true) {
        if (encoder_flag) {
            encoder_flag = false; // Reset the flag
            calc_target_rpm();    // Update target RPM
        }

        update_mode();  // Update mode based on button presses

        switch (current_mode) {
            case OFF:
                handle_off_ctrl();
                break;
            case ENCDR_C_LOOP:
                handle_closed_loop_ctrl();
                break;
            case ENCDR_O_LOOP:
                handle_open_loop_ctrl();
                break;
            case AUTO:
                handle_auto_ctrl();
                break;
        }

        ThisThread::sleep_for(10ms);
    }
}
