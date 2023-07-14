#include "../include/helper.h"

int init_can(int *s)
{
    /* Open CAN socket and listen to it */
	struct sockaddr_can addr;
	struct ifreq ifr;
    int enable_canfd = 1;

	if ((*s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Socket");
		return -1;
	}

	strcpy(ifr.ifr_name, "vcan0" );
	ioctl(*s, SIOCGIFINDEX, &ifr);

	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

    if(ioctl(*s, SIOCGIFMTU, &ifr) < 0)
    {
        perror("SIOCGIFMTU");
        return FAILURE;
    }

    /* enable CAN_FD */
    if(setsockopt(*s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd)))
    {
        perror("CAN FD support");
        return FAILURE;
    }

    /* setup filter */
    struct can_filter rfilter[NUMBER_CAN_IDS];

    rfilter[0].can_id = HORN_OPERATION_MSG;
    rfilter[0].can_mask = CAN_EFF_MASK;
    rfilter[1].can_id = ENGINE_RPM_MSG;
    rfilter[1].can_mask = CAN_EFF_MASK;
    rfilter[2].can_id = POWER_STEERING_OUT_IND_MSG;
    rfilter[2].can_mask = CAN_EFF_MASK;
    rfilter[3].can_id = LIGHT_INDICATOR_MSG;
    rfilter[3].can_mask = CAN_EFF_MASK;
    rfilter[4].can_id = SHIFT_POSITION_MSG;
    rfilter[4].can_mask = CAN_EFF_MASK;
    rfilter[5].can_id = DOOR_LOCK_UNLOCK_MSG;
    rfilter[5].can_mask = CAN_EFF_MASK;
    rfilter[6].can_id = BRAKE_OUTPUT_IND_MSG;
    rfilter[6].can_mask = CAN_EFF_MASK;
    rfilter[7].can_id = TURN_SIGNAL_INDICATOR_MSG;
    rfilter[7].can_mask = CAN_EFF_MASK;
    rfilter[8].can_id = ENGINE_START_MSG;
    rfilter[8].can_mask = CAN_EFF_MASK;

    setsockopt(*s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

	if(bind(*s, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
    {
		perror("Bind socket");
		return FAILURE;
	}

    return SUCCESS;
}

void read_can(struct canfd_frame frame, car_t *car)
{
    /* Horn */
    if(frame.can_id == HORN_OPERATION_MSG && car->tmp.horn != frame.data[0])
    {
        car->tmp.horn = frame.data[0];
        car->changed = HORN;
    }
    /* Speed */
    else if(frame.can_id == ENGINE_RPM_MSG && car->tmp.speed != frame.data[3])
    {
        car->speed = (frame.data[3] * 250 / 255) * MULTI_SPEED;
        car->tmp.speed = frame.data[3];
        car->changed = MOVE;
    }
    /* Steering */
    else if(frame.can_id == POWER_STEERING_OUT_IND_MSG && car->tmp.steering != frame.data[1])
    {
        car->steering_angle = frame.data[1];
        car->tmp.steering = frame.data[1];
        car->changed = MOVE;
    }
    /* Light */
    else if(frame.can_id == LIGHT_INDICATOR_MSG && car->tmp.light != frame.data[0])
    {
        car->tmp.light = frame.data[0];
        car->changed = LIGHT;
    }
    /* Gear */
    else if(frame.can_id == SHIFT_POSITION_MSG && car->tmp.gear != frame.data[0])
    {
        car->tmp.gear = frame.data[0];
        car->changed = GEAR;
    }
    /* Turn switch */
    else if(frame.can_id == TURN_SIGNAL_INDICATOR_MSG && car->tmp.turn != frame.data[0])
    {
        car->tmp.turn = frame.data[0];
        car->changed = TURN;
    }
    /* Brake */
    else if(frame.can_id == BRAKE_OUTPUT_IND_MSG && car->tmp.brake != frame.data[0])
    {
        car->tmp.brake = frame.data[0];
        car->changed = BRAKE;
    }
    /* Lock */
    else if(frame.can_id == DOOR_LOCK_UNLOCK_MSG && car->tmp.lock != frame.data[0] && frame.data[0] != 0x00)
    {
        car->tmp.lock = frame.data[0];
        car->changed = LOCK;
    }
    /* Engine */
    else if(frame.can_id == ENGINE_START_MSG && car->tmp.engine != frame.data[0])
    {
        car->tmp.engine = frame.data[0];
    }
}