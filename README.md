# Palmtrie: A Ternary Key Matching Algorithm for IP Packet Filtering Rules

## About

This is the reference implementation of "Palmtrie: A Ternary Key Matching
Algorithm for IP Packet Filtering Rules," to appear in CoNEXT 2020.


## Author

Hirochika Asai


## License

The use of this software is limited to education, research, and evaluation
purposes only.  Commercial use is strictly prohibited.  For all other uses,
contact the author(s).

## Getting started

To run fundamental test cases and microbenchmarks

    $ make test

## APIs

### Initialization

    NAME
         palmtrie_init -- initialize a palmtrie control data structure

    SYNOPSIS
         struct palmtrie *
         palmtrie_init(struct palmtrie *palmtrie, enum palmtrie_type type);

    DESCRIPTION
         The palmtrie_init() function initializes a palmtrie control data
         structure specified by the palmtrie argument with a type parameter.

         If the palmtrie argument is NULL, a new data structure is allocated and
         initialized.  Otherwise, the data structure specified by the palmtrie
         argument is initialized.

         The type parameter specifies the type of the palmtrie.  The valid type
         are the followings:

         o PALMTRIE_SORTED_LIST: A reference type to implement ternary matching
           using a sorted list, not Palmtrie.

         o PALMTRIE_BASIC: A type to represent Palmtrie (basic) that implements
           a single-bit stride trie.

         o PALMTRIE_DEFAULT: A type to represent Palmtrie that implements
           a multibit stride extension.

         o PALMTRIE_PLUS: A type to represent Palmtrie+ that implements an
           optimization technique using the population count instruction.
           The Palmtrie data structure requires to call the palmtrie_commit()
           function after updating the data structure.

    RETURN VALUES
         Upon successful completion, the palmtrie_init() function returns the
         pointer to the initialized palmtrie data structure.  Otherwise, it
         returns a NULL value and set errno.  If a non-NULL poptrie argument is
         specified, the returned value shall be the original value of the
         poptrie argument if successful, or a NULL value otherwise.

### Addition

    NAME
         palmtrie_add_data -- add an entry to the palmtrie data structure

    SYNOPSIS
         int
         palmtrie_add_data(struct palmtrie *palmtrie, addr_t addr, addr_t mask,
                           int priority, uint64_t data);

    DESCRIPTION
         The palmtrie_add_data() function adds an entry with a ternary key data
         specified by a pair of addr and mask arguments, the priority, and the
         data into the trie specified by the palmtrie argument.


    RETURN VALUES
         On successful, the palmtrie_add_data() function returns a value of 0.
         Otherwise, they return a value of -1.

### Lookup

    NAME
         palmtrie_lookup -- obtain a 64-bit data by looking up the entry
         corresponding the specified key from the palmtrie data structure

    SYNOPSIS
         uint64_t
         palmtrie_lookup(struct palmtrie *palmtrie, addr_t addr);

    DESCRIPTION
         The palmtrie_lookup() function looks up an entry corresponding to the
         key specified by the addr argument.

    RETURN VALUES
         The palmtrie_lookup() function returns a 64-bit data corresponding to
         the addr argument.  If no matching entry is found, a zero value is
         returned.

