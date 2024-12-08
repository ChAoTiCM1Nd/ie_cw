#include "mbed.h"
#include "LCD_ST7066U.h"
#include "mRotaryEncoder.h"
#include <cmath> // For mathematical operations
#include <cstdio>
#include "pid.h"

// Global variables and constants
volatile int pulse_count = 0;       // Counts tachometer pulses
volatile int pulse_per_second = 0;  // Pulses in the last second
int last_encoder_value = 0;         // Last known encoder position
volatile float current_duty_cycle = 0.0f;    // Initial duty cycle

volatile bool encoder_flag = false;


float temp_tSample = 0.1f;    // Sample interval in seconds (e.g., 0.1s for 100ms)

const float integral_max = 500.0;
const float integral_min = -500.0;
const int MAX_RPM = 1850; // Maximum RPM corresponding to 100% duty cycle
const float MIN_DUTY_CYCLE = 0.001f;

const int starting_rpm = 800;

volatile float open_duty_cycle = 0.3;
volatile int global_rpm = 0; // Global variable for the RPM of the fan

volatile int global_encoder_pos = 0;


static char temp_data;

// PID parameters
float Kc = 0.8;            // Proportional gain
float tauI = 0.8f;          // Integral time constant (Ki), keep at zero for now
float tauD = 0.001f;          // Derivative time constant (Kd), keep at zero for now
float tSample = 0.1f;       // Sample interval in seconds (e.g., 0.1s for 100ms)

// Create PID object
PID pid(Kc, tauI, tauD, tSample);

I2C i2c(PB_9, PB_8);

const int addr1 = 0x9A; //Normal operating address of sensor
const int addr2 = 0x9A << 1; //Write address of sensor
char TEMP_REG = 0x00; // Register to access temperature dat

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
    AUTO,
    CALIB
};

DigitalOut led_bi_A(PB_7);
DigitalOut led_bi_B(PA_15);
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
    static uint32_t elapsed_time = 0;

    // Calculate the time difference between the current and last falling edges
    elapsed_time = current_time - last_fall_time;

    // Restart the timer (update the last falling edge time)
    last_fall_time = current_time;

    
    // Only process the pulse if the elapsed time is greater than 15 ms (debouncing)
    if (elapsed_time > 15) {

        if (pulse_count == 0) {
            start_time = current_time;  // Record the start time only when pulse_count is zero
        }

        pulse_count++;  // Increment pulse count

        if (pulse_count == 3) {         // If 4 pulses have been counted
            end_time = current_time;    // Record the end time
            rpm_ready = true;           // Set flag indicating RPM can be calculated
            pulse_count = 0;            // Reset pulse count for the next measurement
        }
    }
}

int calculate_rpm() {
    static int last_rpm = 0;
    static uint32_t last_calculation_time = 0; // Time of the last RPM calculation
    static int last_encoder_value = 0;        // Last encoder position value
    uint32_t current_time = osKernelGetTickCount(); // Current time in ms

    // Local copies of shared variables to prevent data inconsistency
    uint32_t local_start_time = 0, local_end_time = 0;
    static bool local_rpm_ready = false;
    static bool local_encoder_active = false; // Flag to check if the encoder is active
    static int encoder_value = 0; // Read encoder position
    static int encoder_diff = 0; // Calculate encoder change

    static int rpm = 0;

    // Safely copy shared variables with interrupts disabled
    {
        CriticalSectionLock lock; // Disable interrupts during copy
        local_rpm_ready = rpm_ready;

        // Check if the encoder has moved (i.e., if the encoder position has changed)
        encoder_value = encoder.Get();
        encoder_diff = encoder_value - last_encoder_value;
        
        // If encoder value has changed recently, it means the encoder is active
        local_encoder_active = (encoder_diff != 0);
        if (rpm_ready) {
            local_start_time = start_time;
            local_end_time = end_time;
            rpm_ready = false;  // Reset the flag
        }

        // Update the last encoder value for the next check
        last_encoder_value = encoder_value;
    }

    // If the encoder was active recently, skip RPM calculation
    if (local_encoder_active) {
        return last_rpm; // Return the last valid RPM without recalculating
    }
    else if (local_rpm_ready) { // Proceed with RPM calculation only if RPM data is ready
        uint32_t time_diff_ms = local_end_time - local_start_time;  // Time for 4 pulses in ms

        if (time_diff_ms > 0) {
            rpm = static_cast<int>((1.0f * 60000.0f) / time_diff_ms + 0.5f);    // Assuming 2 pulses per revolution
            last_calculation_time = current_time; // Update calculation time

            if (rpm > 0) {
                last_rpm = rpm; // Update the last valid RPM
                // Debugging output for RPM
                return rpm;
            }
            else { 
                return last_rpm; // Return the last valid RPM if RPM is zero
            }
        }
    }
    else {  
        // If no valid RPM data has been calculated recently, handle it
        uint32_t time_since_last_calculation = current_time - last_calculation_time;
        if ((time_since_last_calculation > 1000)) { // Increased timeout to 3 seconds for inactivity
           last_rpm = 0; // Don't reset RPM to zero immediately
        }
    }

    return last_rpm;

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

    static int local_target_rpm = 800;

    // Calculate step size based on the target RPM
    // The step size decreases as RPM decreases, allowing finer control at lower speeds
    static float rpm_scaling_factor = 0;

    int encoder_value = encoder.Get();          // Read encoder position
    int encoder_diff = encoder_value - last_encoder_value; // Calculate change

    
    if (local_target_rpm < 200) {
        rpm_scaling_factor = 0.1f; // Fine adjustments for low RPM
    } else if (local_target_rpm < 300) {
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

    }

    last_encoder_value = encoder_value; // Update last position
    return local_target_rpm;
}


