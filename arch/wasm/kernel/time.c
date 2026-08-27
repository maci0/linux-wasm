// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/time.c: clock source and timer event device.
 *
 * The runtime's monotonic clock is the clocksource; a one-shot armed via
 * wasm_timer_arm() is the timer event device (the runtime raises TIMER_IRQ).
 */

#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <os-wasm.h>

static u64 wasm_clock_read(struct clocksource *cs)
{
	return wasm_time_ns();
}

static struct clocksource wasm_clocksource = {
	.name	= "wasm",
	.rating	= 400,
	.read	= wasm_clock_read,
	.mask	= CLOCKSOURCE_MASK(64),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static int wasm_timer_next_event(unsigned long delta,
				 struct clock_event_device *evt)
{
	wasm_timer_arm((unsigned long long)delta);
	return 0;
}

static int wasm_timer_shutdown(struct clock_event_device *evt)
{
	return 0;
}

static struct clock_event_device wasm_timer = {
	.name			= "wasm-timer",
	.features		= CLOCK_EVT_FEAT_ONESHOT,
	.rating			= 400,
	.set_next_event		= wasm_timer_next_event,
	.set_state_oneshot	= wasm_timer_shutdown,
	.set_state_shutdown	= wasm_timer_shutdown,
};

static irqreturn_t wasm_timer_interrupt(int irq, void *dev_id)
{
	wasm_timer.event_handler(&wasm_timer);
	return IRQ_HANDLED;
}

void __init wasm_timer_init(void)
{
	int err;

	err = request_irq(TIMER_IRQ, wasm_timer_interrupt, IRQF_TIMER,
			  "wasm-timer", NULL);
	if (err)
		pr_err("wasmux: timer irq registration failed: %d\n", err);

	/* ns-resolution clocksource and a 1Hz-bounded oneshot timer */
	clockevents_config_and_register(&wasm_timer, 1000000000ULL /* 1 GHz */,
					1, 0xffffffff);
	clocksource_register_hz(&wasm_clocksource, 1000000000ULL);
}

void __init time_init(void)
{
	wasm_timer_init();
}
