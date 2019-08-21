#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "kernel_io.h"

static int card_detect(const char *mark, const char *cards) {
	int SIZE = 256;
	char buf[SIZE];
	struct file *fp = NULL;
	int len;
	int card = -ENODEV; /* if no card detected */
	char unit[SIZE*2]; /* to process string unit */

	memset(buf, 0, SIZE);

	fp = filp_open(cards, O_RDWR, 0666);
	if(IS_ERR(fp))
		return card;

	unit[0] = '\0';

	while(fgets(fp, buf, SIZE) != NULL)
	{
		len = strlen(buf);
		buf[len-1] = '\0';
		/* space index space [, card index is limited to 0 - 9 */
		if(('[' == buf[3]) && (buf[1] <= '9') && (buf[1] >= '0')){
			strcat(unit, buf);
			while(fgets(fp, buf, SIZE) != NULL){
				if(('[' == buf[3]) && (buf[1] <= '9') && (buf[1] >= '0')){
					len = strlen(buf);
					fp->f_pos -= len; /* back to previous position */
					break;
				}
				strcat(unit, buf); /* append buf to unit */
			}

			if(NULL != strstr(unit, mark)){ /* determine www.loostone.com in unit */
				card = (int)(unit[1] - '0'); /* figure out the card index */
				break; /* by default uses the first device detected conformed with loostone */
			}
			unit[0] = '\0';
		};
	}

	filp_close(fp, NULL);

	return card;
}

static int __init foo_init(void)
{
	int index = -ENODEV;

	index = card_detect("www.loostone.com", "/proc/asound/cards");
	printk("Andrew: %s %s %d %d\n", __FILE__, __func__, __LINE__, index);

	return 0;
}

static void __exit foo_exit(void)
{
}

module_init(foo_init);
module_exit(foo_exit);
MODULE_LICENSE("GPL");
