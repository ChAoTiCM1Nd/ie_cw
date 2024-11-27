#include "mbed.h"
#include "LCD_ST7066U.h"
#include "mRotaryEncoder.h"

volatile int pulse_count = 0;       // Counts tachometer pulses
volatile int pulse_per_second = 0;  // Pulses in the last second
//volatile int target_rpm = 1000;     // Initial target RPM
int last_encoder_value = 0;         // Last known encoder position
volatile float current_duty_cycle = 0.0f;    // Initial duty cycle

// PID control parameters
float Kp = 0.0001;
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
DigitalOut pwm_sync(PB_3);          // DigitalOut to synchronize PWM state
InterruptIn fan_tacho(PA_0);        // Tachometer input to count pulses
BufferedSerial mypc(USBTX, USBRX, 19200);
DigitalIn button(BUTTON1);

Timer rpm_timer, print_timer;
FanMode current_mode = OFF;

volatile float global_dc = 0;

// Tachometer pulse counting logic
void count_pulse() {
    //CriticalSectionLock lock;
    //pulse_count++;  // Increment pulse count

    static uint32_t last_time = 0;  // Time of the last valid pulse
    uint32_t current_time = osKernelGetTickCount();

    if (current_time - last_time > 8) 
    {
        CriticalSectionLock lock;
        pulse_count++;  // Increment count
        last_time = current_time;  // Update last pulse time
    }

}

    /*
    if ((current_time - last_time > 15) && (global_dc >= 0.5)) {  // Minimum time between pulses
       

       CriticalSectionLock lock;
        pulse_count++;  // Increment count
        last_time = current_time;  // Update last pulse time
       }

    else if (   (0.4 < global_dc < 0.5) && (current_time - last_time > 20) ){
    
        CriticalSectionLock lock;
        pulse_count++;  // Increment count
        last_time = current_time;  // Update last pulse time

    }
    else if (0.3 < global_dc < 0.4 && current_time - last_time > 50 ){
    
        CriticalSectionLock lock;
        pulse_count++;  // Increment count
        last_time = current_time;  // Update last pulse time
    }
    else if ((0.2 < global_dc < 0.3) && (current_time - last_time > 150) ){
    
        CriticalSectionLock lock;
        pulse_count++;  // Increment count
        last_time = current_time;  // Update last pulse time
    }

    else if ((0.01 < global_dc < 0.2) && (current_time - last_time > 250) ){
    
        CriticalSectionLock lock;
        pulse_count++;  // Increment count
        last_time = current_time;  // Update last pulse time
    }
*/
    /*
    static uint32_t last_time = 0;  // Time of the last valid pulse
    uint32_t current_time = osKernelGetTickCount();  // Get current system time in ticks (milliseconds)

    if (pwm_sync.read() == 1) {  // Count only during PWM high phase
        if (current_time - last_time > 5) {  // Filter out pulses that occur in rapid succession (<5ms apart)
            pulse_count++;  // Increment pulse count
            led = !led;      // Toggle LED on each valid pulse
            last_time = current_time;  // Update last pulse time
        }
    }
    */


// Function to calculate RPM
int calculate_rpm() {
    static uint32_t last_pulse_time = osKernelGetTickCount();  // Time of the last pulse in milliseconds
    uint32_t current_time = osKernelGetTickCount();  // Current system time in milliseconds

    // Calculate time elapsed in seconds
    float time_elapsed = (current_time - last_pulse_time) / 1000.0f;
    static int local_pulse_count = 0;

    // If more than 1 second has passed, calculate RPM
    if (time_elapsed >= 1.0f) {
        // Each rotation gives 2 pulses, so RPM = (pulse_count * 60) / (time_elapsed * 2)
        int rpm;
        {
            CriticalSectionLock lock; // Atomic access to `pulse_count`
            rpm = (pulse_count * 60) / (time_elapsed * 2);  // Calculate RPM
            local_pulse_count = pulse_count;
            //printf("pulse count: %d\n", pulse_count);
            pulse_count = 0; // Reset after reading
        }
        //int rpm = (pulse_count * 60) / (time_elapsed * 2); 

        //printf("Current RPM is: %d, pulse count: %d, time elapsed: %.2f\n", rpm, local_pulse_count, time_elapsed);

        last_pulse_time = current_time;  // Update the time for the next calculation

        // Apply a simple low-pass filter for stability
        //filtered_rpm = 0.8f * filtered_rpm + 0.2f * rpm;

        //printf(" filtered RPM is: %.2f, rpm: %d, pulse count: %d, time elapsed: %.2f\n", filtered_rpm, rpm, local_pulse_count, time_elapsed);

        // Return filtered RPM (if it’s above a threshold, otherwise 0)
        //return (filtered_rpm > 50) ? (int)filtered_rpm : 0;
        return rpm;
    } else {
        return 0;  // Default to 0 RPM if not enough time has passed
    }
}

