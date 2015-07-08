#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#define GPIO_NUM 67
#define GPIO_NAME "interrupt line"
#define IRQ_NAME "GPIO67_IRQ"

static int gpio_irq_num = 0;

irqreturn_t irq_handler(int irq, void *notused)
{
	pr_alert("Wow motherf****rs, IRQ working !!!\n");
	return IRQ_HANDLED;
}

static int __init gpio_irq_test_init(void) {
	if (gpio_request(GPIO_NUM, GPIO_NAME)) {
		pr_alert("You loose\n");
		return -1;
	}

	if ((gpio_irq_num = gpio_to_irq(GPIO_NUM)) < 0 ) {
		pr_alert("You loose #2\n");
		return -2;
	}

	if (request_irq(gpio_irq_num, irq_handler, 0, IRQ_NAME, NULL)) {
		pr_alert("You loose #3\n");
		return -3;
	}

	irq_set_irq_type(gpio_irq_num, IRQ_TYPE_EDGE_BOTH);
	pr_alert("IRQ added\n");

	return 0;
}

static void __exit gpio_irq_test_exit(void) {
	free_irq(gpio_irq_num, NULL);
	gpio_free(GPIO_NUM);
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Edgar");
MODULE_DESCRIPTION("GPIO peace of shit");

module_init(gpio_irq_test_init);
module_exit(gpio_irq_test_exit);