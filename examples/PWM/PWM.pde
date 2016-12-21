#include <TimerFive.h>

const int PWM_Pin = 44;

void setup(void){
  Timer5.initialize(40);  // 40 us = 25 kHz
  Serial.begin(9600);
}

void loop(void){
  // slowly increase the PWM speed
  for (float Duty_Cycle = 0.0; Duty_Cycle < 100.0; Duty_Cycle++) {
    Serial.print("PWM Duty Cycle = "); //Comment this line to view duty cycle in plotter
    Serial.println(Duty_Cycle);
    Timer5.pwm(PWM_Pin, (Duty_Cycle / 100) * 1023);
    delay(250);
  }
}
