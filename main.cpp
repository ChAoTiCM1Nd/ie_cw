#include "mbed.h"
#include <stdio.h>
#include <cstdint>

DigitalOut led(LED1);
PwmOut fan(PB_0);           // PWM control for the fan
InterruptIn fan_tacho(PA_0); // Tachometer input to count pulses
BufferedSerial mypc(USBTX, USBRX);


volatile int pulse_count = 0; // Counts tachometer pulses

// Interrupt service routine to count tachometer pulses
void count_pulse() {
    pulse_count++;
    led = !led;
}

int main() {
    FILE* mypcFile = fdopen(&mypc, "r+");

    // Initialize PWM for the fan
    fan.period(0.00002f);   // Set period for 25 kHz PWM (adjust for your fan specs)
    fan.write(0.5f);        // 50% duty cycle to start (adjustable)

    // Attach the interrupt for the tachometer signal
    fan_tacho.rise(&count_pulse); // Count rising edges

    while (true) {
        // Capture pulse count over 1 second


        // Clear pulse count at the start of the measurement period
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

        // Output RPM to serial
        fprintf(mypcFile, "Fan RPM: %d\n", rpm);
    }
}