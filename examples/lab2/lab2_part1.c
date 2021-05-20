/*
* This file is based on nullnet-broadcast.c
*/

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */

/* New Includes */
#include "sys/node-id.h"
#include <random.h>
#include "dev/leds.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

/*
We use this struct to broadcast messages.
The ledID holds either 0, 1, or 2. Thus, we identify the three LEDs.
The counter identifies in which "transmission round" the message was sent.
*/
static struct Payload {
  unsigned ledId;
  unsigned counter;
} payload;

/*---------------------------------------------------------------------------*/

/* The input callback is called whenever the node received a message */
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  // Indicates which messages to ignore and which to handle
  static unsigned nextExpectedCounter = 0;

  // Assure that the message has a valid payload
  if (len == sizeof(struct Payload)) {
    struct Payload payload;
    memcpy(&payload, data, sizeof(struct Payload));

    // Handle message only if this node receives this message for the first time
    if (payload.counter >= nextExpectedCounter) {
      nextExpectedCounter = payload.counter + 1;

      // Forward message
      memcpy(nullnet_buf, &payload, sizeof(payload));
      nullnet_len = sizeof(payload);
      NETSTACK_NETWORK.output(NULL);

      // Toggle LED
      if (payload.ledId == 0) {
        leds_toggle(LEDS_RED);
      } else if (payload.ledId == 1) {
        leds_toggle(LEDS_GREEN);
      } else if (payload.ledId == 2) {
        leds_toggle(LEDS_YELLOW);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned counter = 0;
  payload.counter = 0;

  PROCESS_BEGIN();

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload);
  nullnet_set_input_callback(input_callback);

  // Node 13 is the central node, it is the only sender
  if (node_id==13) {  

    // Start periodic timer
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {      
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

      // Initialize the payload to be broadcasted
      payload.ledId = (unsigned)(random_rand() % 3);
      payload.counter = counter;


      LOG_INFO("Broadcast ledId %u, counter %u\n ", payload.ledId, payload.counter);
      
      // Copy payload into buffer and send
      memcpy(nullnet_buf, &payload, sizeof(payload));
      nullnet_len = sizeof(payload);
      NETSTACK_NETWORK.output(NULL);


      // Increment counter and restart the timer
      counter++;
      etimer_reset(&periodic_timer);
    }    
  }

  PROCESS_END(); 
}
