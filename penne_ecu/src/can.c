#include "can.h"
#include "crypto.h"
#include "ecu.h"
#include "helpers.h"
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <openssl/conf.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>

int vcan0_tx_socket;
int vcan1_tx_socket;
static can_message_t out_msg;
msg_def_t msg_array[32] = {0};
unsigned long last_can_msg = 0;
long can_msg_timings_receive[HIGHEST_POSSIBLE_CAN_ID + 1] = {0};
long can_msg_timings_send[HIGHEST_POSSIBLE_CAN_ID + 1] = {0};
int can_reverence_timings[HIGHEST_POSSIBLE_CAN_ID + 1] = {0};
bool gateway_read_whitelist[HIGHEST_POSSIBLE_CAN_ID + 1] = {0};
bool gateway_write_whitelist[HIGHEST_POSSIBLE_CAN_ID + 1] = {0};

int init_can(char *interface_name) {

  int enable_canfd = 1;
  struct sockaddr_can addr;
  struct ifreq ifr;
  struct timeval tv;

  // open socket
  int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (s < 0) {
    perror("Socket");
    return -1;
  }

  strcpy(ifr.ifr_name, interface_name);
  ioctl(s, SIOCGIFINDEX, &ifr);

  memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  // check if the frame fits into the CAN netdevice
  if (ioctl(s, SIOCGIFMTU, &ifr) < 0) {
    perror("SIOCGIFMTU");
    return 1;
  }
  int mtu = ifr.ifr_mtu;

  if (mtu != CANFD_MTU) {
    printf("CAN interface is not CAN FD capable - sorry.\n");
    return 1;
  }

  // interface is ok - try to switch the socket into CAN FD mode
  if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd))) {
    printf("error when enabling CAN FD support\n");
    return 1;
  }

  tv.tv_sec = 0;
  tv.tv_usec = 100;
  if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
    perror("Error setting CAN socket options");
    return -2;
  }

  if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    perror("Bind");
    return -4;
  }
  if (strcmp(interface_name, "vcan0") == 0) {
    vcan0_tx_socket = s;
  } else if (strcmp(interface_name, "vcan1") == 0) {
    vcan1_tx_socket = s;
  } else {
    perror("Interface name must be vcan0 or vcan1");
    return -5;
  }
  return 0;
}

void define_rep_msg(unsigned int id, unsigned int dlc, bool enb, unsigned int period) {
  for (int i = 0; i < MAX_MSGS; i++) {
    if (msg_array[i].id == 0) {
      msg_array[i].id = id;
      msg_array[i].dlc = dlc;
      msg_array[i].enb = enb;
      msg_array[i].freq = period;
      break;
    }
  }
}

ssize_t read_can(can_message_t *msg, int tx_socket) {
  ssize_t n_bytes;
  struct canfd_frame frame;

  if (msg == NULL) {
    return -1;
  }

  n_bytes = read(tx_socket, &frame, sizeof(struct canfd_frame));
  if (n_bytes < 0) {
    return 0;
  }

  msg->id = frame.can_id;
  msg->length = frame.len;

  // If encryption is used we have to decrypt the frame data and check the tag
  if (using_encryption) {

    // Additional authenticated data, we use this to add the ECU name of the sender (BD, CH, PT, OB, GW)
    unsigned char aad[8];
    size_t aad_len = 8;
    // The message tag that is used for the authentication
    unsigned char tag[16];
    size_t tag_len = 16;
    // Buffer for the decrypted message
    unsigned char decryptedtext[128];
    // Initialization vector that is transmitted in plaintext with the message
    unsigned char iv[16];
    size_t iv_len = 16;
    // Buffer for the ciphertext that we copy out of the canfd message
    unsigned char ciphertext[16];
    size_t ciphertext_len = 16;

    // The payload of the CANFD message looks like this:
    // <8 byte Ciphertext> <16 byte Tag> <4 byte additional authenticated data> <18 byte IV>
    // First we copy these fields into our variables
    memcpy(ciphertext, frame.data, ciphertext_len);
    memcpy(tag, frame.data + ciphertext_len, tag_len);
    memcpy(aad, frame.data + ciphertext_len + tag_len, aad_len);
    memcpy(iv, frame.data + ciphertext_len + tag_len + aad_len, iv_len);

    // Reconstruct the timestamp of the received message out of the 8 bytes of aad
    long timestamp = (long) aad[0] + ((long) aad[1] << 8) + ((long) aad[2] << 16) + ((long) aad[3] << 24) +
                     ((long) aad[4] << 32) + ((long) aad[5] << 40) + ((long) aad[6] << 48) + ((long) aad[7] << 56);

    // Get current timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Check if the message was generated more than 1 second ago
    if (tv.tv_sec - timestamp > 1) {
      // We detected a replay attack
      printf("Replay Attack detected! Ignoring message!\n");
      return 0;
    }

    // Then we decrypt the Ciphertext with our commonly shared key
    int decryptedtext_len = gcm_decrypt(ciphertext, ciphertext_len, (unsigned char *) aad, aad_len, tag, encryption_key,
                                        iv, iv_len, decryptedtext);
    if (decryptedtext_len > 0) {
      memcpy(msg->buffer, decryptedtext, 16);
    } else {
      // if the message and the tag do not match, the decryption fails and we return -1
      printf("Decryption failed, ignoring message\n");
      return 0;
    }
    printf("Received message %x%x%x%x%x%x%x%x at %lu\n", tag[0], tag[1], tag[2], tag[3], tag[4], tag[5], tag[6], tag[7],
           micros());
  } else {
    memcpy(msg->buffer, frame.data, 16);
  }
  return n_bytes;
}