void handle_closed_loop_ctrl() {

    static Timer control_timer;
    static int c_target_rpm = 800;
    static bool timer_started = false;
    static int rpm = 0;
    float duty_cycle = 0;

    float current_temp = static_cast<float>(temp_data);

    // Update target RPM from encoder
    c_target_rpm = calc_target_rpm();
    // Read current RPM (ensure calculate_rpm() returns a float)
    rpm = calculate_rpm();
    
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
        duty_cycle = pid.compute();

        printf("Setpoint: %d RPM, RPM: %d, Duty Cycle: %.3f, Temp: %d\n", c_target_rpm, rpm, duty_cycle, temp_data);

        // Clamp duty cycle
        duty_cycle = clamp(duty_cycle, 0.0f, 1.0f);

        // Update fan speed
        update_fan_speed(duty_cycle);

        char buffer_line1[16];
        char buffer_line2[16];

        // Format the first line: "M: AL. TT = XX."
        sprintf(buffer_line1, "M: CL. T= %04d", c_target_rpm);  // Show target temperature on the first line

        safe_lcd_write(buffer_line1, 0); // Write to the first line of the LCD

        // Format the second line: "AT=XX. RPM= XXXX"
        sprintf(buffer_line2, "AT=%02d. RPM= %04d", static_cast<int>(current_temp), rpm);  // Show current temperature and RPM

        safe_lcd_write(buffer_line2, 1); // Write to the second line of the LCD

         // LED Control based on RPM
    if ((rpm <= 200 && rpm != 0) && (duty_cycle > 0)) {

        //Set Bidirectional LCD to red
        led_bi_A = 0; // red
        led_bi_B = 1; // green

        led_2 = 0;    // Turn off other LED
        wait_us(50000);
    } 
    else if (rpm > 1750) {

        //Set Bidirectional LCD to green
        led_bi_A = 1; 
        led_bi_B = 0; 

        led_2 = 0;    // Turn off other LED
        wait_us(50000);
    } 
    else if ((rpm == 0) && (duty_cycle > 0)){

        //Set Bidirectional LCD to red
        led_bi_A = 0; 
        led_bi_B = 1; 

        led_2 = 1;    // Turn on second LED 
        wait_us(50000);
    }
    else {

        //Turn off bidirectional LCD
        led_bi_A = 0; // red
        led_bi_B = 0; // off
        led_2 = 0;    // Turn off all LEDs
    }

    }
}

