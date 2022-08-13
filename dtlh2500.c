#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

MODULE_AUTHOR("Jason Benaim");
MODULE_DESCRIPTION("Driver for Sony DTL-H2500 PlayStation development card");
MODULE_LICENSE("Dual BSD/GPL");
#define DRIVER_NAME "dtlh2500"

#ifndef PCI_DEVICE_ID_SONY_DTLH2500
#define PCI_DEVICE_ID_SONY_DTLH2500	0x8004
#endif

#define MAX_BOARDS (8)

#define HOST_IF_BUFSIZE	0x3fc

// reg addresses
#define PSX_BOOTP	0x000
#define PSX_STAT	0x001
#define PSX_DIPSW	0x002
#define PSX_800		0x800
#define PSX_C00		0xC00
#define PSX_FFE		0xFFE
#define PSX_FFF		0xFFF

// magic bits
#define PSX_BOOTP_RUN	(1<<0)

#define PSX_STAT_IRQACK	(1<<2)

#define PSX_DIPSW_EPROM (1<<3)
#define PSX_DIPSW_FLASH (0<<3)

struct regs_s {
	volatile uint8_t bootp;
	volatile uint8_t stat;
	volatile uint8_t dipsw;
	uint8_t _pad1[2045];
	volatile uint32_t field_800;
	uint8_t _pad2[1020];
	volatile uint32_t field_C00;
	uint8_t _pad3[1018];
	volatile uint8_t field_FFE;
	volatile uint8_t field_FFF;
} __attribute__((packed));

static const struct pci_device_id dtlh2500_pci_tbl[] = {
	{ PCI_VDEVICE(SONY, PCI_DEVICE_ID_SONY_DTLH2500) },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, dtlh2500_pci_tbl);

static struct dtlh2500_board {
	/* corresponding pci device */
	struct pci_dev *pdev;

	/* mapped mmio regs from bar0 */
	char __iomem *regs;
} boards[MAX_BOARDS] = {0};

void dtlh2500_put8(struct dtlh2500_board *board, unsigned reg, uint8_t val)
{
	iowrite8(val, board->regs + reg);
}

uint8_t dtlh2500_get8(struct dtlh2500_board *board, unsigned reg)
{
	return ioread8(board->regs + reg);
}

void dtlh2500_put32(struct dtlh2500_board *board, unsigned reg, uint32_t val)
{
	iowrite32(val, board->regs + reg);
}

uint32_t dtlh2500_get32(struct dtlh2500_board *board, unsigned reg)
{
	return ioread32(board->regs + reg);
}

ssize_t bootp_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct dtlh2500_board *board;
	ssize_t rc;
	board = dev_get_drvdata(dev);

	rc = sprintf(buf, "%u\n", dtlh2500_get8(board, PSX_BOOTP));
	return rc;
}

ssize_t bootp_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct dtlh2500_board *board;
	int rc;
	unsigned val;

	board = dev_get_drvdata(dev);

	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val > 255))
		return -EINVAL;
	
	dtlh2500_put8(board, PSX_BOOTP, val);
	return count;
}

ssize_t stat_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct dtlh2500_board *board;
	ssize_t rc;

	board = dev_get_drvdata(dev);

	rc = sprintf(buf, "%u\n", dtlh2500_get8(board, PSX_STAT));
	return rc;
}

ssize_t stat_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct dtlh2500_board *board;
	int rc;
	unsigned val;

	board = dev_get_drvdata(dev);

	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val > 255))
		return -EINVAL;
	
	dtlh2500_put8(board, PSX_STAT, val);
	return count;
}

ssize_t dipsw_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct dtlh2500_board *board;
	ssize_t rc;

	board = dev_get_drvdata(dev);

	rc = sprintf(buf, "%u\n", dtlh2500_get8(board, PSX_DIPSW));
	return rc;
}

ssize_t dipsw_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct dtlh2500_board *board;
	int rc;
	unsigned val;

	board = dev_get_drvdata(dev);

	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val > 255))
		return -EINVAL;
	
	dtlh2500_put8(board, PSX_DIPSW, val);
	return count;
}

ssize_t eight_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct dtlh2500_board *board;
	int i;

	board = dev_get_drvdata(dev);

	for (i = 0; i < HOST_IF_BUFSIZE; i++) {
		buf[i] = dtlh2500_get8(board, PSX_800 + i);
	}
	return i;
}

