#include "sys/ctimer.h"
#include "net/mac/transparentmac/transparentmac.h"
#include "net/mac/mac-sequence.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "net/netstack.h"
#include <stdio.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "TMAC"
#define LOG_LEVEL LOG_LEVEL_MAC

/* Constants of the IEEE 802.15.4 standard */
//TODO: use macros below for exponential backoff

#define SLOT_WIDTH_IN_RTIMER_TICKS RTIMER_SECOND / 20
#define SLOT_WIDTH_IN_CTIMER_TICKS CLOCK_SECOND / 20

/* macMinBE: Initial backoff exponent. Range 0--TMAC_MAX_BE */
#ifdef TMAC_CONF_MIN_BE
#define TMAC_MIN_BE TMAC_CONF_MIN_BE
#else
#define TMAC_MIN_BE 3
#endif

/* macMaxBE: Maximum backoff exponent. Range 3--8 */
#ifdef TMAC_CONF_MAX_BE
#define TMAC_MAX_BE CSMA_CONF_MAX_BE
#else
#define TMAC_MAX_BE 5
#endif


/*---------------------------------------------------------------------------*/

static void transmitPacket(mac_callback_t sent, void *ptr);

static int current_backoff_exponent = TMAC_MIN_BE;

static void 
incrementBackoffExponent()
{
  if (current_backoff_exponent < TMAC_MAX_BE) {
    current_backoff_exponent += 1;
  }
}

static void 
sendPacketAtNextSlot(mac_callback_t sent, void *ptr)
{

  LOG_INFO("Send packet at next slot\n");
  unsigned numTicksInCurrentSlot = RTIMER_NOW() % SLOT_WIDTH_IN_RTIMER_TICKS;
  unsigned numTicksUntilNextSlot = SLOT_WIDTH_IN_RTIMER_TICKS - numTicksInCurrentSlot;

  RTIMER_BUSYWAIT(numTicksUntilNextSlot);
  LOG_INFO("Slot reached. Send now\n");

  transmitPacket(sent, ptr);
}

static void
backoffTransmitPacket(mac_callback_t sent, void *ptr)
{
  unsigned numSlotsToBackoff = 1 << current_backoff_exponent;
  unsigned numTicksForExponentialBackoff = SLOT_WIDTH_IN_RTIMER_TICKS * numSlotsToBackoff;
  
  LOG_INFO("Backoff for %d slots which is %d ticks!\n", numSlotsToBackoff, numTicksForExponentialBackoff);
  RTIMER_BUSYWAIT(numTicksForExponentialBackoff);
  sendPacketAtNextSlot(sent, ptr);
}

static void
transmitPacket(mac_callback_t sent, void *ptr) 
{
  int hdr_len;
  int ret;

  LOG_INFO("Try transmitting\n");

  hdr_len = NETSTACK_FRAMER.create();
  if(hdr_len < 0) {
    LOG_ERR("failed to create packet, seqno: %d\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
    ret = MAC_TX_ERR_FATAL;
  } else {
    int is_broadcast;
    uint8_t dsn;
    dsn = ((uint8_t *)packetbuf_hdrptr())[2] & 0xff;

    NETSTACK_RADIO.prepare(packetbuf_hdrptr(), packetbuf_totlen());

    is_broadcast = packetbuf_holds_broadcast();

    if(NETSTACK_RADIO.receiving_packet() ||
       (!is_broadcast && NETSTACK_RADIO.pending_packet())) {

      /* Currently receiving a packet over air or the radio has
         already received a packet that needs to be read before
         sending with auto ack. */
      ret = MAC_TX_COLLISION;

      if (NETSTACK_RADIO.receiving_packet()) {
        LOG_INFO("Currently receiving a packet over air\n");
      } else {
        LOG_INFO("The radio has already received a packet that needs to be read before sending with auto ack.\n");
      }
      // incrementBackoffExponent();
      // backoffTransmitPacket(sent, ptr);
      // return;
    } else {
      switch(NETSTACK_RADIO.transmit(packetbuf_totlen())) {
      case RADIO_TX_OK:
        if(is_broadcast) {
          LOG_INFO("Is Broadcast. Return\n");
          ret = MAC_TX_OK;
        } else {
          /* Check for ack */
          LOG_INFO("MAC_TX_OK. Waiting for ack\n");

          /* Wait for max TMAC_ACK_WAIT_TIME */
          RTIMER_BUSYWAIT_UNTIL(NETSTACK_RADIO.pending_packet(), TMAC_ACK_WAIT_TIME);
          ret = MAC_TX_NOACK;
          if(NETSTACK_RADIO.receiving_packet() ||
             NETSTACK_RADIO.pending_packet() ||
             NETSTACK_RADIO.channel_clear() == 0) {
            int len;
            uint8_t ackbuf[TMAC_ACK_LEN];

            /* Wait an additional TMAC_AFTER_ACK_DETECTED_WAIT_TIME to complete reception */
            RTIMER_BUSYWAIT_UNTIL(NETSTACK_RADIO.pending_packet(), TMAC_AFTER_ACK_DETECTED_WAIT_TIME);

            if(NETSTACK_RADIO.pending_packet()) {
              len = NETSTACK_RADIO.read(ackbuf, TMAC_ACK_LEN);
              if(len == TMAC_ACK_LEN && ackbuf[2] == dsn) {
                /* Ack received */
                ret = MAC_TX_OK;
              } else {
                /* Not an ack or ack not for us: collision */
                ret = MAC_TX_COLLISION;
                 //TODO: retry sending after backoff period
                LOG_INFO("A collision occured. Send again.");
                incrementBackoffExponent();
                backoffTransmitPacket(sent, ptr);
              }
            }
          }
        }
        break;
      case RADIO_TX_COLLISION:
        ret = MAC_TX_COLLISION;
        //TODO: retry sending after backoff period
        LOG_INFO("A collision occured. Send again.");
        incrementBackoffExponent();
        backoffTransmitPacket(sent, ptr);
      default:
        LOG_ERR("MAC_TX_ERR\n");
        ret = MAC_TX_ERR;
        break;
      }
    }
  }
  current_backoff_exponent = TMAC_MIN_BE;
  LOG_INFO("Call sent callback \n");
  mac_call_sent_callback(sent, ptr, ret, 1);
}


static void
send_packet(mac_callback_t sent, void *ptr)
{
  static uint8_t initialized = 0;
  static uint8_t seqno;


  if(!initialized) {
    initialized = 1;
    /* Initialize the sequence number to a random value as per 802.15.4. */
    seqno = random_rand();
  }

  if(seqno == 0) {
    /* PACKETBUF_ATTR_MAC_SEQNO cannot be zero, due to a pecuilarity
       in framer-802154.c. */
    seqno++;
  }
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, seqno++);
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);

  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_ACK, 1);



  sendPacketAtNextSlot(sent, ptr);
}



