#include "ecu.h"
#include "can.h"
#include "helpers.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

ecu_type_t ecu_number;
ecu_data_t ecu_data, ecu_data_old;

pthread_mutex_t gateway_lock;

size_t ticks_till_next_simulation = 0;

// Global flag to send the entire ecu_data if there were no updates for a certain time
bool send_all_ecu_data_to_gui = false;

unsigned long last_serial_msg = 0; // PT: CH: BO: Used to check last msg time
void timer_handler(int sig, siginfo_t *si, void *uc) {
    timer_t *tidp;

    tidp = si->si_value.sival_ptr;

    if (*tidp == timer_100_hz) {
        b_100_hz = true;
    } else if (*tidp == timer_20_hz) {
        b_20_hz = true;
    } else if (*tidp == timer_10_hz) {
        b_10_hz = true;
    } else if (*tidp == timer_2_hz) {
        b_2_hz = true;
    }
}

void ecu_setup() {
    // Here we define the cyclic CAN messages that the ECU sends
    switch (ecu_number) {
        case POWERTRAIN:
            // 100 Hz
            define_rep_msg(BRAKE_OUTPUT_IND_MSG, 64, 1, 10);
            define_rep_msg(ENGINE_RPM_MSG, 64, 1, 10);
            define_rep_msg(POWER_STEERING_OUT_IND_MSG, 64, 1, 10);
            define_rep_msg(SHIFT_POSITION_MSG, 64, 1, 10);
            // 20 Hz
            define_rep_msg(ENGINE_STATUS_MSG, 8, 1, 50);
            define_rep_msg(PARKING_BRAKE_STATUS_MSG, 8, 1, 50);
            break;
        case CHASSIS:
            // 100 Hz
            define_rep_msg(BRAKE_OPERATION_MSG, 8, 1, 10);
            define_rep_msg(ACCELERATION_OPERATION_MSG, 8, 1, 10);
            define_rep_msg(STEERING_WHEEL_POS_MSG, 8, 1, 10);
            define_rep_msg(SHIFT_POSITION_SWITCH_MSG, 8, 1, 10);
            define_rep_msg(ENGINE_START_MSG, 8, 1, 10);
            define_rep_msg(TURN_SWITCH_MSG, 8, 1, 10);
            define_rep_msg(HORN_SWITCH_MSG, 8, 1, 10);
            // 20 Hz
            define_rep_msg(LIGHT_SWITCH_MSG, 8, 1, 50);
            define_rep_msg(PARKING_BRAKE_MSG, 8, 1, 50);
            // 10 Hz
            define_rep_msg(WIPER_SWITCH_FRONT_MSG, 8, 1, 100);
            define_rep_msg(WIPER_SWITCH_REAR_MSG, 8, 1, 100);
            define_rep_msg(DOOR_LOCK_UNLOCK_MSG, 8, 1, 100);
            define_rep_msg(L_WINDOW_SWITCH_MSG, 8, 1, 100);
            define_rep_msg(R_WINDOW_SWITCH_MSG, 8, 1, 100);
            define_rep_msg(L_DOOR_HANDLE_MSG, 8, 1, 100);
            define_rep_msg(R_DOOR_HANDLE_MSG, 8, 1, 100);
            break;
        case BODY:
            // 100 Hz
            define_rep_msg(TURN_SIGNAL_INDICATOR_MSG, 8, 1, 10);
            // 20 Hz
            // 10 Hz
            define_rep_msg(DOOR_LOCK_STATUS_MSG, 8, 1, 100);
            // 2 Hz
            define_rep_msg(L_DOOR_POSITION_MSG, 8, 1, 500);
            define_rep_msg(R_DOOR_POSITION_MSG, 8, 1, 500);
            break;
        case OBSERVER:
            // The OBSERVER ECU sends no CAN messages but it reads the bus and checks the timings,
            // therefore we save the reference time intervalls for all messages
            setup_observer_reference_timings();
            break;
        case GATEWAY:
            // Here we define which CAN messages the gateway allows the OBD-II port to read and write
            setup_gateway_whitelist();

            break;
    }

    // Setup Timers
    // The gateway and the observer ECU do not send any CAN messages on regular timings
    if (ecu_number != GATEWAY && ecu_number != OBSERVER) {
        make_timer(&timer_100_hz, 10, 10);
        make_timer(&timer_20_hz, 50, 50);
        if (ecu_number != POWERTRAIN)
            make_timer(&timer_10_hz, 100, 100);
        if (ecu_number == BODY)
            make_timer(&timer_2_hz, 500, 500);
    }
}

