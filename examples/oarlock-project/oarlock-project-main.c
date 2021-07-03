#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

#include <string.h>
#include <stdio.h> /* For printf() */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)
#define MEASUREMENT_BUFFER_LENGTH 20
#define DATA_POINTS_PER_TRANSMISSION 5
#define MEASUREMENT_INTERVAL RTIMER_SECOND / 50
#define RTIMER_MILISECOND RTIMER_SECOND / 1000
#define USE_IDEAL_STROKE 1
static linkaddr_t dest_addr =         {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};


/*------------------------ Fake Data Generator Functions --------------------*/

unsigned msPerStroke = 3000;

/*
Ideal stroke with constant acceleration. 20 Strokes/minute
1 second drive, 2 seconds free-wheeling

No business logic in here, just plain mathematics for faking sensor values.
*/
double idealStroke(unsigned long time, double offset) {
  double aDrive = 0.00018;
  double aFw = -0.000045;
  double offFw = 90;

  unsigned timeSinceStrokeStart = time % msPerStroke;
  if (timeSinceStrokeStart <= 1000) {
    return timeSinceStrokeStart * 0.5 * aDrive * timeSinceStrokeStart + offset; // Do not reduce to timeSinceStrokeStart**2 -> Overflow!
  } else {
    return (timeSinceStrokeStart - 1000) * 0.5 * aFw * (timeSinceStrokeStart - 1000) + offFw + offset; // Do not reduce. Be aware of overflow
  }
}

int int_pow(int base, int exp)
{
    int result = 1;
    while (exp)
    {
        if (exp % 2)
           result *= base;
        exp /= 2;
        base *= base;
    }
    return result;
}

/*
Ideal realistic stroke with plateau. 20 Strokes/minute
1 second drive, 2 seconds free-wheeling
*/
double slowRealisticStroke(unsigned long time, double offset) {
  long double aDrive = -0.00000000000000000000000004582541;

  double kDrive =  0.000179;

  double aFw = 0.0000000225;
  double bFw = -0.0000675;
  // cFw = 0;
  double dFw = 90;

  unsigned timeSinceStrokeStart = time % msPerStroke;
  if (timeSinceStrokeStart <= 1000) {
    return (aDrive / 90) * int_pow(timeSinceStrokeStart - 500, 10) + 0.5 * kDrive * int_pow(timeSinceStrokeStart, 2) + offset;
  } else {
    unsigned long timeSinceFreeWheeling = timeSinceStrokeStart - 1000;
    double firstSummand = timeSinceFreeWheeling * aFw * timeSinceFreeWheeling * timeSinceFreeWheeling;
    return firstSummand + bFw * int_pow(timeSinceFreeWheeling, 2) + dFw + offset;
  }
}


/*-------------------------- Process Initialization -------------------------*/
PROCESS(nullnet_example_process, "NullNet unicast example");
AUTOSTART_PROCESSES(&nullnet_example_process);





/*--------------------------- Data Repository -------------------------------*/

struct DataPoint {
  unsigned long timestamp;
  double value;
};

static struct Payload {
  unsigned timestamp;
  struct DataPoint measurements[DATA_POINTS_PER_TRANSMISSION];
} payload;

/* The measurementBuffer stores the values recorded from the sensor until they are sent to the base station. */
static struct DataPoint measurementBuffer[MEASUREMENT_BUFFER_LENGTH];
static unsigned measurementBufferCounter = 0;

/*
Call this function when you recorded a value from the sensor. 
It will store the value with its timestamp in the measurementBuffer.
*/
void pushToMeasurementBuffer(unsigned long timestamp, double measurement)
{
  struct DataPoint newDataPoint = {timestamp, measurement};
  measurementBuffer[measurementBufferCounter] = newDataPoint;
  LOG_INFO("Saved data point %u / %d into buffer at index %u\n", (unsigned) timestamp, (int) measurement, measurementBufferCounter);
  measurementBufferCounter++; // If this leads to an index out of bound, then you have made a mistake elsewhere because the buffer should be emptied before reaching its full size.
}

void readMeasurementBufferIntoPayload()
{
  unsigned transmissionCounter = 0;
  int measurementCounter = measurementBufferCounter - DATA_POINTS_PER_TRANSMISSION;
  if (measurementCounter < 0) {
    LOG_ERR("Whopsi, there are not enough data points recorded to make a full transmission!\n");
    return;
  }

  do {
    payload.measurements[transmissionCounter] = measurementBuffer[measurementCounter];
    transmissionCounter++;
    measurementCounter++;
  } while(transmissionCounter < DATA_POINTS_PER_TRANSMISSION);
}


