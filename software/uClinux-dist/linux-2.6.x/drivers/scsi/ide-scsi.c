/*
 * Copyright (C) 1996-1999  Gadi Oxman <gadio@netvision.net.il>
 * Copyright (C) 2004-2005  Bartlomiej Zolnierkiewicz
 */

/*
 * Emulation of a SCSI host adapter for IDE ATAPI devices.
 *
 * With this driver, one can use the Linux SCSI drivers instead of the
 * native IDE ATAPI drivers.
 *
 * Ver 0.1   Dec  3 96   Initial version.
 * Ver 0.2   Jan 26 97   Fixed bug in cleanup_module() and added emulation
 *                        of MODE_SENSE_6/MODE_SELECT_6 for cdroms. Thanks
 *                        to Janos Farkas for pointing this out.
 *                       Avoid using bitfields in structures for m68k.
 *                       Added Scatter/Gather and DMA support.
 * Ver 0.4   Dec  7 97   Add support for ATAPI PD/CD drives.
 *                       Use variable timeout for each command.
 * Ver 0.5   Jan  2 98   Fix previous PD/CD support.
 *                       Allow disabling of SCSI-6 to SCSI-10 transformation.
 * Ver 0.6   Jan 27 98   Allow disabling of SCSI command translation layer
 *                        for access through /dev/sg.
 *                       Fix MODE_SENSE_6/MODE_SELECT_6/INQUIRY translation.
 * Ver 0.7   Dec 04 98   Ignore commands where lun != 0 to avoid multiple
 *                        detection of devices with CONFIG_SCSI_MULTI_LUN
 * Ver 0.8   Feb 05 99   Optical media need translation too. Reverse 0.7.
 * Ver 0.9   Jul 04 99   Fix a bug in SG_SET_TRANSFORM.
 * Ver 0.91  Jun 10 02   Fix "off by one" error in transforms
 * Ver 0.92  Dec 31 02   Implement new SCSI mid level API
 */

#define IDESCSI_VERSION "0.92"

#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/slab.h>
#include <linux/ide.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/bitops.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <scsi/sg.h>

#define IDESCSI_DEBUG_LOG		0

typedef struct idescsi_pc_s {
	u8 c[12];				/* Actual packet bytes */
	int request_transfer;			/* Bytes to transfer */
	int actually_transferred;		/* Bytes actually transferred */
	int buffer_size;			/* Size of our data buffer */
	struct request *rq;			/* The corresponding request */
	u8 *buffer;				/* Data buffer */
	u8 *current_position;			/* Pointer into the above buffer */
	struct scatterlist *sg;			/* Scatter gather table */
	unsigned int sg_cnt;			/* Number of entries in sg */
	int b_count;				/* Bytes transferred from current entry */
	struct scsi_cmnd *scsi_cmd;		/* SCSI command */
	void (*done)(struct scsi_cmnd *);	/* Scsi completion routine */
	unsigned long flags;			/* Status/Action flags */
	unsigned long timeout;			/* Command timeout */
} idescsi_pc_t;

/*
 *	Packet command status bits.
 */
#define PC_DMA_IN_PROGRESS		0	/* 1 while DMA in progress */
#define PC_WRITING			1	/* Data direction */
#define PC_TIMEDOUT			3	/* command timed out */
#define PC_DMA_OK			4	/* Use DMA */

/*
 *	SCSI command transformation layer
 */
#define IDESCSI_SG_TRANSFORM		1	/* /dev/sg transformation */

/*
 *	Log flags
 */
#define IDESCSI_LOG_CMD			0	/* Log SCSI commands */

typedef struct ide_scsi_obj {
	ide_drive_t		*drive;
	ide_driver_t		*driver;
	struct gendisk		*disk;
	struct Scsi_Host	*host;

	idescsi_pc_t *pc;			/* Current packet command */
	unsigned long flags;			/* Status/Action flags */
	unsigned long transform;		/* SCSI cmd translation layer */
	unsigned long log;			/* log flags */
} idescsi_scsi_t;

static DEFINE_MUTEX(idescsi_ref_mutex);
static int idescsi_nocd;			/* Set by module param to skip cd */

#define ide_scsi_g(disk) \
	container_of((disk)->private_data, struct ide_scsi_obj, driver)

static struct ide_scsi_obj *ide_scsi_get(struct gendisk *disk)
{
	struct ide_scsi_obj *scsi = NULL;

	mutex_lock(&idescsi_ref_mutex);
	scsi = ide_scsi_g(disk);
	if (scsi)
		scsi_host_get(scsi->host);
	mutex_unlock(&idescsi_ref_mutex);
	return scsi;
}

static void ide_scsi_put(struct ide_scsi_obj *scsi)
{
	mutex_lock(&idescsi_ref_mutex);
	scsi_host_put(scsi->host);
	mutex_unlock(&idescsi_ref_mutex);
}

static inline idescsi_scsi_t *scsihost_to_idescsi(struct Scsi_Host *host)
{
	return (idescsi_scsi_t*) (&host[1]);
}

