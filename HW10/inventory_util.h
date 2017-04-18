/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   inventory_util.h
 * Author: mdj
 *
 * Created on April 12, 2017, 12:40 PM
 */

#ifndef INVENTORY_UTIL_H
#define INVENTORY_UTIL_H
#include "dungeon.h"

void wear_item(dungeon_t *d);
void print_inventory(dungeon_t *d);
void print_equipment(dungeon_t *d);
void takeoff_item(dungeon_t *d);
void drop_item(dungeon_t *d);
void expunge_item(dungeon_t *d);
void list_inventory(dungeon_t *d);
void list_equipment(dungeon_t *d);
void inspect_item(dungeon_t *d);



#endif /* INVENTORY_UTIL_H */

