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

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (5 * CLOCK_SECOND)


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

static unsigned numHopsToCentralNode = 9999;
static linkaddr_t parentNodeAddress = {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};

static struct Payload {
  unsigned type; // type1 is the routing beacon, type2 is the sensor value
  unsigned value;
  unsigned roundCounter;
  unsigned origin;
} payload;



/*---------------------------------------------------------------------------*/

/* The input callback is called whenever the node received a message */
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  static unsigned currentRound = 0;
  static unsigned numMessagesRecievedInCurrentRound = 0;

  if(len == sizeof(struct Payload)) {
    struct Payload payloadReceived;
    memcpy(&payloadReceived, data, sizeof(struct Payload));

    if (payloadReceived.type == 0) {
        
        if (payloadReceived.value < numHopsToCentralNode) {
          LOG_INFO_LLADDR(src);
          LOG_INFO(" is %u hops away from central node. ", payloadReceived.value);
          numHopsToCentralNode = payloadReceived.value + 1;
          parentNodeAddress = *src;
          LOG_INFO("Take it as parent node! \n");
        } 
    }

    if (payloadReceived.type == 1) {
      if (node_id == 13) {
        if (payloadReceived.roundCounter > currentRound) {
          currentRound = payloadReceived.roundCounter;
          numMessagesRecievedInCurrentRound = 0;
          LOG_INFO("Reset numMessagesRecievedInCurrentRound\n");
        }
        numMessagesRecievedInCurrentRound++;
        LOG_INFO("Received sensor value %u from round %u\n", payloadReceived.value, payloadReceived.roundCounter);
        LOG_INFO("Thus received %u values in this round\n", numMessagesRecievedInCurrentRound);
      } else if (numHopsToCentralNode < 9999) {
        memcpy(nullnet_buf, &payloadReceived, sizeof(struct Payload));
        nullnet_len = sizeof(struct Payload);
        NETSTACK_NETWORK.output(&parentNodeAddress);
      }
      
    }     
  }
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned roundCounter = 0;


  PROCESS_BEGIN();
  
  if (node_id == 13) {
    numHopsToCentralNode = 0;
  }

  LOG_INFO("numHopsToCentralNode: %u \n", numHopsToCentralNode );

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload);
  nullnet_set_input_callback(input_callback);


  etimer_set(&periodic_timer, SEND_INTERVAL);

  while (1) {
    // Send beacon
    if (numHopsToCentralNode < 9999) {
      payload.type = 0;
      payload.value = numHopsToCentralNode;

      memcpy(nullnet_buf, &payload, sizeof(struct Payload));
      nullnet_len = sizeof(struct Payload);

      LOG_INFO("Broadcast beacon %u \n", numHopsToCentralNode);
      NETSTACK_NETWORK.output(NULL);
    }
    
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    unsigned sensorValue = random_rand() % 100;    

    if (numHopsToCentralNode < 9999 && node_id != 13) {
      payload.type = 1;
      payload.value = sensorValue;
      payload.roundCounter = roundCounter;
      payload.origin = node_id;

      memcpy(nullnet_buf, &payload, sizeof(struct Payload));
      nullnet_len = sizeof(struct Payload);

      NETSTACK_NETWORK.output(&parentNodeAddress);
      LOG_INFO("Sent sensor value %u in round %u to parent node ", sensorValue, payload.roundCounter);
      LOG_INFO_LLADDR(&parentNodeAddress);
      LOG_INFO("\n");
    }
    roundCounter++;
    
    etimer_reset(&periodic_timer);
  }

  

  PROCESS_END(); 
}
