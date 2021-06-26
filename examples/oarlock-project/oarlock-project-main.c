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
static linkaddr_t dest_addr =         {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};


/*------------------------ Fake Data Generator Functions --------------------*/

unsigned msPerStroke = 3000;

/*
Ideal stroke with constant acceleration. 20 Strokes/minute
1 second drive, 2 seconds free-wheeling

No business logic in here, just plain mathematics for faking sensor values.
*/
double idealStroke(unsigned time, double offset) {
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


/*-------------------------- Process Initialization -------------------------*/
PROCESS(nullnet_example_process, "NullNet unicast example");
AUTOSTART_PROCESSES(&nullnet_example_process);





/*--------------------------- Data Repository -------------------------------*/

struct DataPoint {
  unsigned timestamp;
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
void pushToMeasurementBuffer
(timestamp, measurement)
{
  struct DataPoint newDataPoint = {timestamp, measurement};
  measurementBuffer[measurementBufferCounter] = newDataPoint;
  measurementBufferCounter++; // If this leads to an index out of bound, then you have made a mistake elsewhere because the buffer should be emptied before reaching its full size.
}

void readMeasurementBufferIntoPayload()
{
  unsigned transmissionCounter = 0;
  int measurementCounter = measurementBufferCounter - DATA_POINTS_PER_TRANSMISSION;
  if (measurementCounter < 0) {
    LOG_ERR("Whopsi, there are not enough data points recorded to make a full transmission!\n");
  }

  do {
    payload.measurements[transmissionCounter] = measurementBuffer[measurementCounter];
    transmissionCounter++;
  } while(transmissionCounter < DATA_POINTS_PER_TRANSMISSION);
}




/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  LOG_INFO("Received something\n");
  if(len == sizeof(unsigned)) {
    unsigned count;
    memcpy(&count, data, sizeof(count));
    LOG_INFO("Received %u from ", count);
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n Dest was ");
    LOG_INFO_LLADDR(dest);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count = 0;

  PROCESS_BEGIN();

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&count;
  nullnet_len = sizeof(count);
  nullnet_set_input_callback(input_callback);

  if(!linkaddr_cmp(&dest_addr, &linkaddr_node_addr)) {
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      LOG_INFO("Sending %u to ", count);
      LOG_INFO_LLADDR(&dest_addr);
      LOG_INFO_("\n");

      unsigned timestamp = count * 20;
      double measurementValue = idealStroke(timestamp, 0);
      LOG_INFO("Ideal stroke would be at %d\n", (int) (10 * measurementValue));
      pushToMeasurementBuffer(timestamp, measurementValue);

      NETSTACK_NETWORK.output(&dest_addr);
      count++;
      etimer_reset(&periodic_timer);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
