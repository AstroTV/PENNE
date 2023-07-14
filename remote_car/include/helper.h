#ifndef REMOTE_CAR_HELPER_H
#define REMOTE_CAR_HELPER_H

#include <glib.h>
#include <gio/gio.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <stdio.h>

#define SUCCESS 0
#define FAILURE -1

#define HORN_OPERATION_MSG          0x98
#define ENGINE_RPM_MSG              0x43
#define POWER_STEERING_OUT_IND_MSG  0x62
#define LIGHT_INDICATOR_MSG         0x1a7
#define SHIFT_POSITION_MSG          0x77
#define DOOR_LOCK_UNLOCK_MSG        0x286
#define BRAKE_OUTPUT_IND_MSG        0x24
#define TURN_SIGNAL_INDICATOR_MSG   0x8d
#define ENGINE_START_MSG            0x1b8

#define NUMBER_CAN_IDS              9

#define GEAR_PARK                   0x50
#define GEAR_REVERSE                0x52
#define GEAR_NEUTRAL                0x4e
#define GEAR_DRIVE                  0x44
#define PARK                        0
#define REVERSE                     1
#define NEUTRAL                     2
#define DRIVE                       3

#define MIDDLE_POS_STEERING         0x68
#define OFFSET_SPEED                85
#define MULTI_STEERING              5
#define MULTI_SPEED                 1

#define NONE    0
#define MOVE    1
#define HORN    2
#define LIGHT   3
#define GEAR    4
#define TURN    5
#define BRAKE   6
#define LOCK    7

#define COMMAND_DRIVE   'A'
#define COMMAND_HORN    'D'
#define COMMAND_LIGHT   'C'
#define COMMAND_ALIVE   'E'
#define COMMAND_LOCK    'L'
#define COMMAND_GEAR    'G'

#define RGB_MODE_OFF        0
#define RGB_MODE_LIGHT      1
#define RGB_MODE_HIGH_BEAM  2
#define RGB_MODE_LEFT_TS    3
#define RGB_MODE_RIGHT_TS   4
#define RGB_MODE_WARNING    5
#define RGB_MODE_BRAKE      6
#define RGB_MODE_TURN_SIGN_OFF  7

#define TURN_LEFT   0x00
#define TURN_OFF    0x01
#define TURN_RIGHT  0x02
#define WARNING     0x03

#define TIME_INTERVAL   1
#define TIMEOUT         1

typedef struct tmp_t {
    int speed;
    int steering;
    int horn;
    int light;
    int gear;
    int turn;
    int brake;
    int lock;
    int engine;
} tmp_t;

typedef struct car_t {
    int speed;
    int steering_angle;
    int changed;
    int gear;
    tmp_t tmp;
} car_t;

/**
 * Connects to CAN bus
 * @param s The socket
 * @return 0 if the initialization succeded
 * @return 1 if the initialization failed
*/
int init_can(int *s);

/**
 * @param frame
 * @param car
*/
void read_can(struct canfd_frame frame, car_t *car);

#endif //REMOTE_CAR_HELPER_H