ssize_t eight_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct dtlh2500_board *board;
	int i;

	board = dev_get_drvdata(dev);

	if (count != HOST_IF_BUFSIZE)
		return -EINVAL;
	
	for (i = 0; i < count; i++) {
		dtlh2500_put8(board, PSX_800 + i, buf[i]);
	}
	//iowrite8(buf[0], board.regmap + PSX_800 + 0);
	//iowrite8(buf[1], board.regmap + PSX_800 + 1);

	dtlh2500_put8(board, PSX_FFF, 1);
	if (dtlh2500_get8(board, PSX_STAT) & 4) {
		uint8_t tmp;
		tmp = dtlh2500_get8(board, PSX_STAT);
		tmp |= 4;
		dtlh2500_put8(board, PSX_STAT, tmp);
	}
	return count;
}

ssize_t cee_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct dtlh2500_board *board;
	int i;

	board = dev_get_drvdata(dev);

	for (i = 0; i < HOST_IF_BUFSIZE; i++) {
		buf[i] = dtlh2500_get8(board, PSX_C00 + i);
	}
	return i;
}

ssize_t cee_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct dtlh2500_board *board;
	int i;
	uint8_t tmp;

	board = dev_get_drvdata(dev);

	if (count != HOST_IF_BUFSIZE)
		return -EINVAL;
	
	tmp = dtlh2500_get8(board, PSX_STAT);
	tmp &= 0xa0;
	dtlh2500_put8(board, PSX_STAT, tmp);
	for (i = 0; i < count; i++) {
		dtlh2500_put8(board, PSX_C00 + i, buf[i]);
	}
	//iowrite8(buf[0], board.regmap + PSX_C00 + 0);
	//iowrite8(buf[1], board.regmap + PSX_C00 + 1);

	dtlh2500_put8(board, PSX_FFF, 1);
	if (dtlh2500_get8(board, PSX_STAT) & 4) {
		tmp = dtlh2500_get8(board, PSX_STAT);
		tmp |= 4;
		dtlh2500_put8(board, PSX_STAT, tmp);
	}
	return count;
}

ssize_t host_if_bufsize_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	ssize_t rc;
	rc = sprintf(buf, "%u\n", HOST_IF_BUFSIZE);
	return rc;
}

ssize_t ffe_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct dtlh2500_board *board;
	ssize_t rc;

	board = dev_get_drvdata(dev);

	rc = sprintf(buf, "%u\n", dtlh2500_get8(board, PSX_FFE));
	return rc;
}

ssize_t ffe_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct dtlh2500_board *board;
	int rc;
	unsigned val;

	board = dev_get_drvdata(dev);

	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val > 255))
		return -EINVAL;
	
	dtlh2500_put8(board, PSX_FFE, val);
	return count;
}

ssize_t fff_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct dtlh2500_board *board;
	ssize_t rc;

	board = dev_get_drvdata(dev);

	rc = sprintf(buf, "%u\n", dtlh2500_get8(board, PSX_FFF));
	return rc;
}

ssize_t fff_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct dtlh2500_board *board;
	int rc;
	unsigned val;

	board = dev_get_drvdata(dev);

	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val > 255))
		return -EINVAL;
	
	dtlh2500_put8(board, PSX_FFF, val);

	return count;
}

/*
 * dtlh2500_reset
 *
 * This is how sony's tools reset the board.
 * I'm not sure why it uses this particular routine.
 *
 */
void dtlh2500_reset(struct dtlh2500_board *board)
{
	int i;
	uint8_t tmp;

	for (i = 0; i < HOST_IF_BUFSIZE; i++) {
		dtlh2500_put8(board, PSX_800 + i, 0);
		dtlh2500_put8(board, PSX_C00 + i, 0);
	}
	for (i = 0; i < 30000; i++) {
		dtlh2500_put8(board, PSX_BOOTP, 0);
		dtlh2500_put32(board, PSX_C00, 0);
		dtlh2500_put32(board, PSX_800, 0);
	}
	dtlh2500_put8(board, PSX_BOOTP, PSX_BOOTP_RUN);
	tmp = dtlh2500_get8(board, PSX_STAT);
	tmp &= 0xf0;
	tmp |= 4;
	dtlh2500_put8(board, PSX_STAT, tmp);
	dtlh2500_put8(board, PSX_STAT, 0xa0);
}

ssize_t board_reset_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct dtlh2500_board *board;
	int rc;
	unsigned val;

	board = dev_get_drvdata(dev);

	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val != 1))
		return -EINVAL;

	dtlh2500_reset(board);

	return count;
}

static struct device_attribute board_attrs[] = {
	__ATTR_RW(bootp),
	__ATTR_RW(stat),
	__ATTR_RW(dipsw),
	__ATTR_RW(eight),
	__ATTR_RW(cee),
	__ATTR_RO(host_if_bufsize),
	__ATTR_RW(ffe),
	__ATTR_RW(fff),
	__ATTR_WO(board_reset),
};

