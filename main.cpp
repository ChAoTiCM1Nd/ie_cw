#include "mbed.h"
#include <stdio.h>
#include <cstdint>

DigitalOut led(LED1);
PwmOut fan(PB_0);               // PWM control for the fan
InterruptIn fan_tacho(PA_0);    // Tachometer input to count pulses
InterruptIn encoderA(PA_1);     // Rotary encoder channel A
InterruptIn encoderB(PA_2);     // Rotary encoder channel B
BufferedSerial mypc(USBTX, USBRX);

const int max_rpm = 3480;       // Maximum fan RPM at 100% duty cycle
volatile int pulse_count = 0;   // Counts tachometer pulses
volatile int fan_speed = 50;    // Initial fan speed (50% duty cycle)

// Debounce variables
Timer debounce_timer;
volatile bool debounce = false;

// Interrupt service routine to count tachometer pulses
void count_pulse() {
    pulse_count++;
    led = !led;
}

// Interrupt service routine for rotary encoder
void encoder_interrupt() {
    // Check debounce status
    if (debounce) return;
    debounce = true;
    debounce_timer.reset();

    // Determine rotation direction
    if (encoderA.read() != encoderB.read()) {
        fan_speed += 5;  // Increase speed
    } else {
        fan_speed -= 5;  // Decrease speed
    }

    // Constrain fan speed between 0 and 100%
    if (fan_speed < 0) fan_speed = 0;
    if (fan_speed > 100) fan_speed = 100;

    // Update PWM duty cycle
    fan.write(fan_speed / 100.0f);
}

// Main program
int main() {
    FILE* mypcFile = fdopen(&mypc, "r+");

    // Initialize PWM for the fan
    fan.period(0.00002f);    // Set period for 25 kHz PWM (adjust for your fan specs)
    fan.write(fan_speed / 100.0f);   // Set initial duty cycle based on fan_speed

    // Attach interrupts for the tachometer and encoder channels
    fan_tacho.rise(&count_pulse);    // Count rising edges
    encoderA.rise(&encoder_interrupt);
    encoderB.rise(&encoder_interrupt);

    // Start debounce timer
    debounce_timer.start();

    while (true) {
        // Capture pulse count over 1 second
        {
            CriticalSectionLock lock; // Ensure atomic access
            pulse_count = 0;
        }
        thread_sleep_for(1000); // Measure for 1 second

        // Calculate RPM: (pulse_count / 2) * 60 for a 2-pulse per revolution fan
        int rpm;
        {
            CriticalSectionLock lock; // Ensure atomic access
            rpm = (pulse_count / 2) * 60;
        }

        // Calculate and output fan RPM as a percentage of max RPM
        int speed_rpm = (fan_speed * max_rpm) / 100;   // Map fan_speed to RPM
        fprintf(mypcFile, "Fan RPM: %d (approx %d%% of max)\n", rpm, fan_speed);

        // Debounce delay check
        if (debounce && debounce_timer.elapsed_time().count() > 50) { // 50 ms debounce
            debounce = false;
        }
    }
}
