#include "mbed.h"
#include "LCD_ST7066U.h"
#include "mRotaryEncoder.h"
#include <cmath> // For mathematical operations
#include "pid.h"

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

const int starting_rpm = 800;

volatile float open_duty_cycle = 0.3;
volatile int global_rpm = 0; // Global variable for the RPM of the fan


// PID parameters
float Kc = 1.0;            // Proportional gain
float tauI = 0.1f;          // Integral time constant (Ki), keep at zero for now
float tauD = 0.0f;          // Derivative time constant (Kd), keep at zero for now
float tSample = 0.1f;       // Sample interval in seconds (e.g., 0.1s for 100ms)

// Create PID object
PID pid(Kc, tauI, tauD, tSample);

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

DigitalOut led_2(PC_0);             //LED on external header.
DigitalOut led(LED1);
PwmOut fan(PB_0);                   // PWM control for the fan
InterruptIn fan_tacho(PA_0);        // Tachometer input to count pulses
BufferedSerial mypc(USBTX, USBRX, 19200);
DigitalIn button(BUTTON1);

Timer rpm_timer, print_timer;
FanMode current_mode = OFF;

volatile uint32_t start_time = 0;        // Time when counting starts
volatile uint32_t end_time = 0;          // Time when counting ends
volatile bool rpm_ready = false;         // Flag indicating RPM is ready to be calculated
volatile float global_dc = 0;

void encoder_interrupt_handler() {
    encoder_flag = true; // Set the flag
}

void count_pulse() {
    static uint32_t last_fall_time = 0;  // Time of the last falling edge
    uint32_t current_time = osKernelGetTickCount();  // Current kernel tick count (in ms)

    // Calculate the time difference between the current and last falling edges
    uint32_t elapsed_time = current_time - last_fall_time;

    // Restart the timer (update the last falling edge time)
    last_fall_time = current_time;

    // Only process the pulse if the elapsed time is greater than 15 ms (debouncing)
    if (elapsed_time > 15) {

        if (pulse_count == 0) {
            start_time = current_time;  // Record the start time only when pulse_count is zero
        }

        pulse_count++;  // Increment pulse count

        if (pulse_count == 4) {         // If 4 pulses have been counted
            end_time = current_time;    // Record the end time
            rpm_ready = true;           // Set flag indicating RPM can be calculated
            pulse_count = 0;            // Reset pulse count for the next measurement
        }
    }
}

int calculate_rpm() {
 
    static int last_rpm = 0;
    uint32_t local_start_time = 0, local_end_time = 0;
    bool local_rpm_ready = false;

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
            int rpm = static_cast<int>((2.0f * 60000.0f) / time_diff_ms + 0.5f) * 0.75;  // Calculate RPM and round
            last_rpm = rpm;
            return rpm;
        }
    }

    return last_rpm;  // Return the last valid RPM if no new data


}


// Update fan speed based on duty cycle
void update_fan_speed(float duty_cycle) {
    fan.write(duty_cycle);
}

// Function to safely write to the LCD
void safe_lcd_write(const char* text, int line) {
    static char last_text[2][17] = { "", "" }; // Adjust size for two lines, 16 chars + null terminator

    lcd_mutex.lock();
    if (strncmp(last_text[line], text, 16) != 0) {      // Compare with last written text
        strncpy(last_text[line], text, 16);             // Update the stored text
        last_text[line][16] = '\0';                     // Ensure null termination

        char padded_text[17];                           // Create a blank-padded string
        memset(padded_text, ' ', 16);                   // Fill with spaces
        padded_text[16] = '\0';                         // Ensure null termination

        // Copy the new text into the padded_text buffer
        strncpy(padded_text, text, strlen(text));

        lcd.writeLine(padded_text, line);        // Write the padded string to the LCD
    }
    lcd_mutex.unlock();
}

// Update rotary encoder target RPM
int calc_target_rpm() {

    static int local_target_rpm = 119;

    // Calculate step size based on the target RPM
    // The step size decreases as RPM decreases, allowing finer control at lower speeds
    static float rpm_scaling_factor = 0;

    int encoder_value = encoder.Get();          // Read encoder position
    int encoder_diff = encoder_value - last_encoder_value; // Calculate change

    
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
        safe_lcd_write(buffer, 1);
    }

    last_encoder_value = encoder_value; // Update last position
    return local_target_rpm;
}


void handle_closed_loop_ctrl() {

    static Timer control_timer;
    static int c_target_rpm = 800;
    static bool timer_started = false;

    // Update target RPM from encoder
    c_target_rpm = calc_target_rpm();
    // Read current RPM (ensure calculate_rpm() returns a float)
    int rpm = calculate_rpm();
    
    if (!timer_started) {
        control_timer.start();
        timer_started = true;
    }

    if (control_timer.elapsed_time().count() >= tSample * 1e6) {  // tSample in seconds, convert to microseconds
        control_timer.reset();

        
        // Update PID controller
        pid.setProcessValue(static_cast<float>(rpm));
        pid.setSetPoint(static_cast<float>(c_target_rpm));

        // Compute control output (duty cycle) 
        float duty_cycle = pid.compute();

        printf("Setpoint: %d RPM, RPM: %d, Duty Cycle: %.3f\n", c_target_rpm, rpm, duty_cycle);

        // Clamp duty cycle
        duty_cycle = clamp(duty_cycle, 0.0f, 1.0f);

        // Update fan speed
        update_fan_speed(duty_cycle);
        
    }
}

void handle_open_loop_ctrl() {
    fan_tacho.fall(&count_pulse); // Set tachometer interrupt

    static int t_rpm = 0;
    static int last_pulse_count = 0;

    t_rpm = calc_target_rpm(); // Update target RPM

    // Apply the new quadratic equation: y = 2E-07x^2 + 2E-05x + 0.1063
    float duty_cycle = (2e-7f * t_rpm * t_rpm) + (1e-4f * t_rpm) + 0.07;

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
    //handle_closed_loop_ctrl();
    fan.write(0.0f);
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

    pid.setInputLimits(0.0f, MAX_RPM);  // MAX_RPM is 1850 in your code
    pid.setOutputLimits(0.0f, 1.0f);    // Duty cycle ranges from 0.0 to 1.0
    pid.setMode(1);  // 1 for automatic mode

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