int write_can(can_message_t msg, int tx_socket) {
  struct canfd_frame frame;
  frame.can_id = msg.id;
  frame.len = 64;
  // First we set the 64 byte canfd_frame data to 0
  memset(frame.data, 0, frame.len);

  if (msg.length > 64) {
    perror("CAN Write, Invalid payload length");
    return -1;
  }

  // We only encrypt the message if the encryption flag is set and the key is not NULL
  if (using_encryption && encryption_key != NULL) {
    // Create random 128bit nonce for the IV
    unsigned char iv[16];
    size_t iv_len = 16;
    for (size_t i = 0; i < iv_len; i++)
      iv[i] = rand() & 0xFF;

    unsigned char ciphertext[128];

    // Buffer for the decrypted text
    unsigned char decryptedtext[128];

    // Buffer for the tag
    unsigned char tag[16];
    size_t tag_len = 16;

    // Get the current timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // We use the current time in seconds as our unencrypted "additional authenticated data"
    // This prevents replay attacks
    unsigned char aad[8];
    // long has 8 bytes
    size_t aad_len = 8;
    aad[0] = tv.tv_sec & 0xFF;
    aad[1] = tv.tv_sec >> 8 & 0xFF;
    aad[2] = tv.tv_sec >> 16 & 0xFF;
    aad[3] = tv.tv_sec >> 24 & 0xFF;
    aad[4] = tv.tv_sec >> 32 & 0xFF;
    aad[5] = tv.tv_sec >> 40 & 0xFF;
    aad[6] = tv.tv_sec >> 48 & 0xFF;
    aad[7] = tv.tv_sec >> 56 & 0xFF;
    int ciphertext_len = gcm_encrypt(msg.buffer, 16, aad, aad_len, encryption_key, iv, iv_len, ciphertext, tag);
    if (ciphertext_len > 0) {
      // We copy the ciphertext, tag, aad and IV into the canfd message buffer
      memcpy(frame.data, ciphertext, ciphertext_len);
      memcpy(frame.data + ciphertext_len, tag, 16);
      memcpy(frame.data + ciphertext_len + tag_len, aad, aad_len);
      memcpy(frame.data + ciphertext_len + tag_len + aad_len, iv, iv_len);

    } else {
      printf("Encryption failed\n");
      return -2;
    }

    printf("Sending message %x%x%x%x%x%x%x%x at %lu\n", tag[0], tag[1], tag[2], tag[3], tag[4], tag[5], tag[6], tag[7],
           micros());
  } else {
    // If no encryption is used we simply copy the first 8 bytes from the message struct to the canfd_frame
    memcpy(frame.data, msg.buffer, 16);
  }

  int nbytes = (int) write(tx_socket, &frame, sizeof(struct canfd_frame));
  if (nbytes != sizeof(struct canfd_frame)) {
    perror("CAN Write");
    return -3;
  }
  return nbytes;
}

