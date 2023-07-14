#ifndef REMOTE_CAR_BLUETOOTH_H
#define REMOTE_CAR_BLUETOOTH_H

#include <gio/gio.h>
#include <glib.h>

#include "../include/helper.h"

#define BLUEZ_BUS_NAME  "org.bluez"
#define DEVICE_WRITE    "/service0023/char0024"
#define GATT_IFACE      "org.bluez.GattCharacteristic1"
#define DEVICE_IFACE    "org.bluez.Device1"

typedef struct device_t
{
    char name[10];
    char object_path[38];
    char write_path[59];
    GDBusConnection *con;
    GMainLoop *loop;
} device_t;

/**
 * Connects to the system bus and searches for new bluetooth devices. 
 * If the correct device is found, a connection gets established.
 * @param con The Connection to the system bus
 * @param value Set this value if the BLE device is unknown to the sytem bus, else set this value to 0
 * @return 0 if the initialization succeded
 * @return 1 if the initialization failed
*/
int init_bluetooth(device_t *device, char *name);

/**
 * 
*/

int get_object_path(device_t device);

/**
 * 
*/
void get_devices(GDBusConnection *con, GAsyncResult *res, gpointer data);

/**
 * 
*/
int get_name(GDBusConnection *con, GMainLoop *loop, const gchar *path);

/**
 * @param con
 * @return
*/
int connect_to_BLE(device_t device);

/**
 * @param con
*/
int disconnect_from_BLE(device_t device);


/**
 * @param con
 * @param command
 * @param values
 * @return
*/
int write_to_BLE(device_t device, char command, int *values);

/**
 * @param con
 * @param car
 * @return
*/
int executeCanMsg(device_t device, car_t *car);

/**
 * 
*/
void checkConnection(device_t device);

/**
 * 
*/
void print_info(device_t device);

#endif //REMOTE_CAR_BLUETOOTH_H