/*
 * servo.h
 *
 *  Created on: Jan 22, 2026
 *      Author: norbe
 */

#ifndef INC_SERVO_H_
#define INC_SERVO_H_

//	Servo
#define TURNIGY_TG9E

//	Macro
// 	For Turnigy TG9e
#ifdef	TURNIGY_TG9E
#define SERVO_MIN 550
#define SERVO_MAX 2430
#define ANGLE_MIN 0
#define ANGLE_MAX 180
#endif

typedef struct {
    TIM_HandleTypeDef* htim;
    uint32_t channel;
} servo_t;

// 	Servo initialization
void Servo_Init(servo_t *servo, TIM_HandleTypeDef *_htim, uint32_t _channel);

// 	Servo angle setting function
void Servo_SetAngle(servo_t *servo, uint16_t angle);

// 	Servo angle fine adjustment function
void Servo_SetAngleFine(servo_t *servo, float angle);

// 	Function to scale values ​​from one range to another
long map(long x, long in_min, long in_max, long out_min, long out_max);

#endif /* INC_SERVO_H_ */