static irqreturn_t dtlh2500_interrupt(int irq, void *data)
{
	struct dtlh2500_board *board = data;
	uint8_t tmp;
	printk("interrupt, [0]=%02xh, [1]=%02xh, [2]=%02xh\n",
		dtlh2500_get8(board, PSX_BOOTP),
		dtlh2500_get8(board, PSX_STAT ),
		dtlh2500_get8(board, PSX_DIPSW)
	);
	tmp = dtlh2500_get8(board, PSX_STAT);
	tmp &= 0x0f;
	dtlh2500_put8(board, PSX_STAT, tmp);
	printk("handled, [0]=%02xh, [1]=%02xh, [2]=%02xh\n",
		dtlh2500_get8(board, PSX_BOOTP),
		dtlh2500_get8(board, PSX_STAT ),
		dtlh2500_get8(board, PSX_DIPSW)
	);
	return IRQ_HANDLED;
}

static int dtlh2500_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	__label__ err_regions, err_iomap, err_irq, err_attrs;
	int rc;
	int i;
	int boardnum;
	struct dtlh2500_board *board;
	struct device_attribute *dev_attr;
	printk("was probed %p\n", pdev);

	/* find free board struct */
	for (boardnum = 0; boardnum < MAX_BOARDS; boardnum++) {
		if (boards[boardnum].pdev == NULL) {
			board = &boards[boardnum];
			board->pdev = pdev;
			break;
		}
	}
	if (boardnum == MAX_BOARDS) {
		dev_err(&pdev->dev, "couldn't find free board struct. raise MAX_BOARDS.\n");
		return -ENODEV;
	}

	dev_set_drvdata(&pdev->dev, board);

	rc = pci_enable_device(pdev);
	if (rc)
		return rc;
	rc = pci_request_regions(pdev, DRIVER_NAME);
	if (rc)
		goto err_regions;
	
	board->regs = pci_iomap(pdev, 0, 0);
	if (!board->regs) {
		printk(KERN_ALERT "couldn't map BAR!\n");
		rc = -ENOMEM;
		goto err_iomap;
	}

	rc = request_irq(pdev->irq, dtlh2500_interrupt, 0, DRIVER_NAME, board);
	if (rc) {
		printk("failed requesting IRQ%d\n", pdev->irq);
		goto err_irq;
	}

	dtlh2500_put8(board, PSX_DIPSW, PSX_DIPSW_FLASH);
	dtlh2500_reset(board);

	printk("bootp: %d\n", dtlh2500_get8(board, PSX_BOOTP));
	printk("stat: %d\n",  dtlh2500_get8(board, PSX_STAT));
	printk("dipsw: %d\n", dtlh2500_get8(board, PSX_DIPSW));

	// add sysfs entries
	for (i = 0; i < ARRAY_SIZE(board_attrs); i++) {
		dev_attr = &board_attrs[i];
		rc = device_create_file(&pdev->dev, dev_attr);
		if (rc)
			goto err_attrs;
	}

	return 0;

err_attrs:
	for (i--; i<= 0; i--) {
		dev_attr = &board_attrs[i];
		device_remove_file(&pdev->dev, dev_attr);
	}
	free_irq(pdev->irq, pdev);
err_irq:
	pci_iounmap(pdev, board->regs);
err_iomap:
	pci_release_regions(pdev);
err_regions:
	pci_disable_device(pdev);
	board->pdev = NULL;
	return rc;
}

static void dtlh2500_remove(struct pci_dev *pdev)
{
	int i;
	struct dtlh2500_board *board;
	struct device_attribute *dev_attr;

	board = pci_get_drvdata(pdev);

	printk("was removed %p\n\n", pdev);
	for (i = 0; i < ARRAY_SIZE(board_attrs); i++) {
		dev_attr = &board_attrs[i];
		device_remove_file(&pdev->dev, dev_attr);
	}
	free_irq(pdev->irq, pdev);
	pci_iounmap(pdev, board->regs);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
}

static struct pci_driver dtlh2500_driver = {
	.name		= DRIVER_NAME,
	.id_table	= dtlh2500_pci_tbl,
	.probe		= dtlh2500_probe,
	.remove		= dtlh2500_remove,
	.driver = {
		.owner	= THIS_MODULE,
	},
};

static int __init dtlh2500_init(void)
{
	int rc;
	printk("hello world\n");

	rc = pci_register_driver(&dtlh2500_driver);
	if (rc)
		return rc;
	
	return 0;
}

static void __exit dtlh2500_exit(void)
{
	printk("goodbye world\n");
	
	pci_unregister_driver(&dtlh2500_driver);
}

module_init(dtlh2500_init);
module_exit(dtlh2500_exit);

