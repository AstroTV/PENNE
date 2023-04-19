#ifndef PENNE_ECU_H
#define PENNE_ECU_H
#include "can.h"
#include <signal.h>

/**
 * The 4 different ECU types that are supported by this simulator
 */
typedef enum { POWERTRAIN, CHASSIS, BODY, GATEWAY, OBSERVER } ecu_type_t;

typedef enum { OK, BAD_VALUE, BAD_TIMING, BAD_CAN_ID } observer_code_t;

typedef enum {
  NONE,
  ENGINE_RPM,
  SPEED_KPH,
  BRAKE_VALUE,
  ACCELERATOR_VALUE,
  STEERING_VALUE,
  SHIFT_VALUE,
  ENGINE_VALUE,
  TURN_SWITCH_VALUE,
  HAZARD_VALUE,
  HORN_VALUE,
  LIGHT_SWITCH_VALUE,
  LIGHT_FLASH_VALUE,
  PARKING_VALUE,
  WIPER_F_SW_VALUE,
  WIPER_R_SW_VALUE,
  DOOR_LOCK_VALUE,
  L_WINDOW_SWITCH_VALUE,
  R_WINDOW_SWITCH_VALUE,
  BRAKE_OUTPUT,
  POWER_STEERING,
  GEAR,
  SHIFT_POSITION,
  TURN_SIGNAL_INDICATOR,
  DOOR_OPEN_INDICATOR,
  DOOR_LOCK_INDICATOR,
  HORN_OPERATION,
  ENGINE_STATUS,
  PARKING_BRAKE_STATUS,
  LIGHT_STATUS,
  FRONT_WIPER_STATUS,
  REAR_WIPER_STATUS,
  DOOR_LOCK_STATUS,
  L_DOOR_HANDLE_VALUE,
  R_DOOR_HANDLE_VALUE,
  L_DOOR_POSITION,
  R_DOOR_POSITION,
  L_WINDOW_POSITION,
  R_WINDOW_POSITION,
} observer_id_t;

/**
 * Struct that holds the entire ECU state which is updated by incoming CAN messages from the other ECUs and Serial messages from the GUI
 */
typedef struct ecu_data_t {
  int engine_rpm;
  int speed_kph;
  int brake_value;
  int accelerator_value;
  int steering_value;
  unsigned char shift_value;
  bool engine_value;
  unsigned char turn_switch_value;
  bool hazard_value;
  bool horn_value;
  unsigned char light_switch_value;
  unsigned char light_flash_value;
  bool parking_value;
  unsigned char wiper_f_sw_value;
  unsigned char wiper_r_sw_value;
  unsigned char door_lock_value;
  unsigned char l_window_switch_value;
  unsigned char r_window_switch_value;
  int brake_output;
  int power_steering;
  char gear;
  unsigned char shift_position;
  unsigned char turn_signal_indicator;
  unsigned char door_open_indicator;
  unsigned char door_lock_indicator;
  unsigned char horn_operation;
  unsigned char engine_status;
  unsigned char parking_brake_status;
  unsigned char light_status;
  unsigned char front_wiper_status;
  unsigned char rear_wiper_status;
  unsigned char door_lock_status;
  unsigned char l_door_handle_value;
  unsigned char r_door_handle_value;
  unsigned char l_door_position;
  unsigned char r_door_position;
  unsigned char l_window_position;
  unsigned char r_window_position;
  observer_id_t observer_id;     // Which value is problematic
  observer_code_t observer_code; // Problem Code of the problematic value
  int gateway_id;                // Which can id was handled by the gateway
  unsigned char gateway_code;    // Code that the gateway assigns after handling a CAN message, 0 => OK, 1 => READ_BLOCKED, 2 => WRITE_BLOCKED

} ecu_data_t;

extern ecu_type_t ecu_number;
extern ecu_data_t ecu_data, ecu_data_old;

extern pthread_mutex_t gateway_lock;

/**
 * Callback function for the timer, which calls the correct function depending on the elapsed interval
 * @param sig unused
 * @param si siginfo which stores the si_value.sival_ptr that can be used to determine which timer elapsed (100Hz, 20Hz, 10Hz, 2Hz)
 * @param uc unused
 */
void timer_handler(int sig, siginfo_t *si, void *uc);

/**
 * Defines the repeated CAN messages that are sent from the ECU and calls the timer setup routine
 */
void ecu_setup();

/**
 * Checks when the last message has been sent to the GUI.
 * If the last message is past for a certain threshold a global flag is set to send the entire ecu_data with the next message
 */
void check_message_timers();

/**
 * Sends updates to the GUI for changed values of the ecu_data
 */
void write_powertrain_ecu_data();

/**
 * Sends updates to the GUI for changed values of the ecu_data
 */
void write_chassis_ecu_data();

/**
 * Sends updates to the GUI for changed values of the ecu_data
 */
void write_body_ecu_data();

/**
 * @brief Sends updates to the GUI for changed values of the ecu_data
 *
 */
void write_observer_ecu_data();

/**
 * @brief Sends updates to the GUI for changed values of the ecu_data
 *
 */
void write_gateway_ecu_data();

/**
 * Sends changed ecu_data values to the GUI.
 * When a the global flag send_all_ecu_data_to_gui is set, the entire ecu_data struct is sent
 */
void write_ecu_data_to_serial();

/**
 * Sets the ecu_data with the specified id to a specific value
 * @param id ID of the ecu_data field that is sent from the GUI
 * @param value new value for the field
 */
void set_ecu_id_to_value(int id, int value);

/**
 * High level function that calls write_<ECU_TYPE>_ecu_data according to the global set ecu_type
 */
void update_ecu_data_serial();

/**
 * Main loop of the ECU
 * @return 0 on successful execution
 * @return nonzero on error
 */
int loop();

#endif // PENNE_ECU_H