int send_can_message(msg_def_t msg) {
  memset(&out_msg, 0, sizeof(can_message_t));
  out_msg.length = msg.dlc;
  out_msg.id = msg.id;
  switch (msg.id) {
    case BRAKE_OUTPUT_IND_MSG:
      out_msg.buffer[0] = ((ecu_data.brake_output >> 8) & 0xFF);
      out_msg.buffer[1] = ((ecu_data.brake_output >> 0) & 0xFF);
      break;
    case ENGINE_RPM_MSG:
      out_msg.buffer[0] = ((ecu_data.engine_rpm >> 8) & 0xFF);
      out_msg.buffer[1] = ((ecu_data.engine_rpm >> 0) & 0xFF);
      out_msg.buffer[2] = ((ecu_data.speed_kph >> 8) & 0xFF);
      out_msg.buffer[3] = ((ecu_data.speed_kph >> 0) & 0xFF);
      break;
    case POWER_STEERING_OUT_IND_MSG:
      out_msg.buffer[0] = ((ecu_data.power_steering >> 8) & 0xFF);
      out_msg.buffer[1] = ((ecu_data.power_steering >> 0) & 0xFF);
      break;
    case SHIFT_POSITION_MSG:
      out_msg.buffer[0] = ecu_data.shift_position;
      break;
    case BRAKE_OPERATION_MSG:
      out_msg.buffer[0] = ((ecu_data.brake_value >> 8) & 0xFF);
      out_msg.buffer[1] = ((ecu_data.brake_value >> 0) & 0xFF);
      break;
    case ACCELERATION_OPERATION_MSG:
      out_msg.buffer[0] = ((ecu_data.accelerator_value >> 8) & 0xFF);
      out_msg.buffer[1] = ((ecu_data.accelerator_value >> 0) & 0xFF);
      break;
    case STEERING_WHEEL_POS_MSG:
      out_msg.buffer[0] = ((ecu_data.steering_value >> 8) & 0xFF);
      out_msg.buffer[1] = ((ecu_data.steering_value >> 0) & 0xFF);
      break;
    case SHIFT_POSITION_SWITCH_MSG:
      out_msg.buffer[0] = ecu_data.shift_value;
      break;
    case ENGINE_START_MSG:
      out_msg.buffer[0] = ecu_data.engine_value;
      break;
    case TURN_SWITCH_MSG:
      out_msg.buffer[0] = ecu_data.turn_switch_value;
      out_msg.buffer[1] = ecu_data.hazard_value;
      break;
    case HORN_SWITCH_MSG:
      out_msg.buffer[0] = ecu_data.horn_value;
      break;
    case TURN_SIGNAL_INDICATOR_MSG:
      out_msg.buffer[0] = ecu_data.turn_signal_indicator;
      break;
    case ENGINE_STATUS_MSG:
      out_msg.buffer[0] = ecu_data.engine_status;
      break;
    case PARKING_BRAKE_STATUS_MSG:
      out_msg.buffer[0] = ecu_data.parking_brake_status;
      break;
    case LIGHT_SWITCH_MSG:
      out_msg.buffer[0] = ecu_data.light_switch_value;
      break;
    case PARKING_BRAKE_MSG:
      out_msg.buffer[0] = ecu_data.parking_value;
      break;
    case WIPER_SWITCH_FRONT_MSG:
      out_msg.buffer[0] = ecu_data.wiper_f_sw_value;
      break;
    case WIPER_SWITCH_REAR_MSG:
      out_msg.buffer[0] = ecu_data.wiper_r_sw_value;
      break;
    case DOOR_LOCK_UNLOCK_MSG:
      out_msg.buffer[0] = ecu_data.door_lock_value;
      break;
    case L_DOOR_HANDLE_MSG:
      out_msg.buffer[0] = ecu_data.l_door_handle_value;
      break;
    case R_DOOR_HANDLE_MSG:
      out_msg.buffer[0] = ecu_data.r_door_handle_value;
      break;
    case L_WINDOW_SWITCH_MSG:
      out_msg.buffer[0] = ecu_data.l_window_switch_value;
      break;
    case R_WINDOW_SWITCH_MSG:
      out_msg.buffer[0] = ecu_data.r_window_switch_value;
      break;
    case DOOR_LOCK_STATUS_MSG:
      out_msg.buffer[0] = ecu_data.door_lock_status;
      break;
    case L_DOOR_POSITION_MSG:
      out_msg.buffer[0] = ecu_data.l_door_position;
      break;
    case R_DOOR_POSITION_MSG:
      out_msg.buffer[0] = ecu_data.r_door_position;
      break;
    default:
      return -1;
  }
  return write_can(out_msg, vcan0_tx_socket);
}

