//
// Copyright 2007 - 2009, Stanislav Karchebnyy <berkus+metta@madfire.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
/**
* Associative filesystem interface.
* Defines a namespaced set of file objects with arbitrary metadata attributes.
* There are no directories, directories can be emulated by any UI layer.
* Views or file groups are used instead on the fs level. Views define a filter
* that is used to pick out arbitrary file objects from the whole filesystem
* given a filtering expression.
**/
#include "com.h"

/**
* Whole filesystem interface.
**/
DECLARE_COMINTERFACE(com_ifilesystem)
{
    void mount();
};

struct com_ifilesystem_ops : public com_iunknown_ops
{
    OSKIT_COMDECL (*mount)(com_ifilesystem *obj);
};

inline void com_ifilesystem::mount() { ops->mount(this); }

/**
* Filesystem node interface.
* Defines file-related operations on a node.
**/
struct com_ifsnode
{
	/*open*/ /* return a fsobject (stream, char or block) interface for manipulating
	data contained in this node. */
	/*metadata*/ /* return a record interface for manipulating metadata records */
	// security interface?
};

/**
* Filesystem view (filter that selects only files corresponding to a given criterion).
**/
struct com_ifsview
{
	/*filter*/ /* manipulate filtering expression */
	/*iterator*/ /* enumerate all fsnodes matching current expression (present in view) */
};

// kate: indent-width 4; replace-tabs on;
// vim: set et sw=4 ts=4 sts=4 cino=(4 :