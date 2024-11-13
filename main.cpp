#include "mbed.h"
#include <stdio.h>
#include <cstdint>

// PWM pins and interrupt input
PwmOut led(LED1);           // Control for the onboard LED
PwmOut fan(PB_0);           // PWM control for the fan
InterruptIn fan_taco(PA_0); // Tachometer signal from the fan

// Serial communication for debugging
BufferedSerial mypc(USBTX, USBRX);

// Global variables
volatile int pulse_count = 0;
int rpm = 0;  // Fan speed in RPM

// Calculate RPM based on pulse count (called periodically)
void count_pulse() {
    pulse_count++;
}

void calculate_rpm() {
    // Assuming fan_taco gives 2 pulses per revolution (common for fans)
    // You may adjust this calculation based on actual fan specs
    rpm = (pulse_count * 60) / 2;  // Convert pulse count to RPM
    pulse_count = 0;  // Reset pulse count for the next measurement cycle
}

int main() {
    // Setup serial output for debugging
    FILE* mypcFile = fdopen(&mypc, "r+");
    fprintf(mypcFile, "Starting...\n\n");

    // Configure LED PWM (for indication, e.g., heartbeat LED)
    led.period(2.0f);  // 0.5Hz blink rate
    led.write(0.25f);  // 25% brightness

    // Configure fan PWM
    fan.period(0.02f); // Set PWM period to 50Hz for fan control (adjust if needed)
    fan.write(0.0f);   // Initially turn off the fan

    // Attach interrupt function to count pulses from the fan taco signal
    fan_taco.rise(&count_pulse);  // Rising edge interrupt to count pulses

    while (true) {
        // Calculate RPM every second
        ThisThread::sleep_for(1000ms);
        calculate_rpm();

        // Print the RPM to serial output
        fprintf(mypcFile, "RPM: %d\n", rpm);

        // Fan control logic based on RPM
        if (rpm < 1000) {
            // Low RPM, increase fan speed
            fan.write(0.3f);  // Set PWM duty cycle to 30%
        } else if (rpm < 3000) {
            // Medium RPM, moderate fan speed
            fan.write(0.5f);  // Set PWM duty cycle to 50%
        } else {
            // High RPM, fan running at maximum speed
            fan.write(0.8f);  // Set PWM duty cycle to 80%
        }
    }
}