int send_pending_can_messages() {
  int sent_messages = 0;


  for (int i = 0; i < MAX_MSGS; i++) {
    msg_def_t msg = msg_array[i];
    if (msg.enb) {
      long current_time = micros();
      if ((long) msg.freq * 1000 - (current_time - can_msg_timings_send[msg.id]) < 300) {
        int ret = send_can_message(msg);
        if (ret <= 0) {
          printf("Failed to write CAN message ID: 0x%x, Error Code: %d\n", msg.id, ret);
        }
        can_msg_timings_send[msg.id] = current_time;
        sent_messages++;
        usleep(CAN_MSG_SPACING);
      }
    }
  }
  return sent_messages;
}


void read_can_bus_and_handle_input() {
  can_message_t msg;
  // Check the vcan0 interface for a new message
  ssize_t n_bytes = read_can(&msg, vcan0_tx_socket);
  if (n_bytes > 0) {
    switch (ecu_number) {
      case POWERTRAIN:
        powertrain_handle_can_msg(msg);
        break;
      case CHASSIS:
        chassis_handle_can_msg(msg);
        break;
      case BODY:
        body_handle_can_msg(msg);
        break;
      case OBSERVER:
        observer_handle_can_msg(msg);
        break;
      case GATEWAY:
        gateway_handle_can_msg(msg, vcan0_tx_socket);
        break;
      default:
        break;
    }
  }
}

void *gateway_read_obd_port_loop() {
  can_message_t msg;
  ssize_t n_bytes;

  while (1) {
    n_bytes = read_can(&msg, vcan1_tx_socket);
    if (n_bytes > 0) {
      gateway_handle_can_msg(msg, vcan1_tx_socket);
    }
  }
}

void powertrain_handle_can_msg(can_message_t msg) {
  switch (msg.id) {
    // 100Hz
    case BRAKE_OPERATION_MSG:
      ecu_data.brake_value = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
    case ACCELERATION_OPERATION_MSG:
      ecu_data.accelerator_value = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
    case STEERING_WHEEL_POS_MSG:
      ecu_data.steering_value = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
    case SHIFT_POSITION_SWITCH_MSG:
      ecu_data.shift_value = (msg.buffer[0]);
      break;
    case ENGINE_START_MSG:
      ecu_data.engine_value = (msg.buffer[0]);
      break;
    case PARKING_BRAKE_MSG:
      ecu_data.parking_value = (msg.buffer[0]);
      break;
    default:
      break;
  }
  last_can_msg = millis();
}

void chassis_handle_can_msg(can_message_t msg) {
  switch (msg.id) {
    case BRAKE_OUTPUT_IND_MSG:
      ecu_data.brake_output = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
    case ENGINE_RPM_MSG:
      ecu_data.engine_rpm = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      ecu_data.speed_kph = ((msg.buffer[2] << 8) | (msg.buffer[3]));
      break;
    case POWER_STEERING_OUT_IND_MSG:
      ecu_data.power_steering = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
    case SHIFT_POSITION_MSG:
      ecu_data.shift_position = (msg.buffer[0]);
      break;
    case TURN_SIGNAL_INDICATOR_MSG:
      ecu_data.turn_signal_indicator = (msg.buffer[0]);
      break;
      // 20Hz
    case ENGINE_STATUS_MSG:
      ecu_data.engine_status = (msg.buffer[0] & 0x01);
      break;
    case PARKING_BRAKE_STATUS_MSG:
      ecu_data.parking_brake_status = (msg.buffer[0] & 0x01);
      break;
      // 10Hz
    case DOOR_LOCK_STATUS_MSG:
      ecu_data.door_lock_status = (msg.buffer[0]);
      if (((ecu_data.door_lock_status & 0x04) >> 2) & ((ecu_data_old.door_lock_status & 0x01) >> 0)) {
        ecu_data.door_lock_value = ecu_data.door_lock_value & 0xfe;
      } else if (((ecu_data.door_lock_status & 0x08) >> 3) & ((ecu_data_old.door_lock_status & 0x02) >> 1)) {
        ecu_data.door_lock_value = ecu_data.door_lock_value & 0xfd;
      }
      break;
      // 2Hz
    case L_DOOR_POSITION_MSG:
      ecu_data.l_door_position = (msg.buffer[0]);
      break;
    case R_DOOR_POSITION_MSG:
      ecu_data.r_door_position = (msg.buffer[0]);
      break;

    default:
      break;
  }
  last_can_msg = millis();
}