void handle_open_loop_ctrl() {
    // Remove the tachometer interrupt setup from here
    // It's now set up once in main()

    // Static variables to hold previous values for comparison
    static int prev_t_rpm = -1;
    static int prev_o_rpm = -1;
    static int prev_duty_cycle_percent = -1; // Duty cycle stored as an integer percentage
    static int t_rpm = 800;
    static int global_rpm = 0;

    float current_temp = static_cast<float>(temp_data);

    // Update target RPM from the encoder
    t_rpm = calc_target_rpm(); // Ensure this returns a valid integer

    // Apply the quadratic equation to calculate the duty cycle
    float duty_cycle = (2e-7f * t_rpm * t_rpm) + (8e-5f * t_rpm) + 0.08f;

    // Clamp the duty cycle to a reasonable range (e.g., 0.0 to 1.0)
    open_duty_cycle = clamp(duty_cycle, MIN_DUTY_CYCLE, 1.0f);
    global_dc = open_duty_cycle; // Update the global duty cycle variable

    // Update fan speed with the new duty cycle
    update_fan_speed(open_duty_cycle);

    // Calculate and update the global RPM variable
    global_rpm = calculate_rpm();

    // Convert duty cycle to an integer percentage for comparison
    int duty_cycle_percent = static_cast<int>(open_duty_cycle * 100 + 0.5f); // Round to nearest integer

    // Only print if any of the values have changed
    if (t_rpm != prev_t_rpm || duty_cycle_percent != prev_duty_cycle_percent || global_rpm != prev_o_rpm) {
        printf("Target RPM: %d, Duty Cycle: %.2f, Current RPM: %d, temp: %d\n",
               t_rpm, open_duty_cycle, global_rpm, temp_data);

        char buffer_line1[16];
        char buffer_line2[16];

        // Format the first line: "M: AL. TT = XX."
        sprintf(buffer_line1, "M: OL. T= %04d", t_rpm);  // Show target temperature on the first line

        safe_lcd_write(buffer_line1, 0); // Write to the first line of the LCD

        // Format the second line: "AT=XX. RPM= XXXX"
        sprintf(buffer_line2, "AT=%02d. RPM= %04d", static_cast<int>(current_temp), global_rpm);  // Show current temperature and RPM

        safe_lcd_write(buffer_line2, 1); // Write to the second line of the LCD

        // Update previous values
        prev_t_rpm = t_rpm;
        prev_duty_cycle_percent = duty_cycle_percent;
        prev_o_rpm = global_rpm;

    if ((global_rpm <= 200 && global_rpm != 0) && (duty_cycle > 0)) {

        //Set Bidirectional LCD to red
        led_bi_A = 0; 
        led_bi_B = 1; 

        led_2 = 0;    // Turn off second LED
        wait_us(50000);
    } 
    else if (global_rpm > 1750) {

        //Set Bidirectional LCD to green
        led_bi_A = 1; 
        led_bi_B = 0; 

        led_2 = 0;    // Turn off other LED
        wait_us(50000);
    } 
    else if ((global_rpm == 0) && (duty_cycle > 0)){

        //Set Bidirectional LCD to red
        led_bi_A = 0; 
        led_bi_B = 1; 

        led_2 = 1;    // Turn on second LED 
        wait_us(50000);
    }
    else {
        led_bi_A = 0; // red
        led_bi_B = 0; // off
        led_2 = 0;    // Turn off all LEDs
    }

    }

    ThisThread::sleep_for(1ms); // Add a small delay to avoid overloading the system
}



