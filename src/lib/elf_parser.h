//
// Copyright 2007 - 2009, Stanislav Karchebnyy <berkus+metta@madfire.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "elf.h"
#include "types.h"

/**
 * Parses an ELF file, generating symbolic information and loading code/data
 * segments.
 */
class elf_parser
{
public:
    /**
     * Creates a blank ELF parser, preparing for a call to loadKernel.
     */
    elf_parser();

//     ~elf_parser();

    /**
     * Loads the image file from specified memory location.
     */
    bool load_image(address_t start, size_t size);

    /**
     * Returns the symbol name for an address. Also returns the start address
     * of that symbol in startAddr if startAddr != NULL.
     */
    char* find_symbol(address_t addr, address_t *symbol_start = NULL);

    /**
     * Returns the address of a symbol with name str.
     */
    address_t find_symbol(char* str);

    /**
     * Returns the address of the symbol with offset o in the
     * relocation symbol table.
     */
    address_t find_dynamic_symbol_location(address_t o);

    /**
     * Returns a NULL terminated string specifying the name of
     * the symbol at offset o in the relocation symbol table.
     */
    char *find_dynamic_symbol_name(address_t o);

    /**
     * Gets the address of the global offset table.
     */
    address_t get_global_offset_table();

    /**
     * Returns the entry point of the executable.
     */
    address_t get_entry_point()
    {
        return (address_t)header_->entry;
    }

    /**
     * Returns last location occupied by the elf image in memory.
     */
    address_t get_alloc_end();

    /**
     * Returns true if the parser loaded correctly.
     */
    bool is_valid()
    {
        return (filename != 0);
    }

private:
    elf32::header*         header_;
    elf32::section_header* symbol_table;
    elf32::section_header* string_table;
    elf32::section_header* got_table; ///< Global offset table.
    elf32::section_header* rel_table; ///< Relocations table.
    elf32::section_header* section_headers; ///< Array of all section headers.
    const char*            filename;
};

// kate: indent-width 4; replace-tabs on;
// vim: set et sw=4 ts=4 sts=4 cino=(4 :