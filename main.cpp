#include "mbed.h"
#include "LCD_ST7066U.h"
#include "mRotaryEncoder.h"

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

Timer rpm_timer;
FanMode current_mode = OFF;

volatile int pulse_count = 0;       // Counts tachometer pulses
volatile int target_rpm = 1000;      // Initial target RPM
int last_encoder_value = 0;         // Last known encoder position
float current_duty_cycle = 0.0f;    // Initial duty cycle

// PID control parameters
float Kp = 0.0001;
float Ki = 0.0001;
float Kd = 0.0001;

float filtered_rpm = 0.0f;

float prev_error = 0.0;
float integral = 0.0;

const float integral_max = 500.0;
const float integral_min = -500.0;
const int MAX_RPM = 1800; // Maximum RPM corresponding to 100% duty cycle

Mutex lcd_mutex;

// Global variables for tracking PWM time
float pwm_period = 1.0f; // PWM period in seconds
float current_time = 0.0f; // Time within the PWM period

// Function to count tachometer pulses during high phase of PWM with debounce and filtering
void count_pulse() {
    static uint32_t last_time = 0;  // Time of the last valid pulse
    uint32_t current_time = osKernelGetTickCount();  // Get current system time in ticks (milliseconds)

    if (pwm_sync.read() == 1) {  // Count only during PWM high phase
        if (current_time - last_time > 5) {  // Filter out pulses that occur in rapid succession (less than 5ms apart)
            pulse_count++;  // Increment pulse count
            led = !led;      // Toggle LED on each valid pulse

            last_time = current_time;  // Update last pulse time
        }
    }
}

// Function to update fan speed based on duty cycle
void update_fan_speed(float duty_cycle) {
    if (duty_cycle < 0.0f) duty_cycle = 0.0f;
    if (duty_cycle > 1.0f) duty_cycle = 1.0f;

    fan.write(duty_cycle);
    
    // Simulate PWM time to adjust pwm_sync based on duty cycle
    current_time += 0.01f; // Update time (assuming 10ms step in loop)

    if (current_time >= pwm_period) {
        current_time -= pwm_period; // Reset time when period ends
    }

    // pwm_sync is true for the portion of time corresponding to the duty cycle
    pwm_sync = (current_time < duty_cycle * pwm_period) ? 1 : 0;
}

void safe_lcd_write(const char* text, int line) {
    static char last_text[2][17] = { "", "" }; // Adjust size for two lines, 16 chars + null terminator

    lcd_mutex.lock();
    if (strncmp(last_text[line], text, 16) != 0) { // Compare with last written text
        strncpy(last_text[line], text, 16);      // Update the stored text
        last_text[line][16] = '\0';              // Ensure null termination

        char padded_text[17] = { ' ' };          // Create a blank-padded string
        strncpy(padded_text, text, 16);          // Copy the text to the padded string
        for (int i = strlen(text); i < 16; i++) {
            padded_text[i] = ' ';                // Fill the rest with spaces
        }
        padded_text[16] = '\0';                  // Ensure null termination

        lcd.writeLine(padded_text, line);        // Write the padded string to the LCD
    }
    lcd_mutex.unlock();
}



int calculate_rpm() {
    float time_seconds = rpm_timer.elapsed_time().count() / 1e6; // Convert to seconds
    rpm_timer.reset();
    int rpm = (pulse_count / 2.5f) * (60.0f / time_seconds); // 6 pulses per revolution
    pulse_count = 0;

    // Simple low-pass filter
    filtered_rpm = 0.8f * filtered_rpm + 0.2f * rpm;

    return (filtered_rpm > 50) ? (int)filtered_rpm : 0;
}

// Function to calculate target RPM based on rotary encoder input
void calc_target_rpm() {
    int encoder_value = encoder.Get();          // Read encoder position
    int encoder_diff = encoder_value - last_encoder_value; // Calculate change

    if (encoder_diff != 0) {
        target_rpm += encoder_diff * 25; // Adjust RPM by 25 per encoder step
        if (target_rpm < 0) target_rpm = 0;
        if (target_rpm > MAX_RPM) target_rpm = MAX_RPM;

        // Update LCD and log
        char buffer[16];
        sprintf(buffer, "Target RPM: %d", target_rpm);
        safe_lcd_write(buffer, 0);
        printf("Target RPM: %d\n", target_rpm);
    }

    last_encoder_value = encoder_value; // Update last position
}

// Closed-loop control logic
void handle_closed_loop_ctrl() {
    calc_target_rpm(); // Update target RPM

    int rpm = calculate_rpm();
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

    char buffer[16];
    sprintf(buffer, "RPM: %d", rpm);
    safe_lcd_write(buffer, 1);

    printf("Closed Loop: Target RPM: %d, Measured RPM: %d, Duty Cycle: %.2f, Error: %d\n",
           target_rpm, rpm, current_duty_cycle, error);

    prev_error = error;
}

// Open-loop control logic
void handle_open_loop_ctrl() {
    calc_target_rpm(); // Update target RPM

    // Set duty cycle directly based on target RPM
    current_duty_cycle = (float)target_rpm / MAX_RPM;

    // Constrain duty cycle
    if (current_duty_cycle > 1.0f) current_duty_cycle = 1.0f;
    if (current_duty_cycle < 0.0f) current_duty_cycle = 0.0f;

    update_fan_speed(current_duty_cycle);

    char buffer[16];
    sprintf(buffer, "Duty: %.2f", current_duty_cycle);
    safe_lcd_write(buffer, 1);

    printf("Open Loop: Target RPM: %d, Duty Cycle: %.2f\n", target_rpm, current_duty_cycle);
}

// OFF mode logic
void handle_off_mode() {
    fan.write(0.0f); // Turn off the fan
    pwm_sync = 0;    // Ensure sync signal is off
}

// AUTO mode logic
void handle_auto_mode() {
    // In AUTO mode, implement adaptive control logic
    // For now, mimic closed-loop behavior
    handle_closed_loop_ctrl();
}

// Function to handle button press for mode selection
void update_mode() {
    static Timer debounce_timer;
    debounce_timer.start();

    static int last_button_state = 1;
    int button_state = button.read();

    if (button_state == 0 && last_button_state == 1 && debounce_timer.elapsed_time().count() > 100000) {
        debounce_timer.reset();

        // Cycle through the modes
        current_mode = static_cast<FanMode>((current_mode + 1) % 4);

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

        printf("Mode changed to: %d\n", current_mode);
    }

    last_button_state = button_state;
}

int main() {
    fan.period(0.001f); // PWM period = 1ms
    fan.write(0.0f);    // Start fan with 0 duty cycle
    pwm_sync = 0;       // Initially turn off PWM sync

    safe_lcd_write("M: OFF", 0); // Display the initial mode
    rpm_timer.start();
    fan_tacho.rise(count_pulse); // Set up interrupt for tachometer pulse detection

    while (true) {
        update_mode();  // Check button and update mode

        // Handle fan control based on current mode
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

        ThisThread::sleep_for(10ms);
    }
}

