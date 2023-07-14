#include "../include/bluetooth.h"
#include "../include/helper.h"

#include <pthread.h>

char tmp_name[10];
char tmp_object_path[40];

/*------------------------------------------------------------------------------------------------*/
int init_bluetooth(device_t *device, char *name)
{
    GError *err = NULL;
    int rc = 0;

    strcpy(tmp_name, name);

    /* Connect to system bus */
    device->con = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
    if(device->con == NULL || err != NULL)
    {
        g_print("Not able to get connection to system bus\n");
        g_object_unref(device->con);
        return FAILURE;
    }

    device->loop = g_main_loop_new(NULL, FALSE);

    /* Get object_path of device */
    rc = get_object_path(*device);
    
    g_main_loop_run(device->loop);

    if(rc == FAILURE)
    {
        g_print("Not able to get objectpath by name\n");
        return FAILURE;
    }
    else
    {
        strcpy(device->name, tmp_name);
        strcpy(device->object_path, tmp_object_path);
        strcpy(device->write_path, device->object_path);
        strcat(device->write_path, DEVICE_WRITE);
    }

    /* Connect to BLE device */
    rc = connect_to_BLE(*device);
    if(rc == FAILURE)
    {
        g_print("Not able to connect to BLE device\n");
        return FAILURE;
    }
    else
    {
        g_print("Device connected\n");
    }

    return SUCCESS;
}

/*------------------------------------------------------------------------------------------------*/
int get_object_path(device_t device)
{
    g_dbus_connection_call( 
        device.con,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        NULL,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        (GAsyncReadyCallback)get_devices,
        device.loop
    );

    return SUCCESS;
}

/*------------------------------------------------------------------------------------------------*/
void get_devices(GDBusConnection *con, GAsyncResult *res, gpointer data)
{
    GVariant *result = NULL;
    GMainLoop *loop = NULL;
    GVariantIter i;
    const gchar *object_path;
    GVariant *ifaces_and_properties;
    int rc = FAILURE;

    loop = (GMainLoop *)data;
    result = g_dbus_connection_call_finish(con, res, NULL);

    if(result == NULL) return;

    if(result)
    {
        result = g_variant_get_child_value(result, 0);
        g_variant_iter_init(&i, result);
        while(g_variant_iter_next(&i, "{&o@a{sa{sv}}}", &object_path, &ifaces_and_properties))
        {
            const gchar *device_name;
            GVariant *properties;
            GVariantIter ii;
            g_variant_iter_init(&ii, ifaces_and_properties);
            while(g_variant_iter_next(&ii, "{&s@a{sv}}", &device_name, &properties))
            {
                if(g_strstr_len(g_ascii_strdown(device_name, -1), -1, "device"))
                {
                    rc = get_name(con, loop, object_path);
                    if(rc == SUCCESS) return;
                }
            }
        }
    }

    return;
}