static inline idescsi_scsi_t *drive_to_idescsi(ide_drive_t *ide_drive)
{
	return scsihost_to_idescsi(ide_drive->driver_data);
}

/*
 *	Per ATAPI device status bits.
 */
#define IDESCSI_DRQ_INTERRUPT		0	/* DRQ interrupt device */

/*
 *	ide-scsi requests.
 */
#define IDESCSI_PC_RQ			90

static void idescsi_discard_data (ide_drive_t *drive, unsigned int bcount)
{
	while (bcount--)
		(void) HWIF(drive)->INB(IDE_DATA_REG);
}

static void idescsi_output_zeros (ide_drive_t *drive, unsigned int bcount)
{
	while (bcount--)
		HWIF(drive)->OUTB(0, IDE_DATA_REG);
}

/*
 *	PIO data transfer routines using the scatter gather table.
 */
static void idescsi_input_buffers (ide_drive_t *drive, idescsi_pc_t *pc, unsigned int bcount)
{
	int count;
	char *buf;

	while (bcount) {
		count = min(pc->sg->length - pc->b_count, bcount);
		if (PageHighMem(sg_page(pc->sg))) {
			unsigned long flags;

			local_irq_save(flags);
			buf = kmap_atomic(sg_page(pc->sg), KM_IRQ0) +
					pc->sg->offset;
			drive->hwif->atapi_input_bytes(drive,
						buf + pc->b_count, count);
			kunmap_atomic(buf - pc->sg->offset, KM_IRQ0);
			local_irq_restore(flags);
		} else {
			buf = sg_virt(pc->sg);
			drive->hwif->atapi_input_bytes(drive,
						buf + pc->b_count, count);
		}
		bcount -= count; pc->b_count += count;
		if (pc->b_count == pc->sg->length) {
			if (!--pc->sg_cnt)
				break;
			pc->sg = sg_next(pc->sg);
			pc->b_count = 0;
		}
	}

	if (bcount) {
		printk (KERN_ERR "ide-scsi: scatter gather table too small, discarding data\n");
		idescsi_discard_data (drive, bcount);
	}
}

static void idescsi_output_buffers (ide_drive_t *drive, idescsi_pc_t *pc, unsigned int bcount)
{
	int count;
	char *buf;

	while (bcount) {
		count = min(pc->sg->length - pc->b_count, bcount);
		if (PageHighMem(sg_page(pc->sg))) {
			unsigned long flags;

			local_irq_save(flags);
			buf = kmap_atomic(sg_page(pc->sg), KM_IRQ0) +
						pc->sg->offset;
			drive->hwif->atapi_output_bytes(drive,
						buf + pc->b_count, count);
			kunmap_atomic(buf - pc->sg->offset, KM_IRQ0);
			local_irq_restore(flags);
		} else {
			buf = sg_virt(pc->sg);
			drive->hwif->atapi_output_bytes(drive,
						buf + pc->b_count, count);
		}
		bcount -= count; pc->b_count += count;
		if (pc->b_count == pc->sg->length) {
			if (!--pc->sg_cnt)
				break;
			pc->sg = sg_next(pc->sg);
			pc->b_count = 0;
		}
	}

	if (bcount) {
		printk (KERN_ERR "ide-scsi: scatter gather table too small, padding with zeros\n");
		idescsi_output_zeros (drive, bcount);
	}
}

static void ide_scsi_hex_dump(u8 *data, int len)
{
	print_hex_dump(KERN_CONT, "", DUMP_PREFIX_NONE, 16, 1, data, len, 0);
}

static int idescsi_check_condition(ide_drive_t *drive, struct request *failed_command)
{
	idescsi_scsi_t *scsi = drive_to_idescsi(drive);
	idescsi_pc_t   *pc;
	struct request *rq;
	u8             *buf;

	/* stuff a sense request in front of our current request */
	pc = kzalloc(sizeof(idescsi_pc_t), GFP_ATOMIC);
	rq = kmalloc(sizeof(struct request), GFP_ATOMIC);
	buf = kzalloc(SCSI_SENSE_BUFFERSIZE, GFP_ATOMIC);
	if (!pc || !rq || !buf) {
		kfree(buf);
		kfree(rq);
		kfree(pc);
		return -ENOMEM;
	}
	ide_init_drive_cmd(rq);
	rq->special = (char *) pc;
	pc->rq = rq;
	pc->buffer = buf;
	pc->c[0] = REQUEST_SENSE;
	pc->c[4] = pc->request_transfer = pc->buffer_size = SCSI_SENSE_BUFFERSIZE;
	rq->cmd_type = REQ_TYPE_SENSE;
	pc->timeout = jiffies + WAIT_READY;
	/* NOTE! Save the failed packet command in "rq->buffer" */
	rq->buffer = (void *) failed_command->special;
	pc->scsi_cmd = ((idescsi_pc_t *) failed_command->special)->scsi_cmd;
	if (test_bit(IDESCSI_LOG_CMD, &scsi->log)) {
		printk ("ide-scsi: %s: queue cmd = ", drive->name);
		ide_scsi_hex_dump(pc->c, 6);
	}
	rq->rq_disk = scsi->disk;
	return ide_do_drive_cmd(drive, rq, ide_preempt);
}

static int idescsi_end_request(ide_drive_t *, int, int);

static ide_startstop_t
idescsi_atapi_error(ide_drive_t *drive, struct request *rq, u8 stat, u8 err)
{
	if (ide_read_status(drive) & (BUSY_STAT | DRQ_STAT))
		/* force an abort */
		HWIF(drive)->OUTB(WIN_IDLEIMMEDIATE,IDE_COMMAND_REG);

	rq->errors++;

	idescsi_end_request(drive, 0, 0);

	return ide_stopped;
}

static ide_startstop_t
idescsi_atapi_abort(ide_drive_t *drive, struct request *rq)
{
#if IDESCSI_DEBUG_LOG
	printk(KERN_WARNING "idescsi_atapi_abort called for %lu\n",
			((idescsi_pc_t *) rq->special)->scsi_cmd->serial_number);
#endif
	rq->errors |= ERROR_MAX;

	idescsi_end_request(drive, 0, 0);

	return ide_stopped;
}

static int idescsi_end_request (ide_drive_t *drive, int uptodate, int nrsecs)
{
	idescsi_scsi_t *scsi = drive_to_idescsi(drive);
	struct request *rq = HWGROUP(drive)->rq;
	idescsi_pc_t *pc = (idescsi_pc_t *) rq->special;
	int log = test_bit(IDESCSI_LOG_CMD, &scsi->log);
	struct Scsi_Host *host;
	int errors = rq->errors;
	unsigned long flags;

	if (!blk_special_request(rq) && !blk_sense_request(rq)) {
		ide_end_request(drive, uptodate, nrsecs);
		return 0;
	}
	ide_end_drive_cmd (drive, 0, 0);
	if (blk_sense_request(rq)) {
		idescsi_pc_t *opc = (idescsi_pc_t *) rq->buffer;
		if (log) {
			printk ("ide-scsi: %s: wrap up check %lu, rst = ", drive->name, opc->scsi_cmd->serial_number);
			ide_scsi_hex_dump(pc->buffer, 16);
		}
		memcpy((void *) opc->scsi_cmd->sense_buffer, pc->buffer, SCSI_SENSE_BUFFERSIZE);
		kfree(pc->buffer);
		kfree(pc);
		kfree(rq);
		pc = opc;
		rq = pc->rq;
		pc->scsi_cmd->result = (CHECK_CONDITION << 1) |
					((test_bit(PC_TIMEDOUT, &pc->flags)?DID_TIME_OUT:DID_OK) << 16);
	} else if (test_bit(PC_TIMEDOUT, &pc->flags)) {
		if (log)
			printk (KERN_WARNING "ide-scsi: %s: timed out for %lu\n",
					drive->name, pc->scsi_cmd->serial_number);
		pc->scsi_cmd->result = DID_TIME_OUT << 16;
	} else if (errors >= ERROR_MAX) {
		pc->scsi_cmd->result = DID_ERROR << 16;
		if (log)
			printk ("ide-scsi: %s: I/O error for %lu\n", drive->name, pc->scsi_cmd->serial_number);
	} else if (errors) {
		if (log)
			printk ("ide-scsi: %s: check condition for %lu\n", drive->name, pc->scsi_cmd->serial_number);
		if (!idescsi_check_condition(drive, rq))
			/* we started a request sense, so we'll be back, exit for now */
			return 0;
		pc->scsi_cmd->result = (CHECK_CONDITION << 1) | (DID_OK << 16);
	} else {
		pc->scsi_cmd->result = DID_OK << 16;
	}
	host = pc->scsi_cmd->device->host;
	spin_lock_irqsave(host->host_lock, flags);
	pc->done(pc->scsi_cmd);
	spin_unlock_irqrestore(host->host_lock, flags);
	kfree(pc);
	kfree(rq);
	scsi->pc = NULL;
	return 0;
}

static inline unsigned long get_timeout(idescsi_pc_t *pc)
{
	return max_t(unsigned long, WAIT_CMD, pc->timeout - jiffies);
}

static int idescsi_expiry(ide_drive_t *drive)
{
	idescsi_scsi_t *scsi = drive_to_idescsi(drive);
	idescsi_pc_t   *pc   = scsi->pc;

#if IDESCSI_DEBUG_LOG
	printk(KERN_WARNING "idescsi_expiry called for %lu at %lu\n", pc->scsi_cmd->serial_number, jiffies);
#endif
	set_bit(PC_TIMEDOUT, &pc->flags);

	return 0;					/* we do not want the ide subsystem to retry */
}

/*
 *	Our interrupt handler.
 */