void handle_auto_ctrl() {
    static Timer temp_control_timer;
    static bool timer_started = false;
    static float duty_cycle = 0.0f;

    // PID parameters (manually tuned)
    static const float Kp = 0.1f;   // Proportional gain
    static const float Ki = 0.0f;  // Integral gain
    static const float Kd = 0.0f;  // Derivative gain

    // PID state variables
    static float prev_error = 0.0f;   // Error at the previous time step
    static float integral = 0.0f;    // Cumulative integral term
    static float derivative = 0.0f;  // Derivative term

    // Fan boost parameters
    const float BOOST_DUTY_CYCLE = 0.5f; // Boost duty cycle (20%)
    const float ERROR_THRESHOLD = 3.0f; // Error below which boost is triggered
    static bool boost_active = false;

    // Target temperature (adjustable with encoder)
    static int target_temp = 10;  // Start at 10 degrees
    static int last_encoder_value = 0;

    // Calculate RPM
    int rpm = calculate_rpm();

    // Read encoder input to update target temperature
    int encoder_value = encoder.Get(); // Replace encoder.Get() with your actual encoder reading function
    int encoder_diff = encoder_value - last_encoder_value;
    if (encoder_diff != 0) {
        target_temp += encoder_diff;  // Increment or decrement target temperature
        target_temp = clamp(target_temp, 0, 100);  // Clamp between 0 and 100 degrees
        last_encoder_value = encoder_value;       // Update last encoder value
    }

    // Start the timer for periodic updates
    if (!timer_started) {
        temp_control_timer.start();
        timer_started = true;
    }

    // Periodically update the PID controller
    if (temp_control_timer.elapsed_time().count() >= temp_tSample * 1e6) {  // temp_tSample in seconds, convert to microseconds
        temp_control_timer.reset();

        // Read the current temperature
        float current_temp = static_cast<float>(temp_data);

        // Calculate error (difference between current and target temperature)
        float error = current_temp - target_temp;

        if (error > 0.0f) {  // Only activate fan if the temperature is higher than the target
            // Compute proportional term
            float proportional = Kp * error;

            // Compute integral term (accumulate error over time)
            integral += error * temp_tSample;

            // Compute derivative term (rate of error change)
            derivative = (error - prev_error) / temp_tSample;

            // Compute total PID output
            float pid_output = proportional + (Ki * integral) + (Kd * derivative);

            // Update the previous error for the next cycle
            prev_error = error;

            // Convert PID output to duty cycle and clamp it to the valid range
            duty_cycle = clamp(pid_output, MIN_DUTY_CYCLE, 1.0f);

            // Trigger a brief boost if the error is below the threshold and the fan is not spinning
            if (error < ERROR_THRESHOLD && duty_cycle < BOOST_DUTY_CYCLE && !boost_active && rpm < 100) {
                duty_cycle = BOOST_DUTY_CYCLE; // Apply the boost
                boost_active = true;          // Mark boost as active
            } else {
                boost_active = false;         // Deactivate boost if conditions no longer apply
            }
        } else {
            // Turn off the fan if temperature is below or equal to target
            duty_cycle = 0.0f;
            prev_error = 0.0f;  // Reset previous error
            integral = 0.0f;    // Reset integral term
        }

        // Update fan speed
        update_fan_speed(duty_cycle);

        // Debugging output
        printf("Temp: %.1f C, Target Temp: %d C, Duty Cycle: %.3f, Error: %.2f, P: %.2f, I: %.2f, D: %.2f, RPM: %d\n",
               current_temp, target_temp, duty_cycle, error, Kp * error, Ki * integral, Kd * derivative, rpm);

        // Update the LCD
        char buffer_line1[16];
        char buffer_line2[16];

        // Format the first line: "M: AL. TT = XX."
        sprintf(buffer_line1, "M: AL. TT = %02d", target_temp);  // Show target temperature on the first line

        safe_lcd_write(buffer_line1, 0); // Write to the first line of the LCD

        // Format the second line: "AT=XX. RPM= XXXX"
        sprintf(buffer_line2, "AT=%02d. RPM= %04d", static_cast<int>(current_temp), rpm);  // Show current temperature and RPM

        safe_lcd_write(buffer_line2, 1); // Write to the second line of the LCD

    if ((rpm <= 200 && rpm != 0) && (duty_cycle > 0)) {

        //Set Bidirectional LCD to red
        led_bi_A = 0; 
        led_bi_B = 1; 
        led_2 = 0;    // Turn off second LED
        wait_us(50000);
    } 
    else if (rpm > 1750) {

        //Set Bidirectional LCD to green
        led_bi_A = 1; 
        led_bi_B = 0; 

        led_2 = 0;    // Turn off other LED
        wait_us(50000);
    } 
    else if ((rpm == 0) && (duty_cycle > 0)){
        //Set Bidirectional LCD to red
        led_bi_A = 0; 
        led_bi_B = 1; 

        led_2 = 1;    // Turn on second LED, set it to red
        wait_us(50000);
    }
    else {
        //Set Bidirectional LCD to off
        led_bi_A = 0; // red
        led_bi_B = 0; // off
        led_2 = 0;    // Turn off all LEDs
    }

    }
}



float calib_dc_step(int rpm) {

    float duty_cycle_step = 0;

    if (rpm < 200) {
        duty_cycle_step = 0.0001f; // Fine adjustments for low RPM
    } else if (rpm < 300) {
        duty_cycle_step = 0.0005f; // Fine adjustments for low RPM
    } else if (rpm < 500) {
        duty_cycle_step = 0.001f; // Fine adjustments for low RPM
    } else if (rpm < 1000) {
        duty_cycle_step = 0.005f; // Moderate adjustments
    }
    else{
        duty_cycle_step = 0.05;
    }

    return duty_cycle_step;


}


