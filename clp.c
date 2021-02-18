#include <stdbool.h>
#include "pstrings.h"
#include <stdio.h>
#include <stdlib.h>
#include "clp.h"


/* The Parser consists of three distinct components/ aspects. 
   1) the argument registry. Command line arguments are registered
   and various parameters pertaining to it are specified.

   2) the command-line arguments are parsed from argc, and checked against the registry
   for consistency validation. 

   3) If there's an inconsistency, the program exits with an error. If not, the result of the previous step
   is a stack containing all the options that have been typed at the command line, with own parameters and whatnot.

   This stack can then be popped iteratively, and actions can be taken according to the arguments used. 
*/


/* ____________________________________________________________________________________________--
   ||||||||||||||||||||||||||||||| REGISTRY |||||||||||||||||||||||||||||||||||||||||||||||||||
   -------------------------------------------------------------------------------------------
*/


static struct cl_arg *RT_insert(struct cl_arg *arg_tree, struct cl_arg *argument){
    /* insert argument as a tree node into arg_tree */
    if (!arg_tree){
        return argument;
    }
    
    register char *arg_name = (argument->short_name) ? argument->short_name : argument->long_name;
    char *comparand = (arg_tree->short_name) ? arg_tree->short_name : arg_tree->long_name;

    unsigned short comparison_res = str_compare(arg_name, comparand);
    // if the arg_name < the name of the command in the current node, go left
    if (comparison_res == 2){
        RT_insert(arg_tree->left, argument);
    }
    // else if > the name of the command in hte current node, go right
    else if (comparison_res == 0){
        RT_insert(arg_tree->right, argument);
    }
    // else, they're identical. Disregard
    return arg_tree;
}

static struct cl_arg *RT_search_short(char parsed[], struct cl_arg *tree){
    /* Traverse the tree and look for a short-name argument that matches parsed */
    if (!tree){
        return NULL;
    }

    else{
        unsigned short comparison_res = str_compare(parsed, tree->short_name);

        if (comparison_res == 1){
            return tree;
        }

        else if (comparison_res == 0){
            return RT_search_short(parsed, tree->right);
            }

        else{
            return RT_search_short(parsed, tree->left);
        }
    }
}


static struct cl_arg *RT_search_long(char parsed[], struct cl_arg *tree){
    /* Traverse the tree and look for a long-name argument that matches parsed */
    if (!tree){
        return NULL;
    }

    else{
        unsigned short comparison_res = str_compare(parsed, tree->long_name);

        if (comparison_res == 1){
            return tree;
        }

        else if (comparison_res == 0){
            return RT_search_short(parsed, tree->right);
            }

        else{
            return RT_search_short(parsed, tree->left);
        }
    }
}



static struct cl_arg *Clp_registry_check(RegisTree registry, char parsed[], enum arg_name_type TYPE){
    /* Look for parsed in the registry. 
       If found, return a pointer to its entry there. 
       Else, return NULL.
    */
    if (!registry){
        return NULL;
    }

    struct cl_arg *found;
    
    if (TYPE == SHORT){
        found = RT_search_short(parsed, registry->root);
    }
    else{
        found = RT_search_long(parsed,registry->root);
    }

    return found;
}



void Clp_init(RegisTree *registry_tree_ref){
/* Allocate memory for a registry tree */
    RegisTree tmp = malloc(sizeof(struct registry_tree));
    if (!tmp){
        *registry_tree_ref = NULL;
        return;
    }

    *registry_tree_ref = tmp;
    (*registry_tree_ref)->args_count = 0;
    (*registry_tree_ref)->root = NULL;
}


void Clp_register_arg(RegisTree registry, \
                      char short_name[], \
                      char long_name[], \
                      unsigned short num_args,\
                      bool required, \
                      bool positional_arg, \
                      char help_string[] \
        ){
    // allocate space for a new cl_arg struct
    struct cl_arg *new = malloc(sizeof(struct cl_arg));
    if (!new){
        return;
    }
    // initialize new according to the supplied function arguments
    new->short_name = &short_name[0];
    new->long_name = long_name;
    new->num_args = num_args;
    new->required = required;
    new->help_string = help_string;
    

    // add it to the registry
    registry->root = RT_insert(registry->root, new);
    registry->args_count++;
}



/* ____________________________________________________________________________________________
   ||||||||||||||||||||||||||||||| PARSER  |||||||||||||||||||||||||||||||||||||||||||||||||||
   -------------------------------------------------------------------------------------------
*/

static enum arg_name_type Clp_get_arg_type(char *arg){
    enum arg_name_type type;

   // determine whether it's a short, long, or positional argument
        if (arg[0] == '-' && arg[1] != '-'){
            // short argument
            type = SHORT;
        } 
        else if (arg[0] == '-' && arg[1] == '-'){
            // long argument
            type = LONG;
        }
        else{
            type = POSITIONAL;
        }
    return type;
}



Stack Clp_parse(RegisTree registry, int argc, char *argv[]){
    /* Parse argc by splitting it in tokens and retrieving each short, long and positional
     * argument from it.
     * Then match this against the arguments registered in the registry. If the argument
     * is unknown, do nothing. Else if the argument is found there, add it to the 'parsed'
     * stack, after filling the 'args' field of the argument struct with the arguments
     * parsed from args, if any.
     */
    // initialize a stack to push the parsed arguments on 
    Stack parsed;
    Stack_init(&parsed);

    enum arg_name_type type;
    // get each string (argument) from argv
    // the first argument is the namne of the program
    for (unsigned int i = 1; i < argc; i++){
        char *argument = argv[i];

        type = Clp_get_arg_type(argument);
        
        // check that argument is an expected, defined, command, registered in the
        // registry tree
        if (type == LONG){
            argument += 2;  // discard the '--'
            argument = str_tokenize(argument, '=');
        }
        else if (type == SHORT){
            argument++; // discard the '-'
        }
        struct cl_arg *reg_arg_struct = Clp_registry_check(registry, argument, type);
        if (!reg_arg_struct){
            // unknown argument
            puts("Unknown argument used.");
            exit(EXIT_FAILURE);
        }
        //else, the argument was found and a pointer to it retrieved, stored in
        //reg_arg_struct

        // if it expects arguments, retrieve them and store pointers to them in its args
        // inner field; first allocate memory for these pointers;
        if (reg_arg_struct->num_args > 0){
            char **args = malloc(sizeof(char *) *reg_arg_struct->num_args);
            reg_arg_struct->args = args;
            

            // if it's a long argument, it can only take one argument of its own, and it's
            // part of it, separated by the equal sign.
            if (type == LONG){
                char *arg = str_tokenize(NULL, ' ');
                reg_arg_struct->args[0] = arg;
            }
            else if (type == SHORT){
                // loop and get the as many arguments as it takes
                for (unsigned short n = 1; n<= reg_arg_struct->num_args; n++){
                    // i still points to the current argument, so look forward and get as
                    // many tokens as arg takes arguments of its own
                    char *arg = argv[i+n];
                    reg_arg_struct->args[n-1] = arg; 
                }
            }
        }
            // add the newly-parsed argument to the parsed stack
            StackItem parsed_arg = Stack_make_item(reg_arg_struct);
            Stack_push(parsed, parsed_arg);
    }
    return parsed;
}







