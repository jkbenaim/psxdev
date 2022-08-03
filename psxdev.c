#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>

MODULE_AUTHOR("Jason Benaim");
MODULE_DESCRIPTION("Driver for Sony DTL-H2500 PlayStation development card");
MODULE_LICENSE("Dual BSD/GPL");
#define DRIVER_NAME "psxdev"

#ifndef PCI_DEVICE_ID_SONY_DTLH2500
#define PCI_DEVICE_ID_SONY_DTLH2500	0x8004
#endif

#define MAX_BOARD (8)

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

static const struct pci_device_id psxdev_pci_tbl[] = {
	{ PCI_VDEVICE(SONY, PCI_DEVICE_ID_SONY_DTLH2500) },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, psxdev_pci_tbl);

static struct psxdev_board {
	unsigned bus;
	unsigned devfn;
	void *regmap;
	unsigned irq;
	struct pci_dev *dev;
	struct gendisk *disk;
	uint8_t bootp;
} board = {0};

ssize_t bootp_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	ssize_t rc;
	rc = sprintf(buf, "%u\n", ioread8(board.regmap + PSX_BOOTP));
	return rc;
}

ssize_t bootp_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int rc;
	unsigned val;
	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val > 255))
		return -EINVAL;
	
	iowrite8(val, board.regmap + PSX_BOOTP);
	return count;
}

ssize_t stat_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	ssize_t rc;
	rc = sprintf(buf, "%u\n", ioread8(board.regmap + PSX_STAT));
	return rc;
}

ssize_t stat_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int rc;
	unsigned val;
	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val > 255))
		return -EINVAL;
	
	iowrite8(val, board.regmap + PSX_STAT);

	return count;
}

ssize_t dipsw_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	ssize_t rc;
	rc = sprintf(buf, "%u\n", ioread8(board.regmap + PSX_DIPSW));
	return rc;
}

ssize_t dipsw_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int rc;
	unsigned val;
	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val > 255))
		return -EINVAL;
	
	iowrite8(val, board.regmap + PSX_DIPSW);

	return count;
}

ssize_t eight_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int i;
	for (i = 0; i < HOST_IF_BUFSIZE; i++) {
		buf[i] = ioread8(board.regmap + PSX_800 + i);
	}
	return i;
}

ssize_t eight_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int i;
	if (count != HOST_IF_BUFSIZE)
		return -EINVAL;
	
	for (i = 0; i < count; i++) {
		iowrite8(buf[i], board.regmap + PSX_800 + i);
	}
	//iowrite8(buf[0], board.regmap + PSX_800 + 0);
	//iowrite8(buf[1], board.regmap + PSX_800 + 1);

	iowrite8(1, board.regmap + PSX_FFF);
	if (ioread8(board.regmap + PSX_STAT) & 4) {
		iowrite8(ioread8(board.regmap + PSX_STAT) | 4, board.regmap + PSX_STAT);
	}
	
	return count;
}

ssize_t cee_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int i;
	for (i = 0; i < HOST_IF_BUFSIZE; i++) {
		buf[i] = ioread8(board.regmap + PSX_C00 + i);
	}
	return i;
}

ssize_t cee_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int i;
	if (count != HOST_IF_BUFSIZE)
		return -EINVAL;
	
	iowrite8(ioread8(board.regmap + PSX_STAT) & 0xa0, board.regmap + PSX_STAT);
	for (i = 0; i < count; i++) {
		iowrite8(buf[i], board.regmap + PSX_C00 + i);
	}
	//iowrite8(buf[0], board.regmap + PSX_C00 + 0);
	//iowrite8(buf[1], board.regmap + PSX_C00 + 1);

	iowrite8(1, board.regmap + PSX_FFF);
	if (ioread8(board.regmap + PSX_STAT) & 4) {
		iowrite8(ioread8(board.regmap + PSX_STAT) | 4, board.regmap + PSX_STAT);
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
	ssize_t rc;
	rc = sprintf(buf, "%u\n", ioread8(board.regmap + PSX_FFE));
	return rc;
}

ssize_t ffe_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int rc;
	unsigned val;
	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val > 255))
		return -EINVAL;
	
	iowrite8(val, board.regmap + PSX_FFE);

	return count;
}

ssize_t fff_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	ssize_t rc;
	rc = sprintf(buf, "%u\n", ioread8(board.regmap + PSX_FFF));
	return rc;
}

ssize_t fff_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int rc;
	unsigned val;
	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val > 255))
		return -EINVAL;
	
	iowrite8(val, board.regmap + PSX_FFF);

	return count;
}

