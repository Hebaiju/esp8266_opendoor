#include "drv_servo.h"
#include <Servo.h>

static Servo s_servo;
static uint8_t s_current_angle = 0;

err_code_t drv_servo_init(void)
{
    s_servo.attach(SERVO_PIN);
    s_current_angle = SERVO_MIN_ANGLE;
    s_servo.write(s_current_angle);
    return APP_OK;
}

void drv_servo_set_angle(uint8_t angle)
{
    if (angle > 180) {
        angle = 180;
    }
    s_servo.write(angle);
    s_current_angle = angle;
}

uint8_t drv_servo_get_angle(void)
{
    return s_current_angle;
}
