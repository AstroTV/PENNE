#include "can.h"
#include "crypto.h"
#include "ecu.h"
#include "helpers.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc < 3 || argc > 4) {
    printf("Invalid number of args provided!\n");
    return -1;
  }
  if (strcmp(argv[1], "powertrain") == 0)
    ecu_number = POWERTRAIN;
  else if (strcmp(argv[1], "chassis") == 0)
    ecu_number = CHASSIS;
  else if (strcmp(argv[1], "body") == 0)
    ecu_number = BODY;
  else if (strcmp(argv[1], "gateway") == 0)
    ecu_number = GATEWAY;
  else if (strcmp(argv[1], "observer") == 0)
    ecu_number = OBSERVER;
  else {
    fprintf(stderr, "Error parsing ecu_type, allowed: [\"powertrain\", "
                    "\"chassis\", \"body\", \"gateway\", \"observer\"]");
    return -2;
  }
  printf("Starting %s ECU on port %s\n", argv[1], argv[2]);
  printf("Initializing serial port %s\n", argv[2]);
  if (init_serial_port(argv[2]) != 0) {
    return -3;
  }

  // ECU was started with an encryption key as commandline argument
  if (argc == 4) {
    encryption_key = (unsigned char *)argv[3];
    using_encryption = true;
    printf("Using encryption with key: %x %x %x %x ...\n", encryption_key[0], encryption_key[1], encryption_key[2], encryption_key[3]);
  } else {
    using_encryption = false;
  }

  printf("Initializing CAN socket\n");

  if (init_can("vcan0") != 0) {
    return -4;
  }
  // The GATEWAY ECU starts it's own additional CAN bus where the OBD-II Port is connected
  if (ecu_number == GATEWAY) {
    if (init_can("vcan1") != 0) {
      return -4;
    }
  }
  printf("Setting up %s ECU\n", argv[1]);
  ecu_setup();

  pthread_t pth;
  if (ecu_number == GATEWAY) {
    if (pthread_mutex_init(&gateway_lock, NULL) != 0) {
      perror("Failed to initialize gateway lock\n");
      return -5;
    }
    if (pthread_create(&pth, NULL, gateway_read_obd_port_loop, NULL) != 0) {
      perror("Failed to create Gateway thread!\n");
      return -6;
    }
  }

  printf("Starting main loop\n");
  while (loop() == 0) {
  }
  if (ecu_number == GATEWAY) {
    if (pthread_cancel(pth) != 0) {
      perror("Failed to cancel Gateway thread!\n");
    }
    if (pthread_mutex_destroy(&gateway_lock) != 0) {
      perror("Failed to destroy gateway lock!\n");
    }
  }
  return 0;
}
