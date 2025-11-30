#pragma once
#ifndef KERNEL_ISR_H
#define KERNEL_ISR_H

#include_next <isr.h>

static void irq_disable();
static void irq_enable();

static uint64_t irq_save();
static void irq_restore(uint64_t flags);

#endif