static ide_startstop_t idescsi_pc_intr (ide_drive_t *drive)
{
	idescsi_scsi_t *scsi = drive_to_idescsi(drive);
	ide_hwif_t *hwif = drive->hwif;
	idescsi_pc_t *pc = scsi->pc;
	struct request *rq = pc->rq;
	unsigned int temp;
	u16 bcount;
	u8 stat, ireason;

#if IDESCSI_DEBUG_LOG
	printk (KERN_INFO "ide-scsi: Reached idescsi_pc_intr interrupt handler\n");
#endif /* IDESCSI_DEBUG_LOG */

	if (test_bit(PC_TIMEDOUT, &pc->flags)){
#if IDESCSI_DEBUG_LOG
		printk(KERN_WARNING "idescsi_pc_intr: got timed out packet  %lu at %lu\n",
				pc->scsi_cmd->serial_number, jiffies);
#endif
		/* end this request now - scsi should retry it*/
		idescsi_end_request (drive, 1, 0);
		return ide_stopped;
	}
	if (test_and_clear_bit (PC_DMA_IN_PROGRESS, &pc->flags)) {
#if IDESCSI_DEBUG_LOG
		printk ("ide-scsi: %s: DMA complete\n", drive->name);
#endif /* IDESCSI_DEBUG_LOG */
		pc->actually_transferred=pc->request_transfer;
		(void) HWIF(drive)->ide_dma_end(drive);
	}

	/* Clear the interrupt */
	stat = ide_read_status(drive);

	if ((stat & DRQ_STAT) == 0) {
		/* No more interrupts */
		if (test_bit(IDESCSI_LOG_CMD, &scsi->log))
			printk (KERN_INFO "Packet command completed, %d bytes transferred\n", pc->actually_transferred);
		local_irq_enable_in_hardirq();
		if (stat & ERR_STAT)
			rq->errors++;
		idescsi_end_request (drive, 1, 0);
		return ide_stopped;
	}
	bcount = (hwif->INB(IDE_BCOUNTH_REG) << 8) |
		  hwif->INB(IDE_BCOUNTL_REG);
	ireason = hwif->INB(IDE_IREASON_REG);

	if (ireason & CD) {
		printk(KERN_ERR "ide-scsi: CoD != 0 in idescsi_pc_intr\n");
		return ide_do_reset (drive);
	}
	if (ireason & IO) {
		temp = pc->actually_transferred + bcount;
		if (temp > pc->request_transfer) {
			if (temp > pc->buffer_size) {
				printk(KERN_ERR "ide-scsi: The scsi wants to "
					"send us more data than expected "
					"- discarding data\n");
				temp = pc->buffer_size - pc->actually_transferred;
				if (temp) {
					clear_bit(PC_WRITING, &pc->flags);
					if (pc->sg)
						idescsi_input_buffers(drive, pc, temp);
					else
						drive->hwif->atapi_input_bytes(drive, pc->current_position, temp);
					printk(KERN_ERR "ide-scsi: transferred"
							" %d of %d bytes\n",
							temp, bcount);
				}
				pc->actually_transferred += temp;
				pc->current_position += temp;
				idescsi_discard_data(drive, bcount - temp);
				ide_set_handler(drive, &idescsi_pc_intr, get_timeout(pc), idescsi_expiry);
				return ide_started;
			}
#if IDESCSI_DEBUG_LOG
			printk (KERN_NOTICE "ide-scsi: The scsi wants to send us more data than expected - allowing transfer\n");
#endif /* IDESCSI_DEBUG_LOG */
		}
	}
	if (ireason & IO) {
		clear_bit(PC_WRITING, &pc->flags);
		if (pc->sg)
			idescsi_input_buffers(drive, pc, bcount);
		else
			hwif->atapi_input_bytes(drive, pc->current_position,
						bcount);
	} else {
		set_bit(PC_WRITING, &pc->flags);
		if (pc->sg)
			idescsi_output_buffers(drive, pc, bcount);
		else
			hwif->atapi_output_bytes(drive, pc->current_position,
						 bcount);
	}
	/* Update the current position */
	pc->actually_transferred += bcount;
	pc->current_position += bcount;

	/* And set the interrupt handler again */
	ide_set_handler(drive, &idescsi_pc_intr, get_timeout(pc), idescsi_expiry);
	return ide_started;
}

static ide_startstop_t idescsi_transfer_pc(ide_drive_t *drive)
{
	ide_hwif_t *hwif = drive->hwif;
	idescsi_scsi_t *scsi = drive_to_idescsi(drive);
	idescsi_pc_t *pc = scsi->pc;
	ide_startstop_t startstop;
	u8 ireason;

	if (ide_wait_stat(&startstop,drive,DRQ_STAT,BUSY_STAT,WAIT_READY)) {
		printk(KERN_ERR "ide-scsi: Strange, packet command "
			"initiated yet DRQ isn't asserted\n");
		return startstop;
	}
	ireason = hwif->INB(IDE_IREASON_REG);
	if ((ireason & CD) == 0 || (ireason & IO)) {
		printk(KERN_ERR "ide-scsi: (IO,CoD) != (0,1) while "
				"issuing a packet command\n");
		return ide_do_reset (drive);
	}
	BUG_ON(HWGROUP(drive)->handler != NULL);
	/* Set the interrupt routine */
	ide_set_handler(drive, &idescsi_pc_intr, get_timeout(pc), idescsi_expiry);
	/* Send the actual packet */
	drive->hwif->atapi_output_bytes(drive, scsi->pc->c, 12);
	if (test_bit (PC_DMA_OK, &pc->flags)) {
		set_bit (PC_DMA_IN_PROGRESS, &pc->flags);
		hwif->dma_start(drive);
	}
	return ide_started;
}

