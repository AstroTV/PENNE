from car import Car


def simulate(car: Car, elements: {}):
    # Brake: We add 20% for every tick that the brake pedal is pressed and remove 20% for every tick it is not pressed
    if elements["brake_pedal"].pressed and car.brake_value < 100:
        car.brake_value += 20
    elif not elements["brake_pedal"].pressed and car.brake_value > 0:
        car.brake_value -= 20
    car.brake_value = min(100, car.brake_value)
    car.brake_value = max(0, car.brake_value)
    elements["brake_pedal"].image.set_alpha(255 - car.brake_value * 2)

    # Gas: We add 20% for every tick that the brake pedal is pressed and remove 20% for every tick it is not pressed
    if elements["gas_pedal"].pressed and car.accelerator_value < 100:
        car.accelerator_value += 20
    elif not elements["gas_pedal"].pressed and car.accelerator_value > 0:
        car.accelerator_value -= 20
    car.accelerator_value = min(100, car.accelerator_value)
    car.accelerator_value = max(0, car.accelerator_value)
    elements["gas_pedal"].image.set_alpha(255 - car.accelerator_value * 2)

    car.shift_value = elements["gear_switch"].position
    car.steering_value = elements["steering_wheel"].physical_angle
    car.engine_value = elements["engine"].status
    car.turn_switch_value = elements["blinker_switch"].state
    car.hazard_value = elements["hazard_button"].state
    car.horn_value = elements["horn"].pressed
    car.light_switch_value = elements["light_switch"].state
    car.parking_value = elements["park_brake"].pulled
    car.wiper_f_sw_value = int(elements["wiper_switch"].state == 2)
    car.wiper_r_sw_value = int(elements["wiper_switch"].state == 0)
    car.door_lock_value = elements["lock"].lock_button_state
    car.l_window_switch_value = elements["left_door"].window_button_state
    car.r_window_switch_value = elements["right_door"].window_button_state
    car.l_door_handle_value = elements["left_door"].door_button_state
    car.r_door_handle_value = elements["right_door"].door_button_state
