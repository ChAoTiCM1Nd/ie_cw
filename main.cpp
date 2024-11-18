//https://chatgpt.com/share/6734cb16-d66c-800f-850d-4885cfd94bdc


#include "mbed.h"
#include <iterator>
#include <stdio.h>

#include <cstdint>
#include "LCD_ST7066U.h"

LCD lcd(PB_15, PB_14, PB_10, PA_8, PB_2, PB_1); // Adjust pin names as per your hardware



DigitalOut led(LED1);
DigitalOut led_ext(PC_0);           // External LED for counterclockwise indication
PwmOut fan(PB_0);                   // PWM control for the fan
InterruptIn fan_tacho(PA_0);        // Tachometer input to count pulses
DigitalIn inc1(PA_1);               // Rotary encoder channel A
DigitalIn inc2(PA_4);               // Rotary encoder channel B
BufferedSerial mypc(USBTX, USBRX, 115200);

const int max_rpm = 3600;           // Maximum fan RPM for 100% duty cycle (adjusted)
volatile int pulse_count = 0;       // Counts tachometer pulses
volatile int target_rpm = 1740;     // Initial target RPM (50% of max RPM)
int inc1_prev = 0;                  // Previous state of inc1 for edge detection

// Define limits for the integral term
const float integral_max = 500.0; // Adjust this limit based on tuning
const float integral_min = -500.0;

float Kp = 0.00042;
float Ki = 0.00003;
float Kd = 0.0000;

float current_duty_cycle = 0.0;     // Initial duty cycle set to 50%
float prev_error = 0;
float integral = 0;

// Function to update fan speed based on target RPM
void update_fan_speed(float duty_cycle) {

    /* Working code for open loop control!
    // Constrain the target RPM to a safe range (0 to 3600 RPM)
    //if (target_rpm < 0) target_rpm = 0;
    //if (target_rpm > max_rpm) target_rpm = max_rpm;
    // Calculate and set PWM duty cycle based on target RPM
    // Map target RPM from 0 to 3600 RPM to a duty cycle from 0% to 100%
    //duty_cycle = static_cast<float>(target_rpm) / max_rpm;
    */

    //if (duty_cycle < 0.0) duty_cycle = 0.0;
    //if (duty_cycle > 1.0) duty_cycle = 1.0;
    
    fan.write(duty_cycle);

    printf("Current duty cycle: %.2f (Target RPM: %d)\n", duty_cycle, target_rpm);
}

// Interrupt service routine to count tachometer pulses
void count_pulse() {
    pulse_count++;
    led = !led;
}

// Main program
int main() {
    mypc.set_format(8, BufferedSerial::None, 1); // Set serial format
    printf("Starting fan control with encoder\n");

    // Initialize PWM for the fan
    fan.period(0.02f);   

    // Attach interrupt for the tachometer
    fan_tacho.rise(&count_pulse); // Count rising edges

    Timer rpm_timer;
    rpm_timer.start();

    while (true) {
        // Rotary encoder logic
        if (inc1.read() != inc1_prev) {
            if (inc1_prev == 0 && inc1.read() == 1) {    // Rising edge of inc1
                if (inc2.read() == 0) {
                    // Clockwise rotation: increase target RPM
                    led = 1;
                    target_rpm += 100;  // Increase target RPM by 100
                    led = 0;
                } else {
                    // Counterclockwise rotation: decrease target RPM
                    led_ext = 1;
                    target_rpm -= 100;  // Decrease target RPM by 100
                    led_ext = 0;
                }

                if (target_rpm > max_rpm) target_rpm = max_rpm;
                if (target_rpm < 0) target_rpm = 0;


                // Update fan speed based on the new target RPM
                //update_fan_speed();
                // Output the encoder count and target RPM to serial
                //printf("The encoder count is %d. Target RPM: %d\n", target_rpm / 100, target_rpm);
              
              lcd.writeLine("Initializing...", 1); // Write text to the second line
              wait_us(500000);
              lcd.clear();


            }
        }

        inc1_prev = inc1.read(); // Update previous state

        // Debounce
        wait_us(500);

        float delta_t = 0.1f;
        // Calculate RPM once per second
        if (rpm_timer.elapsed_time().count() >= 1000000) { // 1 second elapsed
            rpm_timer.reset();

            // Calculate RPM: (pulse_count / 2) * 60 for a 2-pulse per revolution fan
            int rpm;
            {
                CriticalSectionLock lock; // Ensure atomic access
                rpm = (pulse_count / 2) * 60;
                pulse_count = 0; // Reset pulse count after reading
            }

            // Output current RPM and target RPM to serial
            

            /* Below code was working for open loop! 
            // Adjust duty cycle if the RPM is not equal to the target RPM
            if (rpm < target_rpm) {
                // If the actual RPM is less than the target, increase the duty cycle
                current_duty_cycle += 0.0025; // Increase duty cycle by 5%
                if (current_duty_cycle > 1.0) current_duty_cycle = 1.0; // Cap the duty cycle to 100%
            } else if (rpm > target_rpm) {
                // If the actual RPM is greater than the target, decrease the duty cycle
                current_duty_cycle -= 0.0025; // Decrease duty cycle by 5%
                printf("RPM is greater than target rpm. Must reduce speed.\n");
                if (current_duty_cycle < 0.0) current_duty_cycle = 0.0; // Cap the duty cycle to 0%
            }

            // Set the adjusted duty cycle to the fan
            fan.write(current_duty_cycle);

            // Output the adjusted duty cycle
            printf("Adjusted Duty Cycle: %.2f\n", current_duty_cycle);
            */


            int error = target_rpm - rpm;
            integral += error * delta_t;
            int derivative = error - prev_error;

            // Apply clamping to prevent windup
            if (integral > integral_max) integral = integral_max;
            if (integral < integral_min) integral = integral_min;

            float pid_output = (Kp * error) + (Ki * integral) + (Kd * 0.00f);

            current_duty_cycle += pid_output;
            update_fan_speed(current_duty_cycle);

            //printf("Fan RPM: %d, Target RPM: %d, calculated PID Output is %.2f \nIntegral: %.2f, Derivative: %d, Error: %d\n", rpm, target_rpm, pid_output, integral, derivative, error);
            printf("Calculated duty cycle: %.2f, Fan RPM: %d, Target RPM: %d\n", current_duty_cycle, rpm, target_rpm);

            prev_error = error;
        }
    }

}

