//
// Copyright 2007 - 2009, Stanislav Karchebnyy <berkus+metta@madfire.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "types.h"

/**
* Write a byte out to the specified port.
* a puts value in eax, dN puts port in edx or uses 1-byte constant.
**/
static inline void outb(uint16_t port, uint8_t value)
{
    asm volatile ("outb %0, %1" :: "a" (value), "dN" (port));
}

/// Write a word out to the specified port.
static inline void outw(uint16_t port, uint16_t value)
{
    asm volatile ("outw %0, %1" :: "a" (value), "dN" (port));
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t ret;
	asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

static inline uint16_t inw(uint16_t port)
{
	uint16_t ret;
	asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

static inline uint64_t rdtsc()
{
    uint32_t upper, lower;
    asm volatile("rdtsc" : "=a"(lower), "=d"(upper));
    return ((uint64_t)upper << 32) | lower;
}

static inline void enable_interrupts(void)
{
    asm volatile ("sti");
}

static inline void disable_interrupts(void)
{
    asm volatile ("cli");
}


// kate: indent-width 4; replace-tabs on;
// vim: set et sw=4 ts=4 sts=4 cino=(4 :