void check_message_timers() {
    long check_time = millis();
    if (check_time - last_serial_msg > 100) {
        send_all_ecu_data_to_gui = true;
    }
}

void write_powertrain_ecu_data() {
    ecu_data.parking_brake_status = ecu_data.parking_value;

    ecu_data.shift_position = ecu_data.shift_value;

    // Only switch engine on when in Parking position
    if (ecu_data.shift_position == 'P' && ecu_data.engine_value) {
        ecu_data.engine_status = true;
    }
    if (!ecu_data.engine_value) {
        ecu_data.engine_status = false;
    }

    // We use a counter (ticks_till_next_simulation) to simulate the RPM and the automatic shifting only every X cycles
    if (ticks_till_next_simulation == 0) {
        ticks_till_next_simulation = 20;

        // Gas pedal
        if (ecu_data.accelerator_value > 0 && ecu_data.engine_status &&
            (ecu_data.shift_position == 'D' || ecu_data.shift_position == 'N' ||
             ecu_data.shift_position == 'R')) { // Increase RPM
            ecu_data.engine_rpm += ecu_data.accelerator_value;
            if (ecu_data.gear < 6 && ecu_data.engine_rpm > 60000 && ecu_data.shift_position == 'D') { // Shift up
                ecu_data.gear += 1;
                ecu_data.engine_rpm = 5000;
            }
            if (ecu_data.engine_rpm > 65535)
                ecu_data.engine_rpm = 65535;
        } else { // Decrease RPM
            ecu_data.engine_rpm -= 100;
            if (ecu_data.gear > 1 && ecu_data.engine_rpm < 5000 && ecu_data.shift_position == 'D') { // Shift down
                ecu_data.gear -= 1;
                ecu_data.engine_rpm = 50000;
            }
            if (ecu_data.engine_rpm < 0) {
                ecu_data.engine_rpm = 0;
            }
        }
        if (ecu_data.shift_position == 'R') {
            ecu_data.gear = 1;
        }

        // Brake Pedal
        if (ecu_data.brake_value > 0) {
            ecu_data.brake_output += ecu_data.brake_value;
            if (ecu_data.brake_output > 65535)
                ecu_data.brake_output = 65535;
        } else {
            ecu_data.brake_output = 0;
        }

        // We scale the rpm and gear to a speed value between 0 and 255
        int shall_speed = (ecu_data.engine_rpm + (ecu_data.gear - 1) * 65535) / 1542;

        // Add the influence of the brake to the speed
        shall_speed -= ecu_data.brake_output * 100 / 65535;
        shall_speed -= ecu_data.parking_brake_status * 40;

        // This is neccessary to prevent the speed from jumping up after braking, if the gas pedal is not pressed, the speed can not be increased
        if (ecu_data.accelerator_value == 0 && shall_speed > ecu_data.speed_kph) {
            shall_speed = ecu_data.speed_kph;
        }

        ecu_data.speed_kph += ceil((shall_speed - ecu_data.speed_kph) / 2.0f);

        if (ecu_data.speed_kph < 0) {
            ecu_data.speed_kph = 0;
        }
        if (ecu_data.speed_kph > 255) {
            ecu_data.speed_kph = 255;
        }
    }
    ticks_till_next_simulation -= 1;

    // Transform the steering wheel angle (from 0° to 720°, 360° is center) to the actual tire angle (-30° to +30° with an offset of 360°)
    ecu_data.power_steering = (ecu_data.steering_value - 360) * 30 / 360 + 360;

    write_ecu_data_to_serial();
}