void start_fan() 
{   
    fan.write(0.5f);
    ThisThread::sleep_for(500ms);  // Wait for 500ms
    fan.write(MIN_DUTY_CYCLE);  // Drop to minimum duty cycle
}

// Update fan speed based on duty cycle
void update_fan_speed(float duty_cycle) {
    //duty_cycle = clamp(duty_cycle, 0.0f, 1.0f);
    fan.write(duty_cycle);


    /*
    static float current_time = 0.0f;  // Time within the PWM period
    const float pwm_period = 1.0f;    // PWM period in seconds

    // Simulate PWM timing
    current_time += 0.01f; // Update time (assuming 10ms step in loop)
    if (current_time >= pwm_period) current_time -= pwm_period;

    // pwm_sync is true for the portion of time corresponding to the duty cycle
    pwm_sync = (current_time < duty_cycle * pwm_period) ? 1 : 0;
    */

    //printf("current duty cycle: %.2f\n", duty_cycle);
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
        local_target_rpm += encoder_diff * 25; // Adjust RPM by 25 per encoder step
        local_target_rpm = clamp(local_target_rpm, 0, MAX_RPM);

        // Update LCD and log
        char buffer[16];
        sprintf(buffer, "Target RPM: %d", local_target_rpm);
        safe_lcd_write(buffer, 0);
    }

    last_encoder_value = encoder_value; // Update last position
    return local_target_rpm;
}

/*
// Print serial output
void print_serial_output() {
    if (print_timer.elapsed_time().count() > 1'000'000) { // Every second
        int rpm = calculate_rpm();
        printf("Mode: %d, Pulses per second: %d, Target RPM: %d, Measured RPM: %d\n", 
               current_mode, pulse_count, target_rpm, rpm); // Added pulse_count to the serial output
        pulse_count = 0; // Reset pulse counter
        print_timer.reset();
    }
}*/

// Closed-loop control logic
void handle_closed_loop_ctrl() {
    static int c_target_rpm = 0;

    c_target_rpm = calc_target_rpm(); // Update target RPM

    //static float pid_output =0;
    //static int rpm = 0;
    //static int error = 0;

    //static float current_duty_cycle = 0;
    int rpm = calculate_rpm();
    int error = c_target_rpm - rpm;

    // PID calculations
    //float delta_t = 1.0f;
    //integral = clamp(integral + error * delta_t, integral_min, integral_max);
    //float derivative = (error - prev_error) / delta_t;
    float pid_output = (Kp * error) + (Ki * 0.0f) + (Kd * 0.0f);

    //current_duty_cycle += pid_output;
    
    //update_fan_speed(pid_output);

    //printf("PID output: %.2f, current duty cycle: %.2f, error: %d, rpm: %d\n", pid_output, current_duty_cycle, error, rpm);

    prev_error = error;
}

// Open-loop control logic (no feedback)
void handle_open_loop_ctrl() {

    fan_tacho.rise(&count_pulse);  // Set tachometer interrupt

    static int t_rpm = 0;

    //t_rpm = 1200;

    int o_rpm = 0;

    o_rpm = calculate_rpm();

    t_rpm = calc_target_rpm();  // Update target RPM

    open_duty_cycle = static_cast<float>(t_rpm) / MAX_RPM;  // Simple open-loop control
    //update_fan_speed(clamp(current_duty_cycle, 0.0f, 1.0f));

    global_dc = open_duty_cycle;

    update_fan_speed(open_duty_cycle);

    if(o_rpm >0)
    {
        printf("Target RPM: %d, duty cycle: %.2f, current rpm: %d\n", t_rpm, open_duty_cycle, o_rpm);
    }
    
    ThisThread::sleep_for(1ms); // Add a small delay to avoid overloading the system
}

// Auto mode (same as closed-loop)
void handle_auto_ctrl() {
    handle_closed_loop_ctrl();
}

// Turn off the fan in OFF mode
void handle_off_ctrl() {
    fan.write(0.0f);
    pwm_sync = 0;
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
    fan.period(0.001f);  // Set PWM period to 1ms (1kHz frequency)
    fan.write(0.0f);     // Start with fan off
    pwm_sync = 0;        // Set PWM synchronization to low initially

    safe_lcd_write("M: OFF", 0); // Display initial mode
    rpm_timer.start();  // Start RPM timer
    print_timer.start(); // Start serial print timer
    //fan_tacho.rise(&count_pulse);  // Set tachometer interrupt

    while (true) {
        update_mode();  // Update mode based on button presses

        // Handle fan control based on the current mode
        switch (current_mode) {
            case OFF:
                handle_off_ctrl();
                break;
            case ENCDR_C_LOOP:
                handle_closed_loop_ctrl();
                break;
            case ENCDR_O_LOOP:
                //kicking fan awake first
                //start_fan();
                handle_open_loop_ctrl();
                break;
            case AUTO:
                handle_auto_ctrl();
                break;
        }

        //print_serial_output();  // Print debug information
        ThisThread::sleep_for(10ms);  // Small delay to prevent overloading the CPU
    }
}