void body_handle_can_msg(can_message_t msg) {
  switch (msg.id) {

    // 100Hz
    case TURN_SWITCH_MSG:
      ecu_data.turn_switch_value = msg.buffer[0];
      ecu_data.hazard_value = msg.buffer[1];
      break;
    case HORN_SWITCH_MSG:
      ecu_data.horn_value = (msg.buffer[0] & 0x01);
      break;
    case BRAKE_OPERATION_MSG:
      ecu_data.brake_value = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
      // 20Hz
    case LIGHT_SWITCH_MSG:
      ecu_data.light_switch_value = (msg.buffer[0] & 0x07);
      break;
      // 10Hz
    case WIPER_SWITCH_FRONT_MSG:
      ecu_data.wiper_f_sw_value = (msg.buffer[0]);
      break;
    case WIPER_SWITCH_REAR_MSG:
      ecu_data.wiper_r_sw_value = (msg.buffer[0]);
      break;
    case DOOR_LOCK_UNLOCK_MSG:
      ecu_data.door_lock_value = (msg.buffer[0]);
      break;
    case L_DOOR_HANDLE_MSG:
      ecu_data.l_door_handle_value = (msg.buffer[0]);
      break;
    case R_DOOR_HANDLE_MSG:
      ecu_data.r_door_handle_value = (msg.buffer[0]);
      break;
    case L_WINDOW_SWITCH_MSG:
      ecu_data.l_window_switch_value = (msg.buffer[0]);
      break;
    case R_WINDOW_SWITCH_MSG:
      ecu_data.r_window_switch_value = (msg.buffer[0]);
      break;

    default:

      break;
  }
  last_can_msg = millis();
}