void psxdev_reset(void)
{
	int i;
	for (i = 0; i < HOST_IF_BUFSIZE; i++) {
		iowrite8(0, board.regmap + PSX_800 + i);
		iowrite8(0, board.regmap + PSX_C00 + i);
	}
	for (i = 0; i < 30000; i++) {
		iowrite8(0, board.regmap + PSX_BOOTP);
		iowrite32(0, board.regmap + PSX_C00);
		iowrite32(0, board.regmap + PSX_800);
	}
	iowrite8(PSX_BOOTP_RUN, board.regmap + PSX_BOOTP);
	iowrite8((ioread8(board.regmap + PSX_STAT) & 0xf0) | 4, board.regmap + PSX_STAT);

	iowrite8(0xa0, board.regmap + PSX_STAT);
}

ssize_t preset_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int rc;
	unsigned val;
	rc = sscanf(buf, "%u", &val);
	if ((rc != 1) || (val != 1))
		return -EINVAL;
	
	psxdev_reset();

	return count;
}

ssize_t zeropage_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int i;
	for (i = 0; i < HOST_IF_BUFSIZE; i++) {
		buf[i] = ioread8(board.regmap + i);
	}
	return i;
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
	__ATTR_WO(preset),
	__ATTR_RO(zeropage),
};

static irqreturn_t psxdev_interrupt(int irq, void *dev_id)
{
	uint8_t temp;
	printk("interrupt, [0]=%02xh, [1]=%02xh, [2]=%02xh\n",
		ioread8(board.regmap + PSX_BOOTP),
		ioread8(board.regmap + PSX_STAT),
		ioread8(board.regmap + PSX_DIPSW)
	);
	temp = ioread8(board.regmap + PSX_STAT);
	temp &= 0x0f;
	iowrite8(temp, board.regmap + PSX_STAT);
	printk("handled, [0]=%02xh, [1]=%02xh, [2]=%02xh\n",
		ioread8(board.regmap + PSX_BOOTP),
		ioread8(board.regmap + PSX_STAT),
		ioread8(board.regmap + PSX_DIPSW)
	);
	return IRQ_HANDLED;
}

static int psxdev_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	__label__ err_regions, err_iomap, err_irq, err_attrs;
	int rc;
	int i;
	struct device_attribute *dev_attr;
	printk("was probed %p\n", pdev);

	memset(&board, 0, sizeof(board));

	rc = pci_enable_device(pdev);
	if (rc)
		return rc;
	rc = pci_request_regions(pdev, DRIVER_NAME);
	if (rc)
		goto err_regions;

	board.regmap = pci_iomap(pdev, 0, 0);
	if (!board.regmap) {
		printk(KERN_ALERT "couldn't map BAR!\n");
		rc = -ENOMEM;
		goto err_iomap;
	}

	rc = request_irq(pdev->irq, psxdev_interrupt, 0, DRIVER_NAME, pdev);
	if (rc) {
		printk("failed requesting IRQ%d\n", pdev->irq);
		goto err_irq;
	}

	psxdev_reset();

	printk("bootp: %d\n", ioread8(board.regmap + PSX_BOOTP));
	printk("stat: %d\n", ioread8(board.regmap + PSX_STAT));
	printk("dipsw: %d\n", ioread8(board.regmap + PSX_DIPSW));

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
	pci_iounmap(pdev, board.regmap);
err_iomap:
	pci_release_regions(pdev);
err_regions:
	pci_disable_device(pdev);
	return rc;
}

static void psxdev_remove(struct pci_dev *pdev)
{
	int i;
	struct device_attribute *dev_attr;

	printk("was removed %p\n\n", pdev);
	for (i = 0; i < ARRAY_SIZE(board_attrs); i++) {
		dev_attr = &board_attrs[i];
		device_remove_file(&pdev->dev, dev_attr);
	}
	free_irq(pdev->irq, pdev);
	pci_iounmap(pdev, board.regmap);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
}

static struct pci_driver psxdev_driver = {
	.name		= DRIVER_NAME,
	.id_table	= psxdev_pci_tbl,
	.probe		= psxdev_probe,
	.remove		= psxdev_remove,
	.driver = {
		.owner	= THIS_MODULE,
	},
};

static int __init psxdev_init(void)
{
	int rc;
	printk("hello world\n");

	rc = pci_register_driver(&psxdev_driver);
	if (rc)
		return rc;
	
	return 0;
}

static void __exit psxdev_exit(void)
{
	printk("goodbye world\n");
	
	pci_unregister_driver(&psxdev_driver);
}

module_init(psxdev_init);
module_exit(psxdev_exit);

