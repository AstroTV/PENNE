#include <glib.h>
#include <gio/gio.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curses.h>

#include <linux/can/raw.h>

#include "../include/helper.h"
#include "../include/bluetooth.h"

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Usage: ./app <name>\n");
        return FAILURE;
    }

    tmp_t tmp = { 0 };
    struct timeval previous_time, current_time;
    previous_time.tv_sec = previous_time.tv_usec = 0;
    car_t car = { 0, 0x68, 0, 0, tmp };
    int rc = 0;

    device_t device;
    device.con = NULL;
    device.loop = NULL;

    /* Init Bluetooth */
    if(init_bluetooth(&device, argv[1]))
    {
        g_object_unref(device.con);
        return FAILURE;
    }
    printf("Bluetooth initialization done\n");

    /* Init CAN */
    int s; 

    if(init_can(&s))
    {
        return FAILURE;
    }
    printf("CAN initialization done\n");

    struct canfd_frame frame;
    ssize_t nbytes;

    printf("Press 'c' to exit...\n");
    sleep(2);
    int c;
    initscr();
    noecho();
    timeout(0);

    /* Read from CAN bus and execute Can messages */
    while(c != 'c')
    {
        nbytes = read(s, &frame, CANFD_MTU);

        if(nbytes < 0) {
            perror("Not able to read socket\n");
            return FAILURE;
        }

        read_can(frame, &car);

        rc = executeCanMsg(device, &car);
        if(rc == FAILURE)
        {
            g_object_unref(device.con);
            return FAILURE;
        }

        gettimeofday(&current_time, NULL);
        if (current_time.tv_sec - previous_time.tv_sec >= TIME_INTERVAL) 
        {
            checkConnection(device);
            gettimeofday(&previous_time, NULL);
        }

        c = getch();
    }

    /* Disconnect from BLE device */
    rc = disconnect_from_BLE(device);
    if(rc == FAILURE)
    {
        g_print("Not able to disconnect from BLE device\n");
        g_object_unref(device.con);
        return FAILURE;
    }
	
	if (close(s) < 0) {
		perror("Not able to close socket\n");
		return FAILURE;
	}

    echo();
    endwin();

    return SUCCESS;
}