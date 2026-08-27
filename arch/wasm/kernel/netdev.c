// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/netdev.c: etherstub network device.
 *
 * Packets sent by the kernel go out via wasm_net_send (the WebSocket bridge).
 * Packets from the bridge arrive via wasm_net_recv and are pushed up the
 * network stack by netif_receive_skb().
 */

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <os-wasm.h>
#include <asm/irq.h>

static struct net_device *wasm_netdev;

static void wasm_net_poll(void)
{
	struct sk_buff *skb;
	int n;

	n = wasm_net_recv(NULL, 0); /* peek length */
	if (n <= 0)
		return;

	skb = dev_alloc_skb(n + 2);
	if (!skb)
		return;
	skb_reserve(skb, 2);
	n = wasm_net_recv(skb_put(skb, n), n);
	if (n <= 0) {
		dev_kfree_skb(skb);
		return;
	}
	skb->dev = wasm_netdev;
	skb->protocol = eth_type_trans(skb, wasm_netdev);
	netif_receive_skb(skb);
}

static irqreturn_t wasm_net_irq(int irq, void *dev_id)
{
	wasm_net_poll();
	return IRQ_HANDLED;
}

static netdev_tx_t wasm_net_xmit(struct sk_buff *skb, struct net_device *dev)
{
	wasm_net_send(skb->data, skb->len);
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static int wasm_net_open(struct net_device *dev)
{
	netif_start_queue(dev);
	return 0;
}

static int wasm_net_stop(struct net_device *dev)
{
	netif_stop_queue(dev);
	return 0;
}

static const struct net_device_ops wasm_netdev_ops = {
	.ndo_open = wasm_net_open,
	.ndo_stop = wasm_net_stop,
	.ndo_start_xmit = wasm_net_xmit,
};

static int __init wasm_netdev_init(void)
{
	int err;

	wasm_netdev = alloc_etherdev(0);
	if (!wasm_netdev)
		return -ENOMEM;

	eth_hw_addr_random(wasm_netdev);
	wasm_netdev->netdev_ops = &wasm_netdev_ops;
	wasm_netdev->watchdog_timeo = 5 * HZ;

	err = register_netdev(wasm_netdev);
	if (err) {
		free_netdev(wasm_netdev);
		return err;
	}

	pr_info("wasmux: registered etherstub %s\n", wasm_netdev->name);

	err = request_irq(WASM_NET_IRQ, wasm_net_irq, IRQF_SHARED,
			  "wasm-net", wasm_netdev);
	if (err)
		pr_warn("wasmux: net irq registration failed: %d\n", err);
	return 0;
}
device_initcall(wasm_netdev_init);