void write_chassis_ecu_data() {
    ecu_data.door_open_indicator = ecu_data.l_door_position | ecu_data.r_door_position;
    ecu_data.door_lock_indicator = ecu_data.door_lock_status;
    write_ecu_data_to_serial();
}

void write_body_ecu_data() {
    if (ecu_data.hazard_value)
        ecu_data.turn_signal_indicator = 3;
    else
        ecu_data.turn_signal_indicator = ecu_data.turn_switch_value;

    ecu_data.front_wiper_status = ecu_data.wiper_f_sw_value;
    ecu_data.rear_wiper_status = ecu_data.wiper_r_sw_value;
    if (ecu_data.light_flash_value)
        ecu_data.light_status = 2;
    else
        ecu_data.light_status = ecu_data.light_switch_value;
    ecu_data.horn_operation = ecu_data.horn_value;
    ecu_data.light_status = ecu_data.light_switch_value;

    if (ecu_data.l_door_handle_value == 2)                                        // Door close button
        ecu_data.l_door_position = 0;                                               // 0 => Door is closed
    else if (ecu_data.l_door_handle_value == 1 && ecu_data.door_lock_status == 0) // Door open button
        ecu_data.l_door_position = 1;                                               // 1 => Door is open
    if (ecu_data.r_door_handle_value == 2)                                        // Door close button
        ecu_data.r_door_position = 0;                                               // 0 => Door is closed
    else if (ecu_data.r_door_handle_value == 1 && ecu_data.door_lock_status == 0) // Door open button
        ecu_data.r_door_position = 1;                                               // 1 => Door is open

    if (ecu_data.l_window_switch_value == 1)      // Window Up button
        ecu_data.l_window_position = 0;             // 0 => Window is up/closed
    else if (ecu_data.l_window_switch_value == 2) // Window Down button
        ecu_data.l_window_position = 1;             // 1 => Window is down/open
    if (ecu_data.r_window_switch_value == 1)      // Window Up button
        ecu_data.r_window_position = 0;             // 0 => Window is up/closed
    else if (ecu_data.r_window_switch_value == 2) // Window Down button
        ecu_data.r_window_position = 1;             // 1 => Window is down/open

    // Only lock the doors if it is unlocked, the lock button is pressed and all doors are closed
    if (ecu_data.door_lock_value == 1 && ecu_data.door_lock_status != 1 && ecu_data.l_door_position == 0 &&
        ecu_data.r_door_position == 0) {
        ecu_data.door_lock_status = 1;
    }
    // Unlock the doors whenever the unlock button is pressed
    if (ecu_data.door_lock_value == 2) {
        ecu_data.door_lock_status = 0;
    }

    write_ecu_data_to_serial();
}

