/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   character-cpp.h
 * Author: mdj

 */

#ifndef CHARACTER_CPP_H
#define CHARACTER_CPP_H

# include <stdint.h>
# include "dims.h"

typedef struct dungeon dungeon_t;
typedef struct npc npc_t;
typedef struct pc pc_t;
typedef struct dice_t dice_t;

typedef enum kill_type {
  kill_direct,
  kill_avenged,
  num_kill_types
} kill_type_t;

 class character {
  char symbol;
  pair_t position;
  int32_t speed;
  uint32_t alive;
  /* Characters use to have a next_turn for the move queue.  Now that it is *
   * an event queue, there's no need for that here.  Instead it's in the    *
   * event.  Similarly, sequence_number was introduced in order to ensure   *
   * that the queue remains stable.  Also no longer necessary here, but in  *
   * this case, we'll keep it, because it provides a bit of interesting     *
   * metadata: locally, how old is this character; and globally, how many   *
   * characters have been created by the game.                              */
  uint32_t sequence_number;
  npc_t *npc;
  pc_t *pc;
  uint32_t kills[num_kill_types];
  
  int32_t compare_characters_by_next_turn(const void *character1,
                                        const void *character2);
  uint32_t can_see(dungeon_t *d, character_t *voyeur, character_t *exhibitionist);
  virtual void character_delete(){return 0;}
  virtual void character_next_pos(dungeon_t * d){return 0;}
  
};





#endif /* CHARACTER_CPP_H */

