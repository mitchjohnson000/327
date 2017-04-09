/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   pc-cpp.h
 * Author: mdj
 *
 * Created on March 22, 2017, 11:36 PM
 */

#ifndef PC_CPP_H
#define PC_CPP_H

# include <stdint.h>

# include "dims.h"

typedef struct dungeon dungeon_t;

 class pc:public character{
     void character_delete();
     void character_next_pos(dungeon_t *d);
     
};

uint32_t pc_is_alive(dungeon_t *d);
void config_pc(dungeon_t *d);
void place_pc(dungeon_t *d);
uint32_t pc_in_room(dungeon_t *d, uint32_t room);



#endif /* PC_CPP_H */