static inline int idescsi_set_direction(idescsi_pc_t *pc)
{
	switch (pc->c[0]) {
		case READ_6: case READ_10: case READ_12:
			clear_bit(PC_WRITING, &pc->flags);
			return 0;
		case WRITE_6: case WRITE_10: case WRITE_12:
			set_bit(PC_WRITING, &pc->flags);
			return 0;
		default:
			return 1;
	}
}

static int idescsi_map_sg(ide_drive_t *drive, idescsi_pc_t *pc)
{
	ide_hwif_t *hwif = drive->hwif;
	struct scatterlist *sg, *scsi_sg;
	int segments;

	if (!pc->request_transfer || pc->request_transfer % 1024)
		return 1;

	if (idescsi_set_direction(pc))
		return 1;

	sg = hwif->sg_table;
	scsi_sg = scsi_sglist(pc->scsi_cmd);
	segments = scsi_sg_count(pc->scsi_cmd);

	if (segments > hwif->sg_max_nents)
		return 1;

	hwif->sg_nents = segments;
	memcpy(sg, scsi_sg, sizeof(*sg) * segments);

	return 0;
}

/*
 *	Issue a packet command
 */
static ide_startstop_t idescsi_issue_pc (ide_drive_t *drive, idescsi_pc_t *pc)
{
	idescsi_scsi_t *scsi = drive_to_idescsi(drive);
	ide_hwif_t *hwif = drive->hwif;
	u16 bcount;
	u8 dma = 0;

	scsi->pc=pc;							/* Set the current packet command */
	pc->actually_transferred=0;					/* We haven't transferred any data yet */
	pc->current_position=pc->buffer;
	/* Request to transfer the entire buffer at once */
	bcount = min(pc->request_transfer, 63 * 1024);

	if (drive->using_dma && !idescsi_map_sg(drive, pc)) {
		hwif->sg_mapped = 1;
		dma = !hwif->dma_setup(drive);
		hwif->sg_mapped = 0;
	}

	ide_pktcmd_tf_load(drive, IDE_TFLAG_NO_SELECT_MASK, bcount, dma);

	if (dma)
		set_bit(PC_DMA_OK, &pc->flags);

	if (test_bit(IDESCSI_DRQ_INTERRUPT, &scsi->flags)) {
		ide_execute_command(drive, WIN_PACKETCMD, &idescsi_transfer_pc,
				    get_timeout(pc), idescsi_expiry);
		return ide_started;
	} else {
		/* Issue the packet command */
		HWIF(drive)->OUTB(WIN_PACKETCMD, IDE_COMMAND_REG);
		return idescsi_transfer_pc(drive);
	}
}

/*
 *	idescsi_do_request is our request handling function.
 */
static ide_startstop_t idescsi_do_request (ide_drive_t *drive, struct request *rq, sector_t block)
{
#if IDESCSI_DEBUG_LOG
	printk (KERN_INFO "dev: %s, cmd: %x, errors: %d\n", rq->rq_disk->disk_name,rq->cmd[0],rq->errors);
	printk (KERN_INFO "sector: %ld, nr_sectors: %ld, current_nr_sectors: %d\n",rq->sector,rq->nr_sectors,rq->current_nr_sectors);
#endif /* IDESCSI_DEBUG_LOG */

	if (blk_sense_request(rq) || blk_special_request(rq)) {
		return idescsi_issue_pc (drive, (idescsi_pc_t *) rq->special);
	}
	blk_dump_rq_flags(rq, "ide-scsi: unsup command");
	idescsi_end_request (drive, 0, 0);
	return ide_stopped;
}

#ifdef CONFIG_IDE_PROC_FS
static void idescsi_add_settings(ide_drive_t *drive)
{
	idescsi_scsi_t *scsi = drive_to_idescsi(drive);

/*
 *			drive	setting name	read/write	data type	min	max	mul_factor	div_factor	data pointer		set function
 */
	ide_add_setting(drive,	"bios_cyl",	SETTING_RW,	TYPE_INT,	0,	1023,	1,		1,		&drive->bios_cyl,	NULL);
	ide_add_setting(drive,	"bios_head",	SETTING_RW,	TYPE_BYTE,	0,	255,	1,		1,		&drive->bios_head,	NULL);
	ide_add_setting(drive,	"bios_sect",	SETTING_RW,	TYPE_BYTE,	0,	63,	1,		1,		&drive->bios_sect,	NULL);
	ide_add_setting(drive,	"transform",	SETTING_RW,	TYPE_INT,	0,	3,	1,		1,		&scsi->transform,	NULL);
	ide_add_setting(drive,	"log",		SETTING_RW,	TYPE_INT,	0,	1,	1,		1,		&scsi->log,		NULL);
}
#else
static inline void idescsi_add_settings(ide_drive_t *drive) { ; }
#endif

/*
 *	Driver initialization.
 */
