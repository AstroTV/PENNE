//
// Created by christoph on 01.02.21.
//
#include "helpers.h"
#include "ecu.h"
#include <errno.h> // Error integer and strerror() function
#include <fcntl.h> // Contains file controls like O_RDWR
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <termios.h> // Contains POSIX terminal control definitions
#include <time.h>

sci_console_t sci_console;

timer_t timer_100_hz;
timer_t timer_20_hz;
timer_t timer_10_hz;
timer_t timer_2_hz;

int serial_port;

long millis() {
  long ms;  // Milliseconds
  time_t s; // Seconds
  struct timespec spec;

  clock_gettime(CLOCK_REALTIME, &spec);

  s = spec.tv_sec;
  ms = (long)round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
  if (ms > 999) {
    s++;
    ms = 0;
  }
  return s * 1000 + ms;
}

long micros() {
  long us;  // Microseconds
  time_t s; // Seconds
  struct timespec spec;

  clock_gettime(CLOCK_REALTIME, &spec);

  s = spec.tv_sec;
  us = (long)round(spec.tv_nsec / 1.0e3); // Convert nanoseconds to microseconds
  if (us > 999999) {
    s++;
    us = 0;
  }
  return s * 1000000 + us;
}

int make_timer(timer_t *timer_id, int expire_ms, int interval_ms) {
  struct sigevent te;
  struct itimerspec its;
  struct sigaction sa;
  int sig_no = SIGRTMIN;

  /* Set up signal handler. */
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = timer_handler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(sig_no, &sa, NULL) == -1) {
    perror("sigaction");
    return -1;
  }

  /* Set and enable alarm */
  te.sigev_notify = SIGEV_SIGNAL;
  te.sigev_signo = sig_no;
  te.sigev_value.sival_ptr = timer_id;
  timer_create(CLOCK_REALTIME, &te, timer_id);

  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = interval_ms * 1000000;
  its.it_value.tv_sec = 0;
  its.it_value.tv_nsec = expire_ms * 1000000;
  timer_settime(*timer_id, 0, &its, NULL);

  return 0;
}

int init_serial_port(char *port) {
  char serial_port_name[strlen(SERIAL_PORT) + strlen(port)];
  strcpy(serial_port_name, SERIAL_PORT);
  strcat(serial_port_name, port);

  serial_port = open(serial_port_name, O_RDWR | O_NOCTTY | O_SYNC);

  if (serial_port < 0) {
    printf("Error %i from open: %s\n", errno, strerror(errno));
    return -1;
  }

  // Create new termios struct, we call it 'tty' for convention
  // No need for "= {0}" at the end as we'll immediately write the existing
  // config to this struct
  struct termios tty;

  // Read in existing settings, and handle any error
  // NOTE: This is important! POSIX states that the struct passed to tcsetattr()
  // must have been initialized with a call to tcgetattr() overwise behaviour
  // is undefined
  if (tcgetattr(serial_port, &tty) != 0) {
    printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
    return -2;
  }

  tty.c_cflag &= ~PARENB;
  tty.c_cflag |= CSTOPB; // Set stop field, two stop bits used in communication

  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;

  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CREAD | CLOCAL;

  tty.c_lflag &= ~ICANON;

  tty.c_lflag &= ~ECHO;   // Disable echo
  tty.c_lflag &= ~ECHOE;  // Disable erasure
  tty.c_lflag &= ~ECHONL; // Disable new-line echo

  tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl

  tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g.
                         // newline chars)
  tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

  tty.c_cc[VTIME] = 0; // Wait for up to 1s (10 deciseconds), returning as soon
                       // as any data is received.
  tty.c_cc[VMIN] = 0;

  // Set in/out baud rate to be 9600
  cfsetispeed(&tty, B9600);
  cfsetospeed(&tty, B9600);

  // Save tty settings, also checking for error
  if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
    return -3;
  }
  return 0;
}

ssize_t read_chassis_ecu_data() {
  int i;
  ssize_t num_bytes;
  char buffer[128] = {0};
  char c;
  num_bytes = read(serial_port, buffer, sizeof(buffer));

  if (num_bytes != 0) {
    for (i = 0; i < num_bytes; i++) {
      c = buffer[i];
      if (c == 0x0D) { //  [CR]
        sci_console.buffer[sci_console.write_pointer] = 0;
        sci_console.write_pointer = 0;
        // Finished reading, we can now process the command
        command_job(sci_console.buffer);
      } else if (c == 0x08 || c == 0x7F) { // Delete 1 character
        if (sci_console.write_pointer > 0) {
          sci_console.write_pointer--;
        }
      } else {
        if (c >= 0x61 && c <= 0x7a) { // Convert lowercase to uppercase
          sci_console.buffer[sci_console.write_pointer] = c - 0x20;
        } else {
          sci_console.buffer[sci_console.write_pointer] = c;
        }
        sci_console.write_pointer++;
        if (sci_console.write_pointer >= COMMAND_BUF_MAX) {
          sci_console.write_pointer = 0;
        }
      }
    }
  }
  return num_bytes;
}

void command_job(char *cmd) {
  if (*cmd == '\n') {
    cmd++;
  }
  if (cmd[0] == 'E' && cmd[1] == 'X' && cmd[2] == 'D') {
    ecu_input_update(cmd + 3);
  } else {
    printf("Invalid command: %s\n", cmd);
  }
  (*cmd)++;
}

void ecu_input_update(char *cmd) {
  int i, f;
  int id;
  char c;
  unsigned long d;

  if (cmd == 0 || *cmd == 0) {
    return;
  }
  // Rewriting process
  while (*cmd != 0) {
    while (*cmd == ' ') {
      cmd++;
    }
    // Get target ID
    id = 0;
    // Convert HEX to Decimal
    for (i = 0; i < 2; i++) {
      c = *cmd;
      if (c >= 'a' && c <= 'f') {
        c -= 0x27;
      }
      if (c >= 'A' && c <= 'F') {
        c -= 7;
      }
      if (c < 0x30 || c > 0x3F) {
        break;
      }
      id = (id << 4) | (unsigned char)(c & 0x0F);
      cmd++;
    }
    if (i == 2) { // && id < EX_IO_MAX) { // ID normal, data processing
      f = 0;
      d = 0;
      // Convert HEX to Decimal
      for (i = 0; i < 8; i++) {
        c = *cmd;
        if (c >= 'a' && c <= 'f') {
          c -= 0x27;
        }
        if (c >= 'A' && c <= 'F') {
          c -= 7;
        }
        if (c < 0x30 || c > 0x3F) {
          break;
        }
        d = (d << 4) | (unsigned char)(c & 0x0F);
        cmd++;
        f++;
      }
      if (f > 0) {
        set_ecu_id_to_value(id, d);
      }
    } else {
      printf("Failed\n");
      break;
    }
  }
}
