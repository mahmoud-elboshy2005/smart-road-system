#ifndef _MOTOR_H_
#define _MOTOR_H_

void motor_task(void *pvParams);

void motor_turn_left();
void motor_turn_right();
void motor_stop();

#endif //_MOTOR_H_