static void idescsi_setup (ide_drive_t *drive, idescsi_scsi_t *scsi)
{
	if (drive->id && (drive->id->config & 0x0060) == 0x20)
		set_bit (IDESCSI_DRQ_INTERRUPT, &scsi->flags);
	clear_bit(IDESCSI_SG_TRANSFORM, &scsi->transform);
#if IDESCSI_DEBUG_LOG
	set_bit(IDESCSI_LOG_CMD, &scsi->log);
#endif /* IDESCSI_DEBUG_LOG */
	idescsi_add_settings(drive);
}

static void ide_scsi_remove(ide_drive_t *drive)
{
	struct Scsi_Host *scsihost = drive->driver_data;
	struct ide_scsi_obj *scsi = scsihost_to_idescsi(scsihost);
	struct gendisk *g = scsi->disk;

	scsi_remove_host(scsihost);
	ide_proc_unregister_driver(drive, scsi->driver);

	ide_unregister_region(g);

	drive->driver_data = NULL;
	g->private_data = NULL;
	put_disk(g);

	ide_scsi_put(scsi);
}

static int ide_scsi_probe(ide_drive_t *);

#ifdef CONFIG_IDE_PROC_FS
static ide_proc_entry_t idescsi_proc[] = {
	{ "capacity", S_IFREG|S_IRUGO, proc_ide_read_capacity, NULL },
	{ NULL, 0, NULL, NULL }
};
#endif

static ide_driver_t idescsi_driver = {
	.gen_driver = {
		.owner		= THIS_MODULE,
		.name		= "ide-scsi",
		.bus		= &ide_bus_type,
	},
	.probe			= ide_scsi_probe,
	.remove			= ide_scsi_remove,
	.version		= IDESCSI_VERSION,
	.media			= ide_scsi,
	.supports_dsc_overlap	= 0,
	.do_request		= idescsi_do_request,
	.end_request		= idescsi_end_request,
	.error                  = idescsi_atapi_error,
	.abort                  = idescsi_atapi_abort,
#ifdef CONFIG_IDE_PROC_FS
	.proc			= idescsi_proc,
#endif
};

static int idescsi_ide_open(struct inode *inode, struct file *filp)
{
	struct gendisk *disk = inode->i_bdev->bd_disk;
	struct ide_scsi_obj *scsi;

	if (!(scsi = ide_scsi_get(disk)))
		return -ENXIO;

	return 0;
}

static int idescsi_ide_release(struct inode *inode, struct file *filp)
{
	struct gendisk *disk = inode->i_bdev->bd_disk;
	struct ide_scsi_obj *scsi = ide_scsi_g(disk);

	ide_scsi_put(scsi);

	return 0;
}

static int idescsi_ide_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	struct block_device *bdev = inode->i_bdev;
	struct ide_scsi_obj *scsi = ide_scsi_g(bdev->bd_disk);
	return generic_ide_ioctl(scsi->drive, file, bdev, cmd, arg);
}

static struct block_device_operations idescsi_ops = {
	.owner		= THIS_MODULE,
	.open		= idescsi_ide_open,
	.release	= idescsi_ide_release,
	.ioctl		= idescsi_ide_ioctl,
};

static int idescsi_slave_configure(struct scsi_device * sdp)
{
	/* Configure detected device */
	sdp->use_10_for_rw = 1;
	sdp->use_10_for_ms = 1;
	scsi_adjust_queue_depth(sdp, MSG_SIMPLE_TAG, sdp->host->cmd_per_lun);
	return 0;
}

static const char *idescsi_info (struct Scsi_Host *host)
{
	return "SCSI host adapter emulation for IDE ATAPI devices";
}

static int idescsi_ioctl (struct scsi_device *dev, int cmd, void __user *arg)
{
	idescsi_scsi_t *scsi = scsihost_to_idescsi(dev->host);

	if (cmd == SG_SET_TRANSFORM) {
		if (arg)
			set_bit(IDESCSI_SG_TRANSFORM, &scsi->transform);
		else
			clear_bit(IDESCSI_SG_TRANSFORM, &scsi->transform);
		return 0;
	} else if (cmd == SG_GET_TRANSFORM)
		return put_user(test_bit(IDESCSI_SG_TRANSFORM, &scsi->transform), (int __user *) arg);
	return -EINVAL;
}

