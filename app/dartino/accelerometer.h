#ifndef APP_DARTINO_ACCELEROMETER_H_
#define APP_DARTINO_ACCELEROMETER_H_

#include <sys/types.h>

// One-shot. Causes the accelerometer to publish data to this port. 
int accelerometer_request_data(void);

// Setup the accelerometer and any accompanying worker threads.
void accelerometer_init(void); 


#endif  // APP_DARTINO_ACCELEROMETER_H_