/*------------------------------------------------------------------------------------------------*/
int get_name(GDBusConnection *con, GMainLoop *loop, const gchar *path)
{
    GVariant *result;
    GVariant *prop;
    GError *error = NULL;

    result = g_dbus_connection_call_sync(   
        con,
        "org.bluez",
        path,
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", "org.bluez.Device1", "Name"),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if(result == NULL) return FAILURE;

    g_variant_get(result, "(v)", &prop);
    g_variant_unref(result);

    if(g_strcmp0(g_variant_get_string(prop, NULL), tmp_name) == 0)
    {
        g_stpcpy(tmp_object_path, path);
        g_main_loop_quit(loop);
        return SUCCESS;
    }

    return FAILURE;
}

/*------------------------------------------------------------------------------------------------*/
int connect_to_BLE(device_t device)
{
    GError *err = NULL;

    g_dbus_connection_call_sync(
        device.con,
        BLUEZ_BUS_NAME,
        device.object_path,
        DEVICE_IFACE,
        "Connect",
        NULL,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        G_MAXINT,
        NULL,
        &err
    );

    if(err != NULL)
    {
        return FAILURE;
    }

    return SUCCESS;
}

/*------------------------------------------------------------------------------------------------*/
int disconnect_from_BLE(device_t device)
{
    GError *err = NULL;

    g_dbus_connection_call_sync(
        device.con,
        BLUEZ_BUS_NAME,
        device.object_path,
        DEVICE_IFACE,
        "Disconnect",
        NULL,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        G_MAXINT,
        NULL,
        &err
    );

    if(err != NULL)
    {
        return FAILURE;
    }

    return SUCCESS;
}

/*------------------------------------------------------------------------------------------------*/
int write_to_BLE(device_t device, char command, int *values)
{
    GVariantBuilder *builder1, *builder2;
    GVariant *value, *args, *dict;

    char arg[18] = { '\n' };

    if(command == COMMAND_HORN)
    {
        sprintf(arg, "%c#%d#", COMMAND_HORN, values[0]);
    }
    else if(command == COMMAND_DRIVE)
    {
        sprintf(arg, "%c#%d#%d#", COMMAND_DRIVE, values[0], values[1]);
    }
    else if(command == COMMAND_LIGHT)
    {
        sprintf(arg, "%c#%d#%d#", COMMAND_LIGHT, values[0], values[1]);
    }
    else if(command == COMMAND_ALIVE)
    {
        sprintf(arg, "%c#%d#", COMMAND_ALIVE, values[0]);
    }
    else if(command == COMMAND_LOCK)
    {
        sprintf(arg, "%c#%d#", COMMAND_LOCK, values[0]);
    }
    else if(command == COMMAND_GEAR)
    {
        sprintf(arg, "%c#%d#", COMMAND_GEAR, values[0]);
    }

    //g_print("Write to device: %s\n", arg);

    builder1 = g_variant_builder_new(G_VARIANT_TYPE("ay"));
    for(size_t i = 0; i < strlen(arg); i++)
    {
        g_variant_builder_add(builder1, "y", arg[i]);
    }
    g_variant_builder_add(builder1, "y", '\n');
    value = g_variant_builder_end(builder1);
    g_variant_builder_unref(builder1);

    builder2 = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(builder2, "{sv}", "param", g_variant_new_string("w"));
    dict = g_variant_builder_end(builder2);
    g_variant_builder_unref(builder2);

    GVariant *params[2] = { value, dict };
    args = g_variant_new_tuple(params, 2);

    if(args == NULL)
    {
        return FAILURE;
    }

    g_dbus_connection_call_sync(
        device.con,
        BLUEZ_BUS_NAME,
        device.write_path,
        GATT_IFACE,
        "WriteValue",
        args,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        G_MAXINT,
        NULL,
        NULL 
    );

    return SUCCESS;    
}

/*------------------------------------------------------------------------------------------------*/
int executeCanMsg(device_t device, car_t *car)
{
    int rc = 0, tmp_angle = 0;
    int parameters[2] = { 0 };
    switch (car->changed)
    {
    case MOVE: 
        tmp_angle = abs(MIDDLE_POS_STEERING - car->steering_angle) * MULTI_STEERING;
        
        if(car->steering_angle < MIDDLE_POS_STEERING)
        {
            parameters[0] = car->speed + tmp_angle;
            parameters[1] = car->speed - tmp_angle;
        }
        else if(car->steering_angle > MIDDLE_POS_STEERING)
        {
            parameters[0] = car->speed - tmp_angle;
            parameters[1] = car->speed + tmp_angle;
        }
        else
        {
            parameters[0] = car->speed;
            parameters[1] = car->speed;
        }
        if(car->speed > 0)
        {
            parameters[0] += OFFSET_SPEED;
            parameters[1] += OFFSET_SPEED;
        }
        else
        {
            parameters[0] = parameters[1] = 0;
        }

        if(parameters[0] < 0) parameters[0] = 0;
        if(parameters[0] >= 255) parameters[0] = 255;
        if(parameters[1] < 0) parameters[1] = 0;
        if(parameters[1] >= 255) parameters[1] = 255;

        if(car->gear == REVERSE)
        {
            parameters[0] *= -1;
            parameters[1] *= -1;
        }
        rc = write_to_BLE(device, COMMAND_DRIVE, parameters);
        if(rc == FAILURE)
        {
            printf("Not able to write to BLE\n");
            return FAILURE;
        }
        car->changed = NONE;
        break;
    case HORN:
        if(car->tmp.horn == 0x01)
        {
            parameters[0] = 2000;
        }
        else
        {
            parameters[0] = 0;
        }
        rc = write_to_BLE(device, COMMAND_HORN, parameters);
        if(rc == FAILURE)
        {
            printf("Not able to write to BLE\n");
            return FAILURE;
        }
        car->changed = NONE;
        break;
    case LIGHT:
        if(car->tmp.light == 0x01)
        {
            parameters[0] = RGB_MODE_LIGHT;
        }
        else if(car->tmp.light == 0x02)
        {
            parameters[0] = RGB_MODE_HIGH_BEAM;
        }
        else
        {
            parameters[0] = RGB_MODE_OFF;
        }
        rc = write_to_BLE(device, COMMAND_LIGHT, parameters);
        if(rc == FAILURE)
        {
            printf("Not able to write to BLE\n");
            return FAILURE;
        }
        car->changed = NONE;
        break;
    case GEAR:
        if(car->tmp.gear == GEAR_PARK)
        {
            car->gear = PARK;
        }
        else if(car->tmp.gear == GEAR_REVERSE)
        {
            car->gear = REVERSE;
        }
        else if(car->tmp.gear == GEAR_NEUTRAL)
        {
            car->gear = NEUTRAL;
        }
        else if(car->tmp.gear == GEAR_DRIVE)
        {
            car->gear = DRIVE;
        }
        parameters[0] = car->gear;
        rc = write_to_BLE(device, COMMAND_GEAR, parameters);
        if(rc == FAILURE)
        {
            printf("Not able to write to BLE\n");
            return FAILURE;
        }
        car->changed = NONE;
        break;
    case TURN:
        if(car->tmp.turn == TURN_OFF)
        {
            parameters[0] = RGB_MODE_TURN_SIGN_OFF;
        }
        else if(car->tmp.turn == TURN_LEFT)
        {
            parameters[0] = RGB_MODE_LEFT_TS;
        }
        else if(car->tmp.turn == TURN_RIGHT)
        {
            parameters[0] = RGB_MODE_RIGHT_TS;
        }
        else if(car->tmp.turn == WARNING)
        {
            parameters[0] = RGB_MODE_WARNING;
        }
        rc = write_to_BLE(device, COMMAND_LIGHT, parameters);
        if(rc == FAILURE)
        {
            printf("Not able to write to BLE\n");
            return FAILURE;
        }
        car->changed = NONE;
        break;
    case BRAKE:
        parameters[0] = RGB_MODE_BRAKE;
        if(car->tmp.brake > 0)
        {
            parameters[1] = 1;
        }
        else
        {
            parameters[1] = 0;
        }
        rc = write_to_BLE(device, COMMAND_LIGHT, parameters);
        if(rc == FAILURE)
        {
            printf("Not able to write to BLE\n");
            return FAILURE;
        }
        car->changed = NONE;
        break;
    case LOCK:
        if(car->tmp.lock == 0x01)
        {
            // lock
            parameters[0] = 1;
        }
        else if(car->tmp.lock == 0x02)
        {
            // unlock
            parameters[0] = 0;
        }
        rc = write_to_BLE(device, COMMAND_LOCK, parameters);
        if(rc == FAILURE)
        {
            printf("Not able to write to BLE\n");
            return FAILURE;
        }
        car->changed = NONE;
        break;
    default:
        break;
    }

    return SUCCESS;
}

/*------------------------------------------------------------------------------------------------*/
void checkConnection(device_t device)
{
    int params[1] = { 1 };
    write_to_BLE(device, COMMAND_ALIVE, params);
}

/*------------------------------------------------------------------------------------------------*/
void print_info(device_t device)
{
    g_print("---------------- Device info ----------------\n");
    g_print("Name: %s\n", device.name);
    g_print("Object path: %s\n", device.object_path);
    g_print("Write path: %s\n", device.write_path);
    g_print("---------------------------------------------\n");
}