static int idescsi_queue (struct scsi_cmnd *cmd,
		void (*done)(struct scsi_cmnd *))
{
	struct Scsi_Host *host = cmd->device->host;
	idescsi_scsi_t *scsi = scsihost_to_idescsi(host);
	ide_drive_t *drive = scsi->drive;
	struct request *rq = NULL;
	idescsi_pc_t *pc = NULL;

	if (!drive) {
		scmd_printk (KERN_ERR, cmd, "drive not present\n");
		goto abort;
	}
	scsi = drive_to_idescsi(drive);
	pc = kmalloc (sizeof (idescsi_pc_t), GFP_ATOMIC);
	rq = kmalloc (sizeof (struct request), GFP_ATOMIC);
	if (rq == NULL || pc == NULL) {
		printk (KERN_ERR "ide-scsi: %s: out of memory\n", drive->name);
		goto abort;
	}

	memset (pc->c, 0, 12);
	pc->flags = 0;
	pc->rq = rq;
	memcpy (pc->c, cmd->cmnd, cmd->cmd_len);
	pc->buffer = NULL;
	pc->sg = scsi_sglist(cmd);
	pc->sg_cnt = scsi_sg_count(cmd);
	pc->b_count = 0;
	pc->request_transfer = pc->buffer_size = scsi_bufflen(cmd);
	pc->scsi_cmd = cmd;
	pc->done = done;
	pc->timeout = jiffies + cmd->timeout_per_command;

	if (test_bit(IDESCSI_LOG_CMD, &scsi->log)) {
		printk ("ide-scsi: %s: que %lu, cmd = ", drive->name, cmd->serial_number);
		ide_scsi_hex_dump(cmd->cmnd, cmd->cmd_len);
		if (memcmp(pc->c, cmd->cmnd, cmd->cmd_len)) {
			printk ("ide-scsi: %s: que %lu, tsl = ", drive->name, cmd->serial_number);
			ide_scsi_hex_dump(pc->c, 12);
		}
	}

	ide_init_drive_cmd (rq);
	rq->special = (char *) pc;
	rq->cmd_type = REQ_TYPE_SPECIAL;
	spin_unlock_irq(host->host_lock);
	rq->rq_disk = scsi->disk;
	(void) ide_do_drive_cmd (drive, rq, ide_end);
	spin_lock_irq(host->host_lock);
	return 0;
abort:
	kfree (pc);
	kfree (rq);
	cmd->result = DID_ERROR << 16;
	done(cmd);
	return 0;
}

static int idescsi_eh_abort (struct scsi_cmnd *cmd)
{
	idescsi_scsi_t *scsi  = scsihost_to_idescsi(cmd->device->host);
	ide_drive_t    *drive = scsi->drive;
	int		busy;
	int             ret   = FAILED;

	/* In idescsi_eh_abort we try to gently pry our command from the ide subsystem */

	if (test_bit(IDESCSI_LOG_CMD, &scsi->log))
		printk (KERN_WARNING "ide-scsi: abort called for %lu\n", cmd->serial_number);

	if (!drive) {
		printk (KERN_WARNING "ide-scsi: Drive not set in idescsi_eh_abort\n");
		WARN_ON(1);
		goto no_drive;
	}

	/* First give it some more time, how much is "right" is hard to say :-( */

	busy = ide_wait_not_busy(HWIF(drive), 100);	/* FIXME - uses mdelay which causes latency? */
	if (test_bit(IDESCSI_LOG_CMD, &scsi->log))
		printk (KERN_WARNING "ide-scsi: drive did%s become ready\n", busy?" not":"");

	spin_lock_irq(&ide_lock);

	/* If there is no pc running we're done (our interrupt took care of it) */
	if (!scsi->pc) {
		ret = SUCCESS;
		goto ide_unlock;
	}

	/* It's somewhere in flight. Does ide subsystem agree? */
	if (scsi->pc->scsi_cmd->serial_number == cmd->serial_number && !busy &&
	    elv_queue_empty(drive->queue) && HWGROUP(drive)->rq != scsi->pc->rq) {
		/*
		 * FIXME - not sure this condition can ever occur
		 */
		printk (KERN_ERR "ide-scsi: cmd aborted!\n");

		if (blk_sense_request(scsi->pc->rq))
			kfree(scsi->pc->buffer);
		kfree(scsi->pc->rq);
		kfree(scsi->pc);
		scsi->pc = NULL;

		ret = SUCCESS;
	}

ide_unlock:
	spin_unlock_irq(&ide_lock);
no_drive:
	if (test_bit(IDESCSI_LOG_CMD, &scsi->log))
		printk (KERN_WARNING "ide-scsi: abort returns %s\n", ret == SUCCESS?"success":"failed");

	return ret;
}

static int idescsi_eh_reset (struct scsi_cmnd *cmd)
{
	struct request *req;
	idescsi_scsi_t *scsi  = scsihost_to_idescsi(cmd->device->host);
	ide_drive_t    *drive = scsi->drive;
	int             ready = 0;
	int             ret   = SUCCESS;

	/* In idescsi_eh_reset we forcefully remove the command from the ide subsystem and reset the device. */

	if (test_bit(IDESCSI_LOG_CMD, &scsi->log))
		printk (KERN_WARNING "ide-scsi: reset called for %lu\n", cmd->serial_number);

	if (!drive) {
		printk (KERN_WARNING "ide-scsi: Drive not set in idescsi_eh_reset\n");
		WARN_ON(1);
		return FAILED;
	}

	spin_lock_irq(cmd->device->host->host_lock);
	spin_lock(&ide_lock);

	if (!scsi->pc || (req = scsi->pc->rq) != HWGROUP(drive)->rq || !HWGROUP(drive)->handler) {
		printk (KERN_WARNING "ide-scsi: No active request in idescsi_eh_reset\n");
		spin_unlock(&ide_lock);
		spin_unlock_irq(cmd->device->host->host_lock);
		return FAILED;
	}

	/* kill current request */
	if (__blk_end_request(req, -EIO, 0))
		BUG();
	if (blk_sense_request(req))
		kfree(scsi->pc->buffer);
	kfree(scsi->pc);
	scsi->pc = NULL;
	kfree(req);

	/* now nuke the drive queue */
	while ((req = elv_next_request(drive->queue))) {
		if (__blk_end_request(req, -EIO, 0))
			BUG();
	}

	HWGROUP(drive)->rq = NULL;
	HWGROUP(drive)->handler = NULL;
	HWGROUP(drive)->busy = 1;		/* will set this to zero when ide reset finished */
	spin_unlock(&ide_lock);

	ide_do_reset(drive);

	/* ide_do_reset starts a polling handler which restarts itself every 50ms until the reset finishes */

	do {
		spin_unlock_irq(cmd->device->host->host_lock);
		msleep(50);
		spin_lock_irq(cmd->device->host->host_lock);
	} while ( HWGROUP(drive)->handler );

	ready = drive_is_ready(drive);
	HWGROUP(drive)->busy--;
	if (!ready) {
		printk (KERN_ERR "ide-scsi: reset failed!\n");
		ret = FAILED;
	}

	spin_unlock_irq(cmd->device->host->host_lock);
	return ret;
}