/*---------------------------------------------------------------------------*/
static void
input_packet(void)
{
#if TMAC_SEND_SOFT_ACK
  uint8_t ackdata[TMAC_ACK_LEN];
#endif

  int hdr_len;

  LOG_INFO("Starting to parse an input packet\n");

  hdr_len = NETSTACK_FRAMER.parse();
  if(hdr_len < 0) {
    LOG_ERR("failed to parse %u\n", packetbuf_datalen());
  } else if(packetbuf_datalen() == TMAC_ACK_LEN && ((uint8_t *)packetbuf_dataptr())[0] == FRAME802154_ACKFRAME) {
    /* Ignore ack packets */
    LOG_DBG("ignored ack\n");
  } else if(!linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
                                         &linkaddr_node_addr) &&
            !packetbuf_holds_broadcast()) {
    LOG_WARN("not for us\n");
  } else if(linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_SENDER), &linkaddr_node_addr)) {
    LOG_WARN("frame from ourselves\n");
  } else {
    int duplicate = 0;

    /* Check for duplicate packet. */
    duplicate = mac_sequence_is_duplicate();
    if(duplicate) {
      /* Drop the packet. */
      LOG_WARN("drop duplicate link layer packet from ");
      LOG_WARN_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
      LOG_WARN_(", seqno %u\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
    } else {
      mac_sequence_register_seqno();
    }

#if TMAC_SEND_SOFT_ACK
    LOG_INFO("time to ack\n");
    if(packetbuf_attr(PACKETBUF_ATTR_MAC_ACK)) {
      ackdata[0] = FRAME802154_ACKFRAME;
      ackdata[1] = 0;
      ackdata[2] = ((uint8_t *)packetbuf_hdrptr())[2];
      NETSTACK_RADIO.send(ackdata, TMAC_ACK_LEN);
    }
#endif /* TMAC_SEND_SOFT_ACK */
    if(!duplicate) {
      LOG_INFO("I received a packet from ");
      LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
      LOG_INFO_(", seqno %u, len %u\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO), packetbuf_datalen());
      NETSTACK_NETWORK.input();
    }
  }
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  LOG_INFO("radio on!\n");
  return NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return NETSTACK_RADIO.off();
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  radio_value_t radio_max_payload_len;

  /* Check that the radio can correctly report its max supported payload */
  if(NETSTACK_RADIO.get_value(RADIO_CONST_MAX_PAYLOAD_LEN, &radio_max_payload_len) != RADIO_RESULT_OK) {
    LOG_ERR("! radio does not support getting RADIO_CONST_MAX_PAYLOAD_LEN. Abort init.\n");
    return;
  }

  /* Forced printf as workaround to not call on() too quickly. Without it
  the radio doesn't start succesfully. Don't use for anything else.
  Use Logging instead.*/
  printf("init TMAC ..\n");

  on();
}
/*---------------------------------------------------------------------------*/
static int
max_payload(void)
{
  int framer_hdrlen;
  radio_value_t max_radio_payload_len;
  radio_result_t res;

  framer_hdrlen = NETSTACK_FRAMER.length();

  res = NETSTACK_RADIO.get_value(RADIO_CONST_MAX_PAYLOAD_LEN,
                                 &max_radio_payload_len);

  if(res == RADIO_RESULT_NOT_SUPPORTED) {
    LOG_ERR("Failed to retrieve max radio driver payload length\n");
    return 0;
  }

  if(framer_hdrlen < 0) {
    /* Framing failed, we assume the maximum header length */
    framer_hdrlen = TMAC_MAC_MAX_HEADER;
  }

  return MIN(max_radio_payload_len, PACKETBUF_SIZE)
    - framer_hdrlen;
}
/*---------------------------------------------------------------------------*/
const struct mac_driver transparentmac_driver = {
  "TMAC",
  init,
  send_packet,
  input_packet,
  on,
  off,
  max_payload,
};
/*---------------------------------------------------------------------------*/