void write_observer_ecu_data() {

    // Look for not allowed values in ecu_data
    if (ecu_data.accelerator_value < 0 || ecu_data.accelerator_value > 100) {
        ecu_data.observer_id = ACCELERATOR_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.brake_output < 0 || ecu_data.brake_output > 65535) {
        ecu_data.observer_id = BRAKE_OUTPUT;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.brake_value < 0 || ecu_data.brake_value > 100) {
        ecu_data.observer_id = BRAKE_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.door_lock_indicator < 0 || ecu_data.door_lock_indicator > 1) {
        ecu_data.observer_id = DOOR_LOCK_INDICATOR;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.door_lock_status < 0 || ecu_data.door_lock_status > 1) {
        ecu_data.observer_id = DOOR_LOCK_STATUS;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.door_open_indicator < 0 || ecu_data.door_open_indicator > 1) {
        ecu_data.observer_id = DOOR_OPEN_INDICATOR;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.engine_rpm < 0 || ecu_data.engine_rpm > 65535) {
        ecu_data.observer_id = ENGINE_RPM;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.engine_status < 0 || ecu_data.engine_status > 1) {
        ecu_data.observer_id = ENGINE_STATUS;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.engine_value < 0 || ecu_data.engine_value > 1) {
        ecu_data.observer_id = ENGINE_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.front_wiper_status < 0 || ecu_data.front_wiper_status > 1) {
        ecu_data.observer_id = FRONT_WIPER_STATUS;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.gear < 0 || ecu_data.gear > 6) {
        ecu_data.observer_id = GEAR;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.hazard_value < 0 || ecu_data.hazard_value > 1) {
        ecu_data.observer_id = HAZARD_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.horn_operation < 0 || ecu_data.horn_operation > 1) {
        ecu_data.observer_id = HORN_OPERATION;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.horn_value < 0 || ecu_data.horn_value > 1) {
        ecu_data.observer_id = HORN_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.l_door_handle_value < 0 || ecu_data.l_door_handle_value > 2) {
        ecu_data.observer_id = L_DOOR_HANDLE_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.l_door_position < 0 || ecu_data.l_door_position > 1) {
        ecu_data.observer_id = L_DOOR_POSITION;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.l_window_position < 0 || ecu_data.l_window_position > 1) {
        ecu_data.observer_id = L_WINDOW_POSITION;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.l_window_switch_value < 0 || ecu_data.l_window_switch_value > 2) {
        ecu_data.observer_id = L_WINDOW_SWITCH_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.light_flash_value < 0 || ecu_data.light_flash_value > 1) {
        ecu_data.observer_id = LIGHT_FLASH_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.light_status < 0 || ecu_data.light_status > 2) {
        ecu_data.observer_id = LIGHT_STATUS;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.light_switch_value < 0 || ecu_data.light_switch_value > 2) {
        ecu_data.observer_id = LIGHT_SWITCH_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.parking_brake_status < 0 || ecu_data.parking_brake_status > 1) {
        ecu_data.observer_id = PARKING_BRAKE_STATUS;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.parking_value < 0 || ecu_data.parking_value > 1) {
        ecu_data.observer_id = PARKING_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.power_steering < 330 || ecu_data.power_steering > 390) {
        ecu_data.observer_id = POWER_STEERING;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.r_door_handle_value < 0 || ecu_data.r_door_handle_value > 1) {
        ecu_data.observer_id = R_DOOR_HANDLE_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.r_door_position < 0 || ecu_data.r_door_position > 2) {
        ecu_data.observer_id = R_DOOR_POSITION;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.r_window_position < 0 || ecu_data.r_window_position > 1) {
        ecu_data.observer_id = R_WINDOW_POSITION;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.r_window_switch_value < 0 || ecu_data.r_window_switch_value > 2) {
        ecu_data.observer_id = R_WINDOW_SWITCH_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.rear_wiper_status < 0 || ecu_data.rear_wiper_status > 1) {
        ecu_data.observer_id = REAR_WIPER_STATUS;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.shift_position != 'P' && ecu_data.shift_position != 'N' && ecu_data.shift_position != 'R' &&
               ecu_data.shift_position != 'D') {
        ecu_data.observer_id = ecu_data.shift_position;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.shift_value != 'P' && ecu_data.shift_value != 'N' && ecu_data.shift_value != 'R' &&
               ecu_data.shift_value != 'D') {
        ecu_data.observer_id = SHIFT_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.speed_kph < 0 || ecu_data.speed_kph > 255) {
        ecu_data.observer_id = SPEED_KPH;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.steering_value < 0 || ecu_data.steering_value > 720) {
        ecu_data.observer_id = STEERING_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.turn_signal_indicator < 0 || ecu_data.turn_signal_indicator > 3) {
        ecu_data.observer_id = TURN_SIGNAL_INDICATOR;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.turn_switch_value < 0 || ecu_data.turn_switch_value > 3) {
        ecu_data.observer_id = TURN_SWITCH_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.wiper_f_sw_value < 0 || ecu_data.wiper_f_sw_value > 1) {
        ecu_data.observer_id = WIPER_F_SW_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    } else if (ecu_data.wiper_r_sw_value < 0 || ecu_data.wiper_r_sw_value > 1) {
        ecu_data.observer_id = WIPER_R_SW_VALUE;
        ecu_data.observer_code = BAD_VALUE;
    }
    write_ecu_data_to_serial();
}

void write_gateway_ecu_data() { write_ecu_data_to_serial(); }

