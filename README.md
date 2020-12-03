# Palmtrie: A Ternary Key Matching Algorithm for IP Packet Filtering Rules

## About

This is the reference implementation of
["Palmtrie: A Ternary Key Matching Algorithm for IP Packet Filtering Rules"](https://dl.acm.org/doi/abs/10.1145/3386367.3431289)
presented in ACM CoNEXT 2020.


## Author

Hirochika Asai


## License

The use of this software is limited to education, research, and evaluation
purposes only.  Commercial use is strictly prohibited.  For all other uses,
contact the author(s).


## Prerequisite & tested platforms

The following tools are required to compile this software from a distribution
package.

* C compiler (tested on gcc 5.4.0)
* make (tested on GNU Make 4.1)

If you build this software from the repository, the following tools are also
required.

* automake (tested on GNU automake 1.15)
* autoconf (tested on GNU Autoconf 2.69)

This software is tested on the following platforms.

* Ubuntu 16.04.7 / Intel(R) Core(TM) i7-6700K
* macOS Mojave (10.14.6) / Intel(R) Core(TM) i9 2.9 GHz


## Getting started

This software uses automake/autoconf.  Run `./autogen.sh` to generate a
`configure` script.  After generating the `configure` script or if you unarchive
a source code archive with the `configure` script, run `./configure` and `make`
to compile the software.

### Programs and library

The `make` command produces  the following test programs and a Palmtrie library
(see APIs section below for the library interfaces).

* palmtrie_test_basic: Basic tests
* palmtrie_test_acl: Additional tests
* palmtrie_eval_lpm: Performance evaluation for longest prefix matching
* palmtrie_eval_acl: Performance evaluation for access control lists (ACLs)
* libpalmtrie.la: Library archive file implementing the Palmtrie algorithm


### Basic tests and microbenchmarking

To run basic tests and microbenchmarks, the `make test` command can be used.

    $ make test
    /usr/bin/make  all-recursive
    make[2]: Nothing to be done for `all-am'.
    Testing all...
    ./palmtrie_test_basic
    basic test (SORTED_LIST): passed
    basic test (BASIC): passed
    basic test (DEFAULT): passed
    basic test (PLUS): passed
    microbenchmarking (SORTED_LIST): ..............(26.088413 sec: 0.000 Mlookup/sec)passed
    microbenchmarking (BASIC): ..............(9.045084 sec: 1.855 Mlookup/sec)passed
    microbenchmarking (DEFAULT): ..............(4.566560 sec: 3.674 Mlookup/sec)passed
    microbenchmarking (PLUS): ..............(2.386559 sec: 7.030 Mlookup/sec)passed
    cross check (BASIC,DEFAULT): ....................................................................passed
    cross check (SORTED_LIST,BASIC): ....................................................................passed
    cross check for ACL (BASIC,DEFAULT): .................passed
    cross check for ACL reverse order scanning (BASIC,DEFAULT): ..........passed
    cross check for ACL reverse order scanning (BASIC,PLUS): ..........passed
    cross check for ACL (SORTED_LIST,BASIC): .................passed
    cross check for ACL reverse order scanning (SORTED_LIST,BASIC): ..........passed

### Evaluation programs

The compiled programs named `palmtrie_eval_lpm` and `palmtrie_eval_acl` run
performance evaluation of Palmtrie variants as well as the conventional sorted
list for longest prefix matching and ternary matching for ACLs, respectively.

The `ptcam_eval_lpm` program takes two arguments: 1) routing table and 2) type
of the data structure (see the initialization section below for the details);
sl) SORTED_LIST, tpt) BASIC, mtpt) DEFAULT, and popmtpt) PLUS.  A sammple
routing table is found at `tests/linx-rib.20141217.0000-p46.txt` in this source
code directory.

The `ptcam_eval_acl` program takes two arguments: 1) ternary matching table and
2) type of the data structure and traffic pattern.  The second argument can be
separated by - (dash) and its prefix and suffix represents the type of the data
structure and the traffic pattern, respectively.  The type is same as the
description for the `ptcam_eval_lpm` program above.  The traffic pattern are
either of rand) random source address, random destination address in the range
of 10.0.0.0/8, and random source and destination port numbers, sfl) traffic
pattern specified by the `tests/traffic.sfl2` file, ross) reverse-byte order
scanning.  Examples of ternary matching tables are found at
`tests/acl-0001.tcam` and `tests/acl-0002.tcam`.

The output of these evaluation programs include 30 lines of the lookup rate
samples.  Each sample measures the lookup rate for 10 seconds. The first column
represents the unix timestamp at the beginning of the sample.  The second and
the third columns represent the lookup count and the average lookup rate for
the duration, respectively.

The following toolset is used to convert an ACL ruleset to a ternary matching
table and generate a traffic pattern file: https://github.com/drpnd/acl

### Reproducing the evaluation results in the Palmtrie paper

To reproduce the evaluation results of Palmtrie^+_8 in the paper, the following
can be used.

    $ ./run_evaluation.sh

It creates a temporary directory to save the evaluation results.  The directory
is reported at the end of the output when the evaluation has finished.  The
`campus` directory stores the results for the synthetic campus network policy
ACLs.  The `campus/random` and `campus/scanning` directories store the results
for the uniform traffic and the reverse-byte order scanning, respectively.
The `result.acl-Dx.txt` files, where x is a decimal number in the range from 0
to 16, are the results for the dataset Dx described in Section 4.1 of the
paper.

The `classbench` directory stores the results for the ClassBench dataset.
The `result.<type>_seed_<size>.txt` files are the results for the datasets
generated by ClassBench.  The `<type>` parameter denotes the parameter file of
ClassBench and is either of `acl1`, `fw2`, or `ipc2` in the paper. The
`<size>` parameter specifies the number of entries specified to ClassBench;
`1k`, `10k`, `50k`, `100k`, `200k`, or `500k`.

Note that this evaluation script requires the following command:

* wget
* tar (with gzip support)
* gunzip


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