void handle_CALI_ctrl() {
    const int loading_bar_length = 16; // Length of the loading bar
    static bool CALIBRATING = true;   // Static flag to track calibration state
    static float duty_cycle = 1.0f;   // Start at 100% duty cycle
    static int step = 0;              // Tracks progress for the loading bar
    static char loading_bar[17] = ""; // 16 characters + null terminator
    static float duty_cycle_step = 0;

    static int calib_max_rpm = 0;
    static int calib_min_rpm = 0;

    static int rpm = 0;

    safe_lcd_write("Calibrating...  ", 0);

    if (CALIBRATING) {

        
        if (led_bi_A == 1) led_bi_A = 0; 
        if (led_bi_B == 0) led_bi_B = 1;


        

        // Update fan speed and wait for stabilization
        update_fan_speed(duty_cycle);
        wait_us(4000000);
        

        // Update loading bar progress
        step++;
        int filled_length = static_cast<int>((1.0f - duty_cycle) * loading_bar_length);
        for (int i = 0; i < loading_bar_length; i++) {
            loading_bar[i] = (i < filled_length) ? '#' : ' ';
        }
        loading_bar[loading_bar_length] = '\0'; // Null-terminate the string

        // Update the LCD with the loading bar
        safe_lcd_write(loading_bar, 1);
        
        wait_us(500000);
        rpm = calculate_rpm(); // Measure RPM
        


        if (duty_cycle == 1.0f) {
            // Display calibration message only at the beginning
            //safe_lcd_write("Calibrating...  ", 0);
            calib_max_rpm = rpm;
        }

        duty_cycle_step = calib_dc_step(rpm);
        // Decrease duty cycle
        duty_cycle -= duty_cycle_step;

        printf("Duty Cycle: %.3f, RPM: %d, dc_step: %.4f\n", duty_cycle, rpm,duty_cycle_step);

        // Check if calibration is complete
        if (duty_cycle < MIN_DUTY_CYCLE) {
            CALIBRATING = false; // Mark calibration as complete

            calib_min_rpm = rpm;
            update_fan_speed(0.0f); // Stop the fan

            // Display results on the LCD
            char buffer[16];
            sprintf(buffer, "Max RPM: %d", calculate_rpm());
            safe_lcd_write(buffer, 0);
            //sprintf(buffer, "Min RPM: %d", calib_max_rpm);
            //safe_lcd_write(buffer, 1);

            printf("Max RPM: %d, MIN RPM: %d", calib_max_rpm, calib_min_rpm);
        }
    }

    // If calibration is complete, keep the results on the LCD
    if (!CALIBRATING) {
        // Do nothing; the results will remain on the screen
        ThisThread::sleep_for(10ms); // Add a small delay to reduce CPU load
    }
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
        current_mode = static_cast<FanMode>((current_mode + 1) % 5);

        // Update LCD mode display
        switch (current_mode) {
            case OFF:
                safe_lcd_write("M: OFF          ", 0);
                safe_lcd_write("                ", 1);
                wait_us(1000000);
                break;
            case ENCDR_C_LOOP:
                safe_lcd_write("M: Closed Loop", 0);
                safe_lcd_write("                ", 1);
                wait_us(1000000);
                break;
            case ENCDR_O_LOOP:
                safe_lcd_write("M: Open Loop", 0);
                safe_lcd_write("                ", 1);
                wait_us(1000000);
                break;
            case AUTO:
                safe_lcd_write("M: AUTO", 0);
                safe_lcd_write("                ", 1);
                wait_us(1000000);
                break;
            case CALIB:
                safe_lcd_write("M: Calibration", 0);
                safe_lcd_write("                ", 1);
                wait_us(1000000);
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

        i2c.write(addr2, &TEMP_REG, 1); //Request data from sensor
        i2c.read(addr1, &temp_data, 1); //Read data from sensor
        
        update_mode();  // Update mode based on button presses

        //printf("Temp = %d degC\n", temp_data); // Write Temperature data to console
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
            case CALIB:
                handle_CALI_ctrl();
                break;
        }

        ThisThread::sleep_for(10ms);
    }
}