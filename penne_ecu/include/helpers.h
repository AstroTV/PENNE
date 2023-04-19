#ifndef PENNE_HELPERS_H
#define PENNE_HELPERS_H
#include <time.h>
#include <unistd.h>

#define SERIAL_PORT "/dev/pts/"
#define COMMAND_BUF_MAX 512

/**
 * buffer with a write-pointer to read messages from the GUI over multiple iterations
 */
typedef struct sci_console_t {
  int write_pointer;
  char buffer[COMMAND_BUF_MAX];
} sci_console_t;

extern timer_t timer_100_hz;
extern timer_t timer_20_hz;
extern timer_t timer_10_hz;
extern timer_t timer_2_hz;
extern int serial_port;

/**
 * Calculate the time the program has run since it's start in milliseconds
 * @return run-time in milliseconds
 */
long millis();

/**
 * Calculate the time the program has run since it's start in microseconds
 * @return run-time in milliseconds
 */
long micros();

/**
 * Helper function to set up the timers for the CAN messages
 * @param timer_id reference to the timer that is used (100Hz, 20Hz, 10Hz or 2Hz)
 * @param expire_ms the value in milliseconds after which the timer should expire the first time
 * @param interval_ms the interval that the timer should be reset to after expiring
 * @return 0 if the timer setup succeeded
 * @return -1 if the timer setup failed
 */
int make_timer(timer_t *timer_id, int expire_ms, int interval_ms);

/**
 * Helper function to setup the serial communication with the GUI over socat
 * @param port the tty port that should be used
 * @return 0 if the serial initialization succeeded
 * @return -1 if open(...) failed
 * @return -2 if tcgetattr(...) failed
 * @return -3 if tcsetattr(...) failed
 */
int init_serial_port(char *port);

/**
 * Helper function to read the serial port e.g. the interface with the GUI
 * @return The number of bytes read
 */
ssize_t read_chassis_ecu_data();

/**
 * Helper function to handle a command from the GUI after it was read-in completely.
 * If the command starts with "EXD", ecu_input_update(...) is called with the rest of the read command
 * @param cmd read in message from the GUI
 */
void command_job(char *cmd);

/**
 * Helper function to parse a "EXD ..." command from the GUI and extract the ID and the value of the ecu_data to update
 * @param cmd payload of a "EXD ..." message from the GUI
 */
void ecu_input_update(char *cmd);

#endif // PENNE_HELPERS_H
