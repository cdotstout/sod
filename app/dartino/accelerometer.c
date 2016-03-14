#include "accelerometer.h"

#include <dev/accelerometer.h>
#include <err.h>
#include <kernel/event.h>
#include <kernel/port.h>
#include <kernel/thread.h>

// Smaller stacks for sensor publishers since they don't do much.
#define SENSOR_PUBLISHER_DEFAULT_STACK_SIZE (512)

// Accelerometer publisher will write data to this port when data is requested.
#define ACCELEROMETER_PORT_NAME ("sys/io/acc")

static event_t accelerometer_event = 
	EVENT_INITIAL_VALUE(accelerometer_event, false, EVENT_FLAG_AUTOUNSIGNAL);

// Worker thread that sleeps until data is requested from the accelerometer. 
// Takes a single reading and publishes it to the accelerometer port when woken.
thread_t *accelerometer_worker;

static int accelerometer_publisher_worker_thread(void *argv)
{
	position_vector_t pos_vector;
	status_t result;
	port_t acc_port;

	// Make sure that position vector elements fit exactly into port packets.
	STATIC_ASSERT(sizeof(pos_vector.x) == sizeof(port_packet_t));
	STATIC_ASSERT(sizeof(pos_vector.y) == sizeof(port_packet_t));
	STATIC_ASSERT(sizeof(pos_vector.z) == sizeof(port_packet_t));
	STATIC_ASSERT(sizeof(pos_vector) == 3 * sizeof(port_packet_t));

	result = port_create(ACCELEROMETER_PORT_NAME, PORT_MODE_BROADCAST, &acc_port);
    if (result != NO_ERROR) {
    	printf("port_open failed for port = \"%s\", status = %d\n", 
    		   ACCELEROMETER_PORT_NAME, result);
    	return result;
    }

	while(true) {
		result = event_wait(&accelerometer_event);
		if (result != NO_ERROR) {
			continue;
		}

		result = acc_read_xyz(&pos_vector);
		if (result != NO_ERROR) {
			// Signal to Dart that we were unable to read the accelerometer.
			uint64_t success = ERR_IO;
			result |= port_write(acc_port, (port_packet_t *)(&success), 1);
			continue;
		}

		uint64_t success = 0;
		result = NO_ERROR;
		result |= port_write(acc_port, (port_packet_t *)(&success), 1);
		result |= port_write(acc_port, (port_packet_t *)(&pos_vector), 3);

		if (result != NO_ERROR) {
			printf("error writing accelerometer data to port.\n");
		}
	}

	return NO_ERROR;
}

void accelerometer_init(void) 
{
	// TODO(gkalsi): Check if an accelerometer exists and bail if it doesn't.

	accelerometer_worker = thread_create(
		"accelerometer worker", 
		&accelerometer_publisher_worker_thread, 
		NULL, 
		DEFAULT_PRIORITY, 
		SENSOR_PUBLISHER_DEFAULT_STACK_SIZE
	);

	thread_resume(accelerometer_worker);
}

status_t accelerometer_request_data(void)
{
	status_t result = event_signal(&accelerometer_event, true);
	return result;
}
