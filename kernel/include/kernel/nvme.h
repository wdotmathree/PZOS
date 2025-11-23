#pragma once
#ifndef KERNEL_NVME_H
#define KERNEL_NVME_H

#define NVME_CAP 0x00
#define NVME_VS 0x08
#define NVME_INTMS 0x0c
#define NVME_INTMC 0x10
#define NVME_CC 0x14
#define NVME_CSTS 0x1c
#define NVME_NSSR 0x20
#define NVME_AQA 0x28
#define NVME_ASQ 0x30
#define NVME_ACQ 0x38

void nvme_init(void);

#endif