/*------------------------ Data Evaluation Functions -----------------------*/

static unsigned long lastTimestamp = 0;
static double lastAngle = -999;
static double lastVelocity = -999;

double calcAccelerationFromAngle(unsigned long newTimestamp, double newAngle) {
  // If this is the first measurement, set this as the las_angle and return invalid
  if (lastAngle == -999) {
    lastAngle = newAngle;
    lastTimestamp = newTimestamp;
    return -999; // Last velocity is still invalid at this point in time
  }

  // There was a previous angle. So calculate the velocity using the differential quotient 
  double angleDifference = newAngle - lastAngle;
  double timeDifference = (newTimestamp - lastTimestamp) / 1000.0; // Divide by 1000 to use normal seconds when differentiating by time;
  double velocity = angleDifference / timeDifference;
  lastAngle = newAngle;
  lastTimestamp = newTimestamp;

  // If there is no previously calculated velocity, then set it and return invalid
  if (lastVelocity == -999) {
    lastVelocity = velocity;
    return -999;
  }

  // There was a previous velocity. So use it to calculate the acceleration
  double velocityDifference = velocity - lastVelocity;
  double acceleration = velocityDifference / timeDifference;
  lastVelocity = velocity;
  return acceleration;
}


/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  if(len == sizeof(struct Payload)) {
    // Copy payload from buffer in dedicated variable here
    struct Payload request;
    memcpy(&request, data, sizeof(struct Payload));

    LOG_INFO("Received values from ");
    LOG_INFO_LLADDR(src);
    LOG_INFO("\n");
  
    return;
  }
}
/*---------------------------------------------------------------------------*/

static struct rtimer real_timer;

void sendMeasurementsToBaseStation() {
  readMeasurementBufferIntoPayload();  

  // Copy data into transmission buffer
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload);
  
  // Transmit
  LOG_INFO("Send to base station\n");
  NETSTACK_NETWORK.output(&dest_addr);

  // Reset the buffer
  measurementBufferCounter = 0;
}

void readSensorValueAndPersist() {
  // First, reset the timer for the next measurement
  rtimer_clock_t time_now = RTIMER_NOW();
  rtimer_set(&real_timer, time_now + MEASUREMENT_INTERVAL, 1, readSensorValueAndPersist, NULL);

  // We cannot use the rtimer module for more exact timestamps, because it overflows after approximately 2 seconds
  unsigned long currentMiliseconds = (clock_time() * 1000) / CLOCK_SECOND;

  // Mock reading data from the sensor
  double currentAngle;
  if (USE_IDEAL_STROKE) {
    currentAngle = idealStroke(currentMiliseconds, 0);
  } else {
    currentAngle = slowRealisticStroke(currentMiliseconds, 0);
  }
  // LOG_INFO("Current angle is (int) %d\n", (int) currentAngle);

  // Translate angle into acceleration
  double currentAcceleration = calcAccelerationFromAngle(currentMiliseconds, currentAngle);
  // LOG_INFO("Acceleration is (int) %d\n", (int) currentAcceleration);
  if (currentAcceleration < 0) {
    return;
  }

  // Persist measurement if buffer
  pushToMeasurementBuffer(currentMiliseconds, currentAcceleration);  

  // If buffer holds enough data points, send them to the base station
  if (measurementBufferCounter >= DATA_POINTS_PER_TRANSMISSION) {
    sendMeasurementsToBaseStation();
  }
}



PROCESS_THREAD(nullnet_example_process, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("MEASUREMENT_INTERVAL: %u ticks\n", MEASUREMENT_INTERVAL);
  LOG_INFO("DATA_POINTS_PER_TRANSMISSION: %u ticks\n", DATA_POINTS_PER_TRANSMISSION);

  /* Initialize NullNet */
  nullnet_set_input_callback(input_callback);

  if (!linkaddr_cmp(&dest_addr, &linkaddr_node_addr)) {
    // This call is basically the beginning of a loop because it sets an rtimer for itself
    readSensorValueAndPersist();
  }
  

  while(1) {
    PROCESS_YIELD();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
