#ifndef _MESH_H
#define _MESH_H

#define MAX_NODES 30

int mesh_nodes(void);
bool mesh_init(void);
void mesh_update(void);
void mesh_announce_buck(char * ch);
#endif