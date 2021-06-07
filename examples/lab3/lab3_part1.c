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
#define AP_SEND_INTERVAL (5 * CLOCK_SECOND)
#define MOBILE_SEND_INTERVAL CLOCK_SECOND
#define MOBILE_NODE_ID 1

#define PAYLOAD_TYPE_BEACON 0
#define PAYLOAD_TYPE_CONNECTION 1
#define PAYLOAD_TYPE_VALUE 2
#define PAYLOAD_TYPE_DISCONNECTION 3


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);



/*
We use the same struct for all messages because the distinction between structs at the receiver side did not work out 
(we tried to identify different structs by the sizeof method).

The type is:
- 0 for beacons
- 1 for connection requests and their acknowledgements
- 2 for value transmission
- 3 for disconnection requests and their acknowledgements
 */
static struct Payload { 
  char type;
  unsigned value; // Any value to send
  unsigned ap; // The access point id involved in the communication
} payload;



/*---------------------------------------------------------------------------*/

static linkaddr_t uplinkNodeAddress = {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
static unsigned uplinkNode = 0;
static char isDownLinkConnected = 0;

/* The input callback is called whenever the node received a message */
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{


  if (len != sizeof(struct Payload)) {
    return;
  }

  struct Payload receivedPayload;
  memcpy(&receivedPayload, data, sizeof(struct Payload));

  LOG_INFO("Received message of type %u with value %u\n", receivedPayload.type, receivedPayload.value);























  /* Here begins the code for the MOBILE NODE */

  if (node_id == MOBILE_NODE_ID) {


    // Case: Mobile node received a beacon
    if (receivedPayload.type == PAYLOAD_TYPE_BEACON) {  
      LOG_INFO("Message is Beacon from %u with number %u while uplink is %u\n", receivedPayload.ap, receivedPayload.value, uplinkNode);

      linkaddr_t apInQuestion = *src;

      if (uplinkNode == 0) {
        LOG_INFO("Send connection request to access point with address ");
        LOG_INFO_LLADDR(&apInQuestion);
        LOG_INFO("\n");
        payload.type = PAYLOAD_TYPE_CONNECTION;
        payload.value = 1;
        nullnet_buf = (uint8_t *)&payload;
        nullnet_len = sizeof(payload);


        NETSTACK_NETWORK.output(&linkaddr_null);
      } 
      return;
    } 




















    // Case: Mobile node received connection acknowledgement
    if (receivedPayload.type == PAYLOAD_TYPE_CONNECTION) {
      if (receivedPayload.value == 0) {
        LOG_ERR("Access point denied connection!\n");
        return;
      } else {
        LOG_INFO("Access point accepted connection. Uplink node is now %u with address ", receivedPayload.ap);
        LOG_INFO_LLADDR(src);
        LOG_INFO("\n. I am ");
        LOG_INFO_LLADDR(dest);
        LOG_INFO("\n");
      }

      uplinkNodeAddress = *src;
      uplinkNode = receivedPayload.ap;
      leds_on(LEDS_RED);
      return;
    }

    return;
  }



  /* Here begins the code for the ACCESS POINT NODES */

  // // Access points do not allow broadcast messages so just return early here
  // if (!linkaddr_cmp(dest, &linkaddr_node_addr)) {
  //   LOG_INFO("Dropped packet: dest was ");
  //   LOG_INFO_LLADDR(dest);
  //   LOG_INFO(" and I am ");
  //   LOG_INFO_LLADDR(&linkaddr_node_addr);
  //   LOG_INFO(". Source is ");
  //   LOG_INFO_LLADDR(src);
  //   LOG_INFO("\n");
  //   return;
  // }

  if (receivedPayload.type == PAYLOAD_TYPE_CONNECTION) {
    LOG_INFO("Mobile node wants to connect to ");
    LOG_INFO_LLADDR(dest);
    LOG_INFO("\n");
    leds_on(LEDS_RED);

    payload.type = PAYLOAD_TYPE_CONNECTION;
    payload.value = 1;
    payload.ap = node_id;
    nullnet_buf = (uint8_t *)&payload;
    nullnet_len = sizeof(payload);

    NETSTACK_NETWORK.output(src);
    isDownLinkConnected = 1;
    LOG_INFO("Sent acknowledgment\n");
    return;
  }

  if (receivedPayload.type == PAYLOAD_TYPE_VALUE) {
    if (isDownLinkConnected == 0) {
      LOG_INFO("Dropped value message, because the node is not connected\n");
      return;
    }

    LOG_INFO("Logged value %u from mobile node\n", receivedPayload.value);
  }
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();  

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload);
  nullnet_set_input_callback(input_callback);

  if (node_id == MOBILE_NODE_ID) {
    LOG_INFO("I am the mobile node!\n");

    static unsigned counter = 0;
    etimer_set(&periodic_timer, MOBILE_SEND_INTERVAL);

    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      payload.type = PAYLOAD_TYPE_VALUE;
      payload.value= counter;
      payload.ap = uplinkNode;
      LOG_INFO("Send value %u to uplink node %u\n", counter, uplinkNode);
      NETSTACK_NETWORK.output(&uplinkNodeAddress);
      counter++;
      etimer_reset(&periodic_timer);
    }    
  } 


  /* Here begins the code for the ACCESS POINT NODES */


  LOG_INFO("I am an access-point node with id %u \n", node_id);

  static unsigned beaconCounter = 0;

  etimer_set(&periodic_timer, AP_SEND_INTERVAL);
  while (1) {   
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    payload.type = PAYLOAD_TYPE_BEACON;
    payload.value = beaconCounter;
    payload.ap = node_id;
    nullnet_buf = (uint8_t *)&payload;
    NETSTACK_NETWORK.output(NULL);

    LOG_INFO("Broadcasted beacon %u\n", beaconCounter);
    beaconCounter++; 
    etimer_reset(&periodic_timer);
  }
  PROCESS_END(); 
}
