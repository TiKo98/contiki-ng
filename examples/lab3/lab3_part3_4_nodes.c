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
#define AP_SEND_INTERVAL (2 * CLOCK_SECOND)
#define MOBILE_SEND_INTERVAL CLOCK_SECOND

#define PAYLOAD_TYPE_BEACON 0
#define PAYLOAD_TYPE_CONNECTION 1
#define PAYLOAD_TYPE_VALUE 2
#define PAYLOAD_TYPE_DISCONNECTION 3


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

unsigned amIMobile ()
{
  if (node_id == 6 || node_id == 7 || node_id == 8 || node_id == 9) {
    return 1;
  }
  return 0;
}



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
  unsigned mobileNodeId;
} payload;


/* 

Using the beaconHistory array we can store the five latest beacons we received. 
We store the id of the broadcasting access point and its link address.

The pushToBeaconHistory function works as a setter for this array.
For retrieving the access point which has currently the best connection to the mobile node 
(i.e. the one that the most items in the beaconHistory array are from)
we built the getApIdToConnectToFromBeaconHistory.

*/

struct BeaconHistoryItem {
  unsigned nodeID;
  linkaddr_t nodeAddress;  
};

const unsigned beaconHistoryLength = 5;
static struct BeaconHistoryItem beaconHistory[5];
static unsigned beaconHistoryCounter = 0;


void pushToBeaconHistory
(unsigned apId, const linkaddr_t *address)
{
  struct BeaconHistoryItem newItem = {apId, *address};
  beaconHistory[beaconHistoryCounter] = newItem;
  if (beaconHistoryCounter <= 3) {
    beaconHistoryCounter++;
  } else {
    beaconHistoryCounter = 0;
  }  
}

struct BeaconHistoryItem 
getApIdToConnectToFromBeaconHistory
()
{
    int count = 1, tempCount;
    int i = 0,j = 0;

    struct BeaconHistoryItem temp = {0, linkaddr_null};
    struct BeaconHistoryItem popular = beaconHistory[0];


    //Get first element
    for (i = 0; i < (beaconHistoryLength- 1); i++)
    {
        temp = beaconHistory[i];
        tempCount = 0;
        for (j = 1; j < beaconHistoryLength; j++)
        {
            if (temp.nodeID == beaconHistory[j].nodeID)
                tempCount++;
        }
        if (tempCount > count)
        {
            popular = temp;
            count = tempCount;
        }
    }
    return popular;
}



/*---------------------------------------------------------------------------*/