static int idescsi_bios(struct scsi_device *sdev, struct block_device *bdev,
		sector_t capacity, int *parm)
{
	idescsi_scsi_t *idescsi = scsihost_to_idescsi(sdev->host);
	ide_drive_t *drive = idescsi->drive;

	if (drive->bios_cyl && drive->bios_head && drive->bios_sect) {
		parm[0] = drive->bios_head;
		parm[1] = drive->bios_sect;
		parm[2] = drive->bios_cyl;
	}
	return 0;
}

static struct scsi_host_template idescsi_template = {
	.module			= THIS_MODULE,
	.name			= "idescsi",
	.info			= idescsi_info,
	.slave_configure        = idescsi_slave_configure,
	.ioctl			= idescsi_ioctl,
	.queuecommand		= idescsi_queue,
	.eh_abort_handler	= idescsi_eh_abort,
	.eh_host_reset_handler  = idescsi_eh_reset,
	.bios_param		= idescsi_bios,
	.can_queue		= 40,
	.this_id		= -1,
	.sg_tablesize		= 256,
	.cmd_per_lun		= 5,
	.max_sectors		= 128,
	.use_clustering		= DISABLE_CLUSTERING,
	.emulated		= 1,
	.proc_name		= "ide-scsi",
};

static int ide_scsi_probe(ide_drive_t *drive)
{
	idescsi_scsi_t *idescsi;
	struct Scsi_Host *host;
	struct gendisk *g;
	static int warned;
	int err = -ENOMEM;

	if (!warned && drive->media == ide_cdrom) {
		printk(KERN_WARNING "ide-scsi is deprecated for cd burning! Use ide-cd and give dev=/dev/hdX as device\n");
		warned = 1;
	}

	if (idescsi_nocd && drive->media == ide_cdrom)
		return -ENODEV;

	if (!strstr("ide-scsi", drive->driver_req) ||
	    !drive->present ||
	    drive->media == ide_disk ||
	    !(host = scsi_host_alloc(&idescsi_template,sizeof(idescsi_scsi_t))))
		return -ENODEV;

	g = alloc_disk(1 << PARTN_BITS);
	if (!g)
		goto out_host_put;

	ide_init_disk(g, drive);

	host->max_id = 1;

#if IDESCSI_DEBUG_LOG
	if (drive->id->last_lun)
		printk(KERN_NOTICE "%s: id->last_lun=%u\n", drive->name, drive->id->last_lun);
#endif
	if ((drive->id->last_lun & 0x7) != 7)
		host->max_lun = (drive->id->last_lun & 0x7) + 1;
	else
		host->max_lun = 1;

	drive->driver_data = host;
	idescsi = scsihost_to_idescsi(host);
	idescsi->drive = drive;
	idescsi->driver = &idescsi_driver;
	idescsi->host = host;
	idescsi->disk = g;
	g->private_data = &idescsi->driver;
	ide_proc_register_driver(drive, &idescsi_driver);
	err = 0;
	idescsi_setup(drive, idescsi);
	g->fops = &idescsi_ops;
	ide_register_region(g);
	err = scsi_add_host(host, &drive->gendev);
	if (!err) {
		scsi_scan_host(host);
		return 0;
	}
	/* fall through on error */
	ide_unregister_region(g);
	ide_proc_unregister_driver(drive, &idescsi_driver);

	put_disk(g);
out_host_put:
	scsi_host_put(host);
	return err;
}

static int __init init_idescsi_module(void)
{
	return driver_register(&idescsi_driver.gen_driver);
}

static void __exit exit_idescsi_module(void)
{
	driver_unregister(&idescsi_driver.gen_driver);
}

module_param(idescsi_nocd, int, 0600);
MODULE_PARM_DESC(idescsi_nocd, "Disable handling of CD-ROMs so they may be driven by ide-cd");
module_init(init_idescsi_module);
module_exit(exit_idescsi_module);
MODULE_LICENSE("GPL");
