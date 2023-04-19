# Powertrain
# 100Hz
BRAKE_OUTPUT_IND_MSG = 0x24
ENGINE_RPM_MSG = 0x43
POWER_STEERING_OUT_IND_MSG = 0x62
SHIFT_POSITION_MSG = 0x77
# 20Hz
ENGINE_STATUS_MSG = 0x19a
PARKING_BRAKE_STATUS_MSG = 0x1d3
# 2Hz

# Chassis
# 100Hz
BRAKE_OPERATION_MSG = 0x1a
ACCELERATION_OPERATION_MSG = 0x2f
STEERING_WHEEL_POS_MSG = 0x58
SHIFT_POSITION_SWITCH_MSG = 0x6d
ENGINE_START_MSG = 0x1b8
TURN_SWITCH_MSG = 0x83
HORN_SWITCH_MSG = 0x98
# 20Hz
LIGHT_SWITCH_MSG = 0x1a7
LIGHT_FLASH_MSG = 0x1b1
PARKING_BRAKE_MSG = 0x1c9
# 10Hz
WIPER_SWITCH_FRONT_MSG = 0x25c
WIPER_SWITCH_REAR_MSG = 0x271
DOOR_LOCK_UNLOCK_MSG = 0x286
L_WINDOW_SWITCH_MSG = 0x29c
R_WINDOW_SWITCH_MSG = 0x2b1
L_DOOR_HANDLE_MSG = 0x29d
R_DOOR_HANDLE_MSG = 0x2b2

# Body
# 100Hz
TURN_SIGNAL_INDICATOR_MSG = 0x8d
HORN_OPERATION_MSG = 0xa2
# 20Hz
LIGHT_INDICATOR_MSG = 0x1bb
# 10Hz
FRONT_WIPER_STATUS_MSG = 0x266
REAR_WIPER_STATUS_MSG = 0x27b
DOOR_LOCK_STATUS_MSG = 0x290
L_WINDOW_POSITION_MSG = 0x2bb
R_WINDOW_POSITION_MSG = 0x2a6
# 2Hz
L_DOOR_POSITION_MSG = 0x2bc
R_DOOR_POSITION_MSG = 0x2a7

can_messages_dict = {var_value: var_name for var_name, var_value in locals().items() if isinstance(var_value, int)}


def get_message_from_id(can_id: int) -> str:
    if can_id in can_messages_dict:
        return can_messages_dict[can_id]
    else:
        return "UNKNOWN ID"