static linkaddr_t uplinkNodeAddress = {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
static unsigned uplinkNode = 0;
static unsigned connectedMobileNodes[6] = {0, 0, 0, 0, 0, 0};

/* The input callback is called whenever the node received a message */
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{

  // Return early if the received message does not match our payload struct
  if (len != sizeof(struct Payload)) {
    return;
  }

  // Copy payload from buffer in dedicated variable here
  struct Payload receivedPayload;
  memcpy(&receivedPayload, data, sizeof(struct Payload));

  // LOG_INFO("Received message of type %u with value %u\n", receivedPayload.type, receivedPayload.value);

  

  /* Here begins the code for the MOBILE NODE */

  if (amIMobile()) {


    // Case: Mobile node received a beacon
    if (receivedPayload.type == PAYLOAD_TYPE_BEACON) {  
      LOG_INFO("Received beacon from %u with number %u while uplink is %u\n", receivedPayload.ap, receivedPayload.value, uplinkNode);

      pushToBeaconHistory(receivedPayload.ap, src);

      struct BeaconHistoryItem mostPopularAp = getApIdToConnectToFromBeaconHistory();
      // LOG_INFO("Most popular ap is %u\n", mostPopularAp.nodeID);

      if (uplinkNode == 0) {
        // Send connection request to beacon sender because it is the first access point we found
        payload.type = PAYLOAD_TYPE_CONNECTION;
        payload.value = 1;
        payload.mobileNodeId = node_id;
        nullnet_buf = (uint8_t *)&payload;
        nullnet_len = sizeof(payload);

        NETSTACK_NETWORK.output(src);
        LOG_INFO("Sent direct connection request to %u\n", receivedPayload.ap);
        return;
      }

      // Return early if the most popular ap already is our uplink node
      if (mostPopularAp.nodeID == uplinkNode) {
        return;
      }

      // Because the mobile node is still connected to another uplink, we have to disconnect
      payload.type = PAYLOAD_TYPE_DISCONNECTION;
      payload.value = 0;
      payload.mobileNodeId = node_id;
      nullnet_buf = (uint8_t *)&payload;
      nullnet_len = sizeof(payload);

      NETSTACK_NETWORK.output(&uplinkNodeAddress);
      LOG_INFO("Sent disconnection request to %u\n", uplinkNode);
  
      return;
    } 

    // Case: Mobile node received connection acknowledgement
    if (receivedPayload.type == PAYLOAD_TYPE_CONNECTION) {
      if (receivedPayload.value == 0) {
        LOG_ERR("Access point denied connection!\n");
        return;
      }
  
      LOG_INFO("Access point accepted connection. Uplink node is now %u\n", receivedPayload.ap);

      uplinkNodeAddress = *src;
      uplinkNode = receivedPayload.ap;
      leds_on(LEDS_RED);
      return;
    }

    // Case: Mobile node received disconnection acknowledgement
    if (receivedPayload.type == PAYLOAD_TYPE_DISCONNECTION) {
      if (receivedPayload.value == 0) {
        LOG_ERR("Access point denied disconnection!\n");
        return;
      }
      LOG_INFO("Access point %u accepted disconnection\n", receivedPayload.ap);

      // Remove knowledge about the past uplink
      uplinkNode = 0;
      uplinkNodeAddress = linkaddr_null;

      // Connect to new most popular access point
      struct BeaconHistoryItem mostPopularAp = getApIdToConnectToFromBeaconHistory();
      payload.type = PAYLOAD_TYPE_CONNECTION;
      payload.value = 1;
      payload.mobileNodeId = node_id;
      nullnet_buf = (uint8_t *)&payload;
      nullnet_len = sizeof(payload);

      NETSTACK_NETWORK.output(&mostPopularAp.nodeAddress);
      LOG_INFO("Sent connection request to %u\n", mostPopularAp.nodeID);
    }

    return;
  }



  /* Here begins the code for the ACCESS POINT NODES */

  // Case: Access point receives connection request from mobile node
  if (receivedPayload.type == PAYLOAD_TYPE_CONNECTION) {
    LOG_INFO("Mobile node %u wants to connect to me\n", receivedPayload.mobileNodeId);

    // Create packet for connection acknowledgement
    payload.type = PAYLOAD_TYPE_CONNECTION;
    payload.value = 1;
    payload.ap = node_id;
    nullnet_buf = (uint8_t *)&payload;
    nullnet_len = sizeof(payload);

    // Send ack
    NETSTACK_NETWORK.output(src);

    connectedMobileNodes[payload.mobileNodeId - 6] = 1;
    leds_on(LEDS_RED);

    LOG_INFO("Sent acknowledgment\n");
    return;
  }

  // Case: Access point receives value from mobile node
  if (receivedPayload.type == PAYLOAD_TYPE_VALUE) {
    if (connectedMobileNodes[payload.mobileNodeId - 6] == 0) {
      // LOG_INFO("Dropped value message, because the node is not connected\n");
      return;
    }

    LOG_INFO("Logged value %u from mobile node %u\n", receivedPayload.value, receivedPayload.mobileNodeId);
  }

  // Case: Access point receives disconnection request from mobile node
  if (receivedPayload.type == PAYLOAD_TYPE_DISCONNECTION) {
    payload.type = PAYLOAD_TYPE_DISCONNECTION;
    payload.value = 1;
    payload.ap = node_id;
    nullnet_buf = (uint8_t *)&payload;
    nullnet_len = sizeof(payload);

    NETSTACK_NETWORK.output(src);

    LOG_INFO("Accepted disconnection of mobile node %u\n", receivedPayload.mobileNodeId);
    connectedMobileNodes[payload.mobileNodeId - 6] = 0;
    leds_off(LEDS_RED);

    return;
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

  if (amIMobile()) {
    LOG_INFO("I am a mobile node!\n");

    static unsigned counter = 0;
    etimer_set(&periodic_timer, MOBILE_SEND_INTERVAL);

    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

      // Create value packet
      payload.type = PAYLOAD_TYPE_VALUE;
      payload.value= counter;
      payload.ap = uplinkNode;

      // LOG_INFO("Send value %u to uplink node %u\n", counter, uplinkNode);

      // Send value to uplink
      NETSTACK_NETWORK.output(&uplinkNodeAddress);


      counter++;
      etimer_reset(&periodic_timer);
    }    
  } 


  /* Here begins the code for the ACCESS POINT NODES */

  LOG_INFO("I am an access-point node with id %u\n", node_id);

  static unsigned beaconCounter = 0;

  etimer_set(&periodic_timer, AP_SEND_INTERVAL);
  while (1) {   
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    // Create beacon payload
    payload.type = PAYLOAD_TYPE_BEACON;
    payload.value = beaconCounter;
    payload.ap = node_id;

    // Send beacon
    nullnet_buf = (uint8_t *)&payload;
    NETSTACK_NETWORK.output(NULL);

    // LOG_INFO("Broadcasted beacon %u\n", beaconCounter);

    beaconCounter++; 
    etimer_reset(&periodic_timer);
  }
  PROCESS_END(); 
}
