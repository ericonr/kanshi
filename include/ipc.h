#ifndef KANSHI_IPC_H
#define KANSHI_IPC_H

#include <stdio.h>
#include <stdlib.h>

#include "kanshi.h"

int kanshi_init_ipc(struct kanshi_state *state);
void kanshi_free_ipc(struct kanshi_state *state);

int check_env(void);
int get_ipc_address(char *address, size_t size);

#endif
