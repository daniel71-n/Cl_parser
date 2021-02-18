#include <stdbool.h>
#include "pstrings.h"
#include <stdio.h>
#include <stdlib.h>
#include "stack.h"


enum arg_name_type{SHORT, LONG, POSITIONAL};


struct cl_arg{
    char *short_name; // NULL if not not used, else a 1-char string
    char *long_name; // NULL if not used, else a longer string
    int num_args; // 0 if not arguments, else the number of arguments
    char **args;    // Mallocated with size==num_args; this should be mallocated NOT when the command is registered, 
    //but only if upon parsing the parser determines an argument was entered at the cli that matches this
    char *help_string; // the help string for the option, displayed when --help/-h is invoked
    bool required;  // whether or not this option is always required
    bool positional_arg;

    // tree-related pointers
    struct cl_arg *left;
    struct cl_arg *right;
};

typedef struct registry_tree{
    struct cl_arg *root;
    int args_count; // total number of registered arguments
}* RegisTree;   // pointer to a refistry_tree struct


void Clp_init(RegisTree *registry_tree_ref);


void Clp_register_arg(RegisTree registry, \
                      char short_name[], \
                      char long_name[], \
                      unsigned short num_args,\
                      bool required, \
                      bool positional_arg, \
                      char help_string[] \
        );


Stack Clp_parse(RegisTree registry, int argc, char *argv[]);