void write_ecu_data_to_serial() {
    char msg[256] = {0};
    char cat[256] = {0};
    strcpy(msg, "EXU");
    switch (ecu_number) {
        case POWERTRAIN:
            if (ecu_data_old.shift_position != ecu_data.shift_position || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 00%x", ecu_data.shift_position);
                strcat(msg, cat);
            }
            if (ecu_data_old.engine_status != ecu_data.engine_status || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 01%x", ecu_data.engine_status);
                strcat(msg, cat);
            }
            if (ecu_data_old.brake_output != ecu_data.brake_output || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 02%x", ecu_data.brake_output);
                strcat(msg, cat);
            }
            if (ecu_data_old.parking_brake_status != ecu_data.parking_brake_status ||
                send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 03%x", ecu_data.parking_brake_status);
                strcat(msg, cat);
            }
            if (ecu_data_old.gear != ecu_data.gear || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 04%x", ecu_data.gear);
                strcat(msg, cat);
            }
            if (ecu_data_old.power_steering != ecu_data.power_steering || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 05%x", ecu_data.power_steering);
                strcat(msg, cat);
            }
            break;

        case CHASSIS:
            if (ecu_data_old.engine_rpm != ecu_data.engine_rpm || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 00%x", ecu_data.engine_rpm);
                strcat(msg, cat);
            }
            if (ecu_data_old.shift_position != ecu_data.shift_position || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 01%x", ecu_data.shift_position);
                strcat(msg, cat);
            }
            if (ecu_data_old.engine_status != ecu_data.engine_status || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 02%x", ecu_data.engine_status);
                strcat(msg, cat);
            }
            if (ecu_data_old.parking_brake_status != ecu_data.parking_brake_status ||
                send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 03%x", ecu_data.parking_brake_status);
                strcat(msg, cat);
            }
            if (ecu_data_old.turn_signal_indicator != ecu_data.turn_signal_indicator ||
                send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 04%x", ecu_data.turn_signal_indicator);
                strcat(msg, cat);
            }
            if (ecu_data_old.door_open_indicator != ecu_data.door_open_indicator || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 05%x", ecu_data.door_open_indicator);
                strcat(msg, cat);
            }
            if (ecu_data_old.door_lock_indicator != ecu_data.door_lock_indicator || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 06%x", ecu_data.door_lock_indicator);
                strcat(msg, cat);
            }
            if (ecu_data_old.speed_kph != ecu_data.speed_kph || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 07%x", ecu_data.speed_kph);
                strcat(msg, cat);
            }
            break;
        case BODY:
            if (ecu_data_old.horn_operation != ecu_data.horn_operation || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 00%x", ecu_data.horn_operation);
                strcat(msg, cat);
            }
            if (ecu_data_old.light_status != ecu_data.light_status || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 01%x", ecu_data.light_status);
                strcat(msg, cat);
            }
            if (ecu_data_old.turn_signal_indicator != ecu_data.turn_signal_indicator ||
                send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 02%x", ecu_data.turn_signal_indicator);
                strcat(msg, cat);
            }
            if (ecu_data_old.wiper_f_sw_value != ecu_data.wiper_f_sw_value || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 03%x", ecu_data.front_wiper_status);
                strcat(msg, cat);
            }
            if (ecu_data_old.wiper_r_sw_value != ecu_data.wiper_r_sw_value || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 04%x", ecu_data.rear_wiper_status);
                strcat(msg, cat);
            }
            if (ecu_data_old.door_lock_value != ecu_data.door_lock_value || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 05%x", ecu_data.door_lock_status);
                strcat(msg, cat);
            }
            if (ecu_data_old.l_door_position != ecu_data.l_door_position || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 06%x", ecu_data.l_door_position);
                strcat(msg, cat);
            }
            if (ecu_data_old.r_door_position != ecu_data.r_door_position || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 07%x", ecu_data.r_door_position);
                strcat(msg, cat);
            }
            if (ecu_data_old.l_window_position != ecu_data.l_window_position || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 08%x", ecu_data.l_window_position);
                strcat(msg, cat);
            }
            if (ecu_data_old.r_window_position != ecu_data.r_window_position || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 09%x", ecu_data.r_window_position);
                strcat(msg, cat);
            }
            break;
        case GATEWAY:
            if (ecu_data_old.gateway_id != ecu_data.gateway_id || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 00%x", ecu_data.gateway_id);
                strcat(msg, cat);
            }
            if (ecu_data_old.gateway_code != ecu_data.gateway_code || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 01%x", ecu_data.gateway_code);
                strcat(msg, cat);
            }
            break;
        case OBSERVER:
            if (ecu_data_old.observer_id != ecu_data.observer_id || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 00%x", ecu_data.observer_id);
                strcat(msg, cat);
            }
            if (ecu_data_old.observer_code != ecu_data.observer_code || send_all_ecu_data_to_gui == true) {
                sprintf(cat, " 01%x", ecu_data.observer_code);
                strcat(msg, cat);
            }
            break;
        default:
            break;
    }
    if (strlen(msg) > 3) {
        strcat(msg, "\n");
        if (write(serial_port, msg, strlen(msg)) <= 0) {
            perror("Failed to write to serial");
        }
        last_serial_msg = millis();
        send_all_ecu_data_to_gui = false;
    }
}

