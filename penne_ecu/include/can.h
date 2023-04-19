#ifndef PENNE_CAN_H
#define PENNE_CAN_H

#include <stdbool.h>
#include <unistd.h>

#define MAX_MSGS 32
#define CAN_MSG_SPACING 10
#define HIGHEST_POSSIBLE_CAN_ID 0xFFF

// Powertrain
// 100Hz
#define BRAKE_OUTPUT_IND_MSG 0x24
#define ENGINE_RPM_MSG 0x43
#define POWER_STEERING_OUT_IND_MSG 0x62
#define SHIFT_POSITION_MSG 0x77
// 20Hz
#define ENGINE_STATUS_MSG 0x19a
#define PARKING_BRAKE_STATUS_MSG 0x1d3
// 2Hz

// Chassis
// 100Hz
#define BRAKE_OPERATION_MSG 0x1a
#define ACCELERATION_OPERATION_MSG 0x2f
#define STEERING_WHEEL_POS_MSG 0x58
#define SHIFT_POSITION_SWITCH_MSG 0x6d
#define ENGINE_START_MSG 0x1b8
#define TURN_SWITCH_MSG 0x83
#define HORN_SWITCH_MSG 0x98
// 20Hz
#define LIGHT_SWITCH_MSG 0x1a7
#define PARKING_BRAKE_MSG 0x1c9
// 10Hz
#define WIPER_SWITCH_FRONT_MSG 0x25c
#define WIPER_SWITCH_REAR_MSG 0x271
#define DOOR_LOCK_UNLOCK_MSG 0x286
#define L_WINDOW_SWITCH_MSG 0x29c
#define R_WINDOW_SWITCH_MSG 0x2b1
#define L_DOOR_HANDLE_MSG 0x29d
#define R_DOOR_HANDLE_MSG 0x2b2

// Body
// 100Hz
#define TURN_SIGNAL_INDICATOR_MSG 0x8d
// 10Hz
#define DOOR_LOCK_STATUS_MSG 0x290
// 2Hz
#define L_DOOR_POSITION_MSG 0x2bc
#define R_DOOR_POSITION_MSG 0x2a7

/**
 * Struct that holds a CAN message
 */
typedef struct can_message_t {
    size_t id;
    unsigned char length;
    unsigned char buffer[64];
} can_message_t;

/**
 * Struct for the definition of repeated CAN messages that are sent from the ECU
 */
typedef struct msg_def_t {
    unsigned int id;
    unsigned int dlc;
    bool enb;
    unsigned int freq;
} msg_def_t;

extern bool b_100_hz;
extern bool b_20_hz;
extern bool b_10_hz;
extern bool b_2_hz;

extern long can_msg_timings_receive[];
extern long can_msg_timings_send[];
extern unsigned int can_reverence_timings[];

extern bool gateway_read_whitelist[];
extern bool gateway_write_whitelist[];

/**
 * Initializes the vCAN Bus integrated in the Linux kernel by creating a socket and binding to it.
 * @return 0 if the initialization succeeded
 * @return -1 if the socket could not be created
 * @return -2 if the socket could not be bound to
 */
int init_can(char *interface_name);

/**
 * Defines a CAN message with id that is sent repeatedly with a specific period
 * @param id The ID of the CAN message
 * @param dlc The length of the messages content
 * @param enb Bool to control if the message is enabled, only enabled messages will be sent
 * @param period Period with which the message should be sent in milliseconds
 */
void define_rep_msg(unsigned int id, unsigned int dlc, bool enb, unsigned int period);

/**
 * Reads a message from the vCan bus and stores it in the pointer to a can_message_t
 * @param msg pointer to a can_message_t
 * @param tx_socket can socket for the transmission
 * @return
 */
ssize_t read_can(can_message_t *msg, int tx_socket);

/**
 * Writes the content of a can_message_t variable to the vCan bus
 * @param msg can_message_t variable
 * @param tx_socket can socket for the transmission
 * @return
 */
int write_can(can_message_t msg, int tx_socket);

/**
 * Callback function for the 100Hz timer
 * Sends all the CAN messages that were defined with a 10ms interval
 */
void can_write_100_hz_msgs();

/**
 * Callback function for the 20Hz timer
 * Sends all the CAN messages that were defined with a 50ms interval
 */
void can_write_20_hz_msgs();

/**
 * Callback function for the 10Hz timer
 * Sends all the CAN messages that were defined with a 100ms interval
 */
void can_write_10_hz_msgs();

/**
 * Callback function for the 2Hz timer
 * Sends all the CAN messages that were defined with a 500ms interval
 */
void can_write_2_hz_msgs();

int send_can_message(msg_def_t msg);

int send_pending_can_messages();

/**
 * High level function that reads the vCan bus and calls the message handler for the correct ECU
 */
void read_can_bus_and_handle_input();

/**
 *
 * Loop for gateway ecu running in extra thread to stop the blocking of can messages, reads vcan1 and writes to vcan0
 */
void *gateway_read_obd_port_loop();

/**
 * Incoming CAN message handler for the powertrain ECU
 * Updates the ecu_data with the values from the incoming CAN message
 * @param msg incoming CAN message
 */
void powertrain_handle_can_msg(can_message_t msg);

/**
 * Incoming CAN message handler for the chassis ECU
 * Updates the ecu_data with the values from the incoming CAN message
 * @param msg incoming CAN message
 */
void chassis_handle_can_msg(can_message_t msg);

/**
 * Incoming CAN message handler for the body ECU
 * Updates the ecu_data with the values from the incoming CAN message
 * @param msg incoming CAN message
 */
void body_handle_can_msg(can_message_t msg);

/**
 * Incoming CAN message handler for the observer ECU
 * Updates the ecu_data with the values from the incoming CAN message
 * @param msg incoming CAN message
 */
void observer_handle_can_msg(can_message_t msg);

/**
 * Incoming CAN message handler for the gateway ECU
 * Distributes the message to the right ECU and filters it if it does not match certain security criteria
 * @param msg  incoming CAN message
 */
void gateway_handle_can_msg(can_message_t msg, int receiving_socket);

/**
 * @brief Set the up observer reference timings, to check if the received CAN messages have an unusual timing
 *
 */
void setup_observer_reference_timings();

/**
 * @brief Set the up a whitlist of CAN IDs that are allowed to be sent and received by the OBD-II port
 *
 */
void setup_gateway_whitelist();

#endif // PENNE_CAN_H
