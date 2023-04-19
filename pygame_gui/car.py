from dataclasses import dataclass
from enum import Enum


class ObserverCode(Enum):
    OK = 0
    BAD_VALUE = 1
    BAD_TIMING = 2
    BAD_CAN_ID = 3


class GatewayCode(Enum):
    OK = 0
    R_BLOCKED = 1
    W_BLOCKED = 2


@dataclass
class Car:
    # Things the driver can operate / Chassis
    accelerator_value: int = 0
    brake_value: int = 0
    steering_value: int = 360
    light_switch_value: int = 0
    light_flash_value: int = 0
    parking_value: int = 0
    wiper_f_sw_value: int = 0
    wiper_r_sw_value: int = 0
    door_lock_value: int = 0
    l_door_handle_value: int = 0
    r_door_handle_value: int = 0
    l_window_switch_value: int = 0
    r_window_switch_value: int = 0
    turn_switch_value: int = 0
    hazard_value: int = 0
    horn_value: int = 0
    engine_value: int = 0

    # Things the driver can not manually change
    engine_rpm: int = 0
    speed_kph: int = 0
    shift_value: str = 0
    gear: int = 0
    brake_output: int = 0
    power_steering: int = 0
    shift_position: int = 0
    turn_signal_indicator: int = 0
    horn_operation: int = 0
    engine_malfunction: int = 0
    engine_status: int = 0
    parking_brake_status: int = 0
    light_status: int = 0
    front_wiper_status: int = 0
    front_wiper_timer: int = 0
    rear_wiper_status: int = 0
    door_lock_status: int = 0
    l_window_position: int = 0
    r_window_position: int = 0
    l_door_position: int = 0
    r_door_position: int = 0
    door_open_indicator: int = 0
    door_lock_indicator: int = 0
    observer_id: int = 0
    observer_code: ObserverCode = ObserverCode.OK
    gateway_id: int = 0
    gateway_code: GatewayCode = GatewayCode.OK
