//
// Copyright 2007 - 2008, Stanislav Karchebnyy <berkus+metta@madfire.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "object.h"

/**
* A mapping makes part or all of a region accessible in a task at a
* selected address.  Mappings are the mechanism by which tasks populate
* their own or other task's address spaces.
*
* Given a region reference object, a task may map all or part of the
* region at a given address with specified access permissions.
**/
class mapping : public object
{
private:
    link          task_ref; /** link form of task reference */
    addrress_t    region_off; /** offset into the region */
    address_t     start_va; /** VA at which the region is mapped */
    size_t        size; /** size of mapping */
    protection_t  prot; /** protection attributes granted */
};

// kate: indent-width 4; replace-tabs on;
// vi:set ts=4:set expandtab=on:
