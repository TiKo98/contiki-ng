#include "contiki.h"

#include <stdio.h> /* For printf() */

/*---------------------------------------------------------------------------*/

PROCESS(timer_process, "Timer process");
AUTOSTART_PROCESSES(&timer_process);

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(timer_process, ev, data)
{
  /* Introduce timer */
  static struct etimer timer;

  /* Begin with an interval of two seconds. Will double after each timer restart. */ 
  static int timer_interval = 2 * CLOCK_SECOND;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, timer_interval);

  while(1) {
    /* Wait for the periodic timer to expire. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    /* Print statement comes after first timer expiry as stated in the task. */
    printf("Hello Simon, Tim, and the world!\n");

    /* Double the interval and start the timer again. */
    timer_interval = timer_interval * 2;
    etimer_set(&timer, timer_interval );
  }

  PROCESS_END();
}
