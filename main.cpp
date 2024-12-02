#include "mbed.h"
#include "LCD_ST7066U.h"
#include "mRotaryEncoder.h"
#include <cmath> // For mathematical operations

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

volatile float open_duty_cycle = 0.3;
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
mRotaryEncoder encoder(PA_1, PA_4, NC, PullUp, 200, 1, 0); // Rotary encoder setup

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


volatile uint32_t start_time = 0;        // Time when counting starts
volatile uint32_t end_time = 0;          // Time when counting ends
volatile uint32_t last_pulse_time = 0;   // Time when the last pulse was received
volatile bool rpm_ready = false;   


void count_pulse() {
    static uint32_t last_fall_time = 0;  // Time of the last falling edge
    uint32_t current_time = osKernelGetTickCount();  // Current kernel tick count in ms

    // Calculate the time difference between the current and last falling edges
    uint32_t elapsed_time = current_time - last_fall_time;

    last_fall_time = current_time;  // Update the last falling edge time
    last_pulse_time = current_time;

    // Only process the pulse if the elapsed time is greater than 15 ms (debouncing)
    if (elapsed_time > 15) {
        
        if (pulse_count == 0) {
            start_time = current_time;  // Record the start time only when pulse_count is zero
        }

        pulse_count++;  // Increment pulse count

        if (pulse_count == 5) {         // If 4 pulses have been counted
            end_time = current_time;    // Record the end time
            rpm_ready = true;           // Set flag indicating RPM can be calculated
            pulse_count = 0;            // Reset pulse count for the next measurement
        }
    }
}

int calculate_rpm() {
    static int last_rpm = 0;
    static uint32_t last_calculation_time = 0; // Time when the last RPM was calculated

    // Local copies of shared variables to prevent data inconsistency
    uint32_t local_start_time = 0, local_end_time = 0;
    bool local_rpm_ready = false;

    uint32_t current_time = osKernelGetTickCount(); // Current time in ms

    // Safely copy shared variables with interrupts disabled
    {
        CriticalSectionLock lock; // Disable interrupts during copy
        local_rpm_ready = rpm_ready;
        if (rpm_ready) {
            local_start_time = start_time;
            local_end_time = end_time;
            rpm_ready = false;  // Reset the flag
        }
    }

    if (local_rpm_ready) {
        uint32_t time_diff_ms = local_end_time - local_start_time;  // Time for 4 pulses in ms

        if (time_diff_ms > 0) {
            // Assuming 2 pulses per revolution
            // RPM = (2 revolutions * 60000 ms per minute) / time_diff_ms
            int rpm = static_cast<int>((2.0f * 60000.0f) / time_diff_ms + 0.5f);

            last_rpm = rpm;
            last_calculation_time = current_time; // Update the time of the last RPM calculation
            return rpm;
        }
    }

    // No new RPM calculation available
    uint32_t time_since_last_calculation = current_time - last_calculation_time;

    if (time_since_last_calculation > 400) { // Adjust the timeout threshold as needed
        // No new RPM calculations for longer than the threshold; set RPM to zero
        last_rpm = 0;
        last_calculation_time = current_time; // Update last_calculation_time to reset the timeout
    }

    return last_rpm;  // Return the last known RPM or zero if timeout has occurred
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

    // Calculate step size based on the target RPM
    // The step size decreases as RPM decreases, allowing finer control at lower speeds
    float rpm_scaling_factor = 1.0f;

    if (local_target_rpm < 50) {
        rpm_scaling_factor = 0.1f; // Fine adjustments for low RPM
    } else if (local_target_rpm < 200) {
        rpm_scaling_factor = 0.2f; // Fine adjustments for low RPM
    } else if (local_target_rpm < 500) {
        rpm_scaling_factor = 0.5f; // Fine adjustments for low RPM
    } else if (local_target_rpm < 1000) {
        rpm_scaling_factor = 1.0f; // Moderate adjustments
    }

    // Adjust target RPM based on encoder change and scaling factor
    if (encoder_diff != 0) {
        local_target_rpm += static_cast<int>(encoder_diff * 10 * rpm_scaling_factor); // Adjust RPM based on scaled encoder change
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



void handle_open_loop_ctrl() {
    // Remove the tachometer interrupt setup from here
    // It's now set up once in main()

    // Static variables to hold previous values for comparison
    static int prev_t_rpm = -1;
    static int prev_global_rpm = -1;
    static int prev_duty_cycle_percent = -1; // Duty cycle stored as an integer percentage

    // Update target RPM from the encoder
    int t_rpm = calc_target_rpm(); // Ensure this returns a valid integer

    // Apply the quadratic equation to calculate the duty cycle
    float duty_cycle = (2e-7f * t_rpm * t_rpm) + (1e-4f * t_rpm) + 0.07f;

    // Clamp the duty cycle to a reasonable range (e.g., 0.0 to 1.0)
    open_duty_cycle = clamp(duty_cycle, MIN_DUTY_CYCLE, 1.0f);
    global_dc = open_duty_cycle; // Update the global duty cycle variable

    // Update fan speed with the new duty cycle
    update_fan_speed(open_duty_cycle);

    // Calculate and update the global RPM variable
    int global_rpm = calculate_rpm();

    // Convert duty cycle to an integer percentage for comparison
    int duty_cycle_percent = static_cast<int>(open_duty_cycle * 100 + 0.5f); // Round to nearest integer

    // Only print if any of the values have changed
    if (t_rpm != prev_t_rpm || duty_cycle_percent != prev_duty_cycle_percent || global_rpm != prev_global_rpm) {
        printf("Target RPM: %d, Duty Cycle: %.2f, Current RPM: %d\n",
               t_rpm, open_duty_cycle, global_rpm);

        // Update previous values
        prev_t_rpm = t_rpm;
        prev_duty_cycle_percent = duty_cycle_percent;
        prev_global_rpm = global_rpm;
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