void observer_handle_can_msg(can_message_t msg) {

  if (micros() - last_observer_warning > 1000000) {
    // We only reset if the last warning was more than 1 second ago, so it is displayed in the GUI better
    // Reset Observer to OK, it will be overwritten if we find anything irregular
    ecu_data.observer_id = NONE;
    ecu_data.observer_code = OK;
  }


  last_can_msg = micros();

  // Ignore Messages with a higher ID so we don't get an access out of bounds
  if (msg.id >= 0xFFF) {
    return;
  }
  if (can_msg_timings_receive[msg.id] != 0 && can_reverence_timings[msg.id] !=
                                              0) { // if can_msg_timings[msg.id] is 0, the message was received for the first time
    int time_deviation = last_can_msg - can_msg_timings_receive[msg.id] - can_reverence_timings[msg.id] * 1000;

    if (time_deviation > 6000 || time_deviation < -6000) {
      printf("ID: 0x%zx, reference timing: %u\n", msg.id, can_reverence_timings[msg.id]);
      printf("Deviation: %d us\n", time_deviation);
      ecu_data.observer_code = BAD_TIMING;
      ecu_data.observer_id = msg.id;
    }
  }

  switch (msg.id) {

    // 100Hz
    case TURN_SWITCH_MSG:
      ecu_data.turn_switch_value = msg.buffer[0];
      ecu_data.hazard_value = msg.buffer[1];
      break;
    case HORN_SWITCH_MSG:
      ecu_data.horn_value = (msg.buffer[0] & 0x01);
      break;
    case BRAKE_OPERATION_MSG:
      ecu_data.brake_value = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
      // 20Hz
    case LIGHT_SWITCH_MSG:
      ecu_data.light_switch_value = (msg.buffer[0] & 0x07);
      break;
      // 10Hz
    case WIPER_SWITCH_FRONT_MSG:
      ecu_data.wiper_f_sw_value = (msg.buffer[0]);
      break;
    case WIPER_SWITCH_REAR_MSG:
      ecu_data.wiper_r_sw_value = (msg.buffer[0]);
      break;
    case DOOR_LOCK_UNLOCK_MSG:
      ecu_data.door_lock_value = (msg.buffer[0]);
      break;
    case L_DOOR_HANDLE_MSG:
      ecu_data.l_door_handle_value = (msg.buffer[0]);
      break;
    case R_DOOR_HANDLE_MSG:
      ecu_data.r_door_handle_value = (msg.buffer[0]);
      break;
    case L_WINDOW_SWITCH_MSG:
      ecu_data.l_window_switch_value = (msg.buffer[0]);
      break;
    case R_WINDOW_SWITCH_MSG:
      ecu_data.r_window_switch_value = (msg.buffer[0]);
      break;
    case BRAKE_OUTPUT_IND_MSG:
      ecu_data.brake_output = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
    case ENGINE_RPM_MSG:
      ecu_data.engine_rpm = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      ecu_data.speed_kph = ((msg.buffer[2] << 8) | (msg.buffer[3]));
      break;
    case POWER_STEERING_OUT_IND_MSG:
      ecu_data.power_steering = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
    case SHIFT_POSITION_MSG:
      ecu_data.shift_position = (msg.buffer[0]);
      break;
    case TURN_SIGNAL_INDICATOR_MSG:
      ecu_data.turn_signal_indicator = (msg.buffer[0]);
      break;
      // 20Hz
    case ENGINE_STATUS_MSG:
      ecu_data.engine_status = (msg.buffer[0] & 0x01);
      break;
    case PARKING_BRAKE_STATUS_MSG:
      ecu_data.parking_brake_status = (msg.buffer[0] & 0x01);
      break;
      // 10Hz
    case DOOR_LOCK_STATUS_MSG:
      ecu_data.door_lock_status = (msg.buffer[0]);
      if (((ecu_data.door_lock_status & 0x04) >> 2) & ((ecu_data_old.door_lock_status & 0x01) >> 0)) {
        ecu_data.door_lock_value = ecu_data.door_lock_value & 0xfe;
      } else if (((ecu_data.door_lock_status & 0x08) >> 3) & ((ecu_data_old.door_lock_status & 0x02) >> 1)) {
        ecu_data.door_lock_value = ecu_data.door_lock_value & 0xfd;
      }
      break;
      // 2Hz
    case L_DOOR_POSITION_MSG:
      ecu_data.l_door_position = (msg.buffer[0]);
      break;
    case R_DOOR_POSITION_MSG:
      ecu_data.r_door_position = (msg.buffer[0]);
      break;
    case ACCELERATION_OPERATION_MSG:
      ecu_data.accelerator_value = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
    case STEERING_WHEEL_POS_MSG:
      ecu_data.steering_value = ((msg.buffer[0] << 8) | (msg.buffer[1]));
      break;
    case SHIFT_POSITION_SWITCH_MSG:
      ecu_data.shift_value = (msg.buffer[0]);
      break;
    case ENGINE_START_MSG:
      ecu_data.engine_value = (msg.buffer[0]);
      break;
    case PARKING_BRAKE_MSG:
      ecu_data.parking_value = (msg.buffer[0]);
      break;
    default:
      ecu_data.observer_code = BAD_CAN_ID;
      ecu_data.observer_id = msg.id;
      break;
  }

  can_msg_timings_receive[msg.id] = last_can_msg;
}

void gateway_handle_can_msg(can_message_t msg, int receiving_socket) {
  if (msg.id > HIGHEST_POSSIBLE_CAN_ID) {
    perror("Gatway ECU received invalid CAN ID");
    return;
  }
  pthread_mutex_lock(&gateway_lock);
  ecu_data.gateway_id = msg.id;
  ecu_data.gateway_code = 0; // OK
  if (receiving_socket == vcan0_tx_socket) {
    if (gateway_read_whitelist[msg.id] == true) {
      write_can(msg, vcan1_tx_socket);
    } else {
      ecu_data.gateway_code = 1; // READ_BLOCKED
    }
  }

  if (receiving_socket == vcan1_tx_socket) {
    if (gateway_write_whitelist[msg.id] == true) {
      write_can(msg, vcan0_tx_socket);
    } else {
      ecu_data.gateway_code = 2; // WRITE_BLOCKED
    }
  }
  pthread_mutex_unlock(&gateway_lock);
}

void setup_observer_reference_timings() {
  can_reverence_timings[BRAKE_OUTPUT_IND_MSG] = 10;
  can_reverence_timings[ENGINE_RPM_MSG] = 10;
  can_reverence_timings[POWER_STEERING_OUT_IND_MSG] = 10;
  can_reverence_timings[SHIFT_POSITION_MSG] = 10;
  can_reverence_timings[BRAKE_OPERATION_MSG] = 10;
  can_reverence_timings[ACCELERATION_OPERATION_MSG] = 10;
  can_reverence_timings[STEERING_WHEEL_POS_MSG] = 10;
  can_reverence_timings[SHIFT_POSITION_SWITCH_MSG] = 10;
  can_reverence_timings[ENGINE_START_MSG] = 10;
  can_reverence_timings[TURN_SWITCH_MSG] = 10;
  can_reverence_timings[HORN_SWITCH_MSG] = 10;
  can_reverence_timings[TURN_SIGNAL_INDICATOR_MSG] = 10;
  can_reverence_timings[ENGINE_STATUS_MSG] = 50;
  can_reverence_timings[PARKING_BRAKE_STATUS_MSG] = 50;
  can_reverence_timings[LIGHT_SWITCH_MSG] = 50;
  can_reverence_timings[PARKING_BRAKE_MSG] = 50;
  can_reverence_timings[WIPER_SWITCH_FRONT_MSG] = 100;
  can_reverence_timings[WIPER_SWITCH_REAR_MSG] = 100;
  can_reverence_timings[DOOR_LOCK_UNLOCK_MSG] = 100;
  can_reverence_timings[L_WINDOW_SWITCH_MSG] = 100;
  can_reverence_timings[R_WINDOW_SWITCH_MSG] = 100;
  can_reverence_timings[L_DOOR_HANDLE_MSG] = 100;
  can_reverence_timings[R_DOOR_HANDLE_MSG] = 100;
  can_reverence_timings[DOOR_LOCK_STATUS_MSG] = 100;
  can_reverence_timings[L_DOOR_POSITION_MSG] = 500;
  can_reverence_timings[R_DOOR_POSITION_MSG] = 500;
}

void setup_gateway_whitelist() {
  // true -> reading/writing allowed, by default all values are 0/false
  gateway_read_whitelist[BRAKE_OUTPUT_IND_MSG] = false;
  gateway_read_whitelist[ENGINE_RPM_MSG] = true;
  gateway_read_whitelist[POWER_STEERING_OUT_IND_MSG] = true;
  gateway_read_whitelist[SHIFT_POSITION_MSG] = true;
  gateway_read_whitelist[BRAKE_OPERATION_MSG] = true;
  gateway_read_whitelist[ACCELERATION_OPERATION_MSG] = true;
  gateway_read_whitelist[STEERING_WHEEL_POS_MSG] = true;
  gateway_read_whitelist[SHIFT_POSITION_SWITCH_MSG] = true;
  gateway_read_whitelist[ENGINE_START_MSG] = true;
  gateway_read_whitelist[TURN_SWITCH_MSG] = true;
  gateway_read_whitelist[HORN_SWITCH_MSG] = true;
  gateway_read_whitelist[TURN_SIGNAL_INDICATOR_MSG] = true;
  gateway_read_whitelist[ENGINE_STATUS_MSG] = true;
  gateway_read_whitelist[PARKING_BRAKE_STATUS_MSG] = true;
  gateway_read_whitelist[LIGHT_SWITCH_MSG] = true;
  gateway_read_whitelist[PARKING_BRAKE_MSG] = true;
  gateway_read_whitelist[WIPER_SWITCH_FRONT_MSG] = true;
  gateway_read_whitelist[WIPER_SWITCH_REAR_MSG] = true;
  gateway_read_whitelist[DOOR_LOCK_UNLOCK_MSG] = true;
  gateway_read_whitelist[L_WINDOW_SWITCH_MSG] = true;
  gateway_read_whitelist[R_WINDOW_SWITCH_MSG] = true;
  gateway_read_whitelist[L_DOOR_HANDLE_MSG] = true;
  gateway_read_whitelist[R_DOOR_HANDLE_MSG] = true;
  gateway_read_whitelist[DOOR_LOCK_STATUS_MSG] = true;
  gateway_read_whitelist[L_DOOR_POSITION_MSG] = true;
  gateway_read_whitelist[R_DOOR_POSITION_MSG] = true;
}