void set_ecu_id_to_value(int id, int value) {

    if (ecu_number == CHASSIS) {
        switch (id) {
            case 0x00:
                ecu_data.brake_value = value;
                break; // C Brake operation amount
            case 0x01:
                ecu_data.accelerator_value = value;
                break; // C Accelerator operation amount
            case 0x02:
                ecu_data.steering_value = value;
                break; // C Handle operation position
            case 0x03:
                ecu_data.shift_value = value;
                break; // C Shift position switch
            case 0x04:
                ecu_data.turn_switch_value = value;
                break; // C Blinker left / right
            case 0x05:
                ecu_data.horn_value = value;
                break; // C Horn switch
            case 0x06:
                ecu_data.light_switch_value = value;
                break; // C Position headlight high beam switch
            case 0x07:
                ecu_data.light_flash_value = value;
                break; // C Passing switch
            case 0x08:
                ecu_data.parking_value = value;
                break;
            case 0x09:
                ecu_data.wiper_f_sw_value = value;
                break;
            case 0x0A:
                ecu_data.wiper_r_sw_value = value;
                break;
            case 0x0B:
                ecu_data.door_lock_value = value;
                break;
            case 0x0C:
                ecu_data.l_door_handle_value = value;
                break;
            case 0x0D:
                ecu_data.r_door_handle_value = value;
                break;
            case 0x0E:
                ecu_data.l_window_switch_value = value;
                break;
            case 0x0F:
                ecu_data.r_window_switch_value = value;
                break;
            case 0x10:
                ecu_data.hazard_value = value;
                break;
            case 0x11:
                ecu_data.engine_value = value;
                break;
            default:
                printf("Error: Bad ID: 0x%x\n", id);
        }
    }
}

void update_ecu_data_serial() {
    switch (ecu_number) {
        case POWERTRAIN:
            write_powertrain_ecu_data();
            break;
        case CHASSIS:
            // The Chassis ECU is the only ECU that reads updates from the GUI, all the buttons, steeringwheel, pedals etc. belong to the chassis
            read_chassis_ecu_data();
            write_chassis_ecu_data();
            break;
        case BODY:
            write_body_ecu_data();
            break;
        case OBSERVER:
            write_observer_ecu_data();
            break;
        case GATEWAY:
            write_gateway_ecu_data();
            break;
        default:
            break;
    }
}


int loop() {
    check_message_timers();
    update_ecu_data_serial();
    read_can_bus_and_handle_input();
    ecu_data_old = ecu_data;

    // Optimization to reduce delays of the CAN messages, as the timer based solution produced some delays
    send_pending_can_messages();
    /*
    if (b_100_hz) {
        can_write_100_hz_msgs();
    }
    if (b_20_hz) {
        can_write_20_hz_msgs();
    }
    if (b_10_hz) {
        can_write_10_hz_msgs();
    }
    if (b_2_hz) {
        can_write_2_hz_msgs();
    }
     */
    usleep(10);
    return 0;
}
