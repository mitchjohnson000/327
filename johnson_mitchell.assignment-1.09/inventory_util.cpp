#include "dungeon.h"
#include "pc.h"
#include "object.h"
#include <ncurses.h>

void print_inventory(dungeon_t *d){
    printw("Inventory: \n");
    if(d->PC->inventory.size() <= 0){
        printw("1) Nothing \n");
    }else{
        printw("1) %s \n",d->PC->inventory.at(0)->get_name());
    }
    if(d->PC->inventory.size() <= 1){
        printw("2) Nothing \n");
    }else{
        printw("2) %s \n",d->PC->inventory.at(1)->get_name());
    }    
    if(d->PC->inventory.size() <= 2){
        printw("3) Nothing \n");
    }else{
        printw("3) %s \n",d->PC->inventory.at(2)->get_name());
    }    
    if(d->PC->inventory.size() <= 3){
        printw("4) Nothing \n");
    }else{
        printw("4) %s \n",d->PC->inventory.at(3)->get_name());
    }    
    if(d->PC->inventory.size() <= 4){
        printw("5) Nothing \n");
    }else{
        printw("5) %s \n",d->PC->inventory.at(4)->get_name());
    }    
    if(d->PC->inventory.size() <= 5){
        printw("6) Nothing \n");
    }else{
        printw("6) %s \n",d->PC->inventory.at(5)->get_name());
    }   
    if(d->PC->inventory.size() <= 6){
        printw("7) Nothing \n");
    }else{
        printw("7) %s \n",d->PC->inventory.at(6)->get_name());
    }
    if(d->PC->inventory.size() <= 7){
        printw("8) Nothing \n");
    }else{
        printw("8) %s \n",d->PC->inventory.at(7)->get_name());
    }
    if(d->PC->inventory.size() <= 8){
        printw("9) Nothing \n");
    }else{
        printw("89 %s \n",d->PC->inventory.at(8)->get_name());
    }
    if(d->PC->inventory.size() <= 9){
        printw("10) Nothing \n");
    }else{
        printw("10) %s \n",d->PC->inventory.at(9)->get_name());
    }
    
}

void print_equipment(dungeon_t *d){
    
    printw("Equipment: \n");
    if(d->PC->equipment.size() <= 0){
        printw("1) Nothing \n");
    }else{
        printw("1) %s \n",d->PC->equipment.at(0)->get_name());
    }
    if(d->PC->equipment.size() <= 1){
        printw("2) Nothing \n");
    }else{
        printw("2) %s \n",d->PC->equipment.at(1)->get_name());
    }    
    if(d->PC->equipment.size() <= 2){
        printw("3) Nothing \n");
    }else{
        printw("3) %s \n",d->PC->equipment.at(2)->get_name());
    }    
    if(d->PC->equipment.size() <= 3){
        printw("4) Nothing \n");
    }else{
        printw("4) %s \n",d->PC->equipment.at(3)->get_name());
    }    
    if(d->PC->equipment.size() <= 4){
        printw("5) Nothing \n");
    }else{
        printw("5) %s \n",d->PC->equipment.at(4)->get_name());
    }    
    if(d->PC->equipment.size() <= 5){
        printw("6) Nothing \n");
    }else{
        printw("6) %s \n",d->PC->equipment.at(5)->get_name());
    }   
    if(d->PC->equipment.size() <= 6){
        printw("7) Nothing \n");
    }else{
        printw("7) %s \n",d->PC->equipment.at(6)->get_name());
    }
    if(d->PC->equipment.size() <= 7){
        printw("8) Nothing \n");
    }else{
        printw("8) %s \n",d->PC->equipment.at(7)->get_name());
    }
    if(d->PC->equipment.size() <= 8){
        printw("9) Nothing \n");
    }else{
        printw("9 %s \n",d->PC->equipment.at(8)->get_name());
    }
    if(d->PC->equipment.size() <= 9){
        printw("10) Nothing \n");
    }else{
        printw("10) %s \n",d->PC->equipment.at(9)->get_name());
    }
   
}

void wear_item(dungeon_t *d){
    clear();
    printw("Please select a slot from your inventory \n");
    print_inventory(d);
    char temp = getch();
    if(temp == 27) {clear(); return;}
    int slot = (int)temp - 48;
    slot += -1;
    if(d->PC->inventory.size() < slot){
        printw("empty slot!");
        clear();
    }else{
        object *obj = d->PC->inventory.at(slot);
        for(int i = 0;i<d->PC->equipment.size();i++){
            if(obj->get_type() == d->PC->equipment.at(i)->get_type()){
                object * temp = d->PC->equipment.at(i);
                d->PC->equipment.assign(i,d->PC->inventory.at(slot));
                d->PC->inventory.assign(slot,temp);
                d->PC->speed = d->PC->speed + d->PC->inventory.at(slot)->get_speed();
                return; 
            }
        }
        d->PC->equipment.push_back(d->PC->inventory.at(slot));
        d->PC->speed = d->PC->speed + d->PC->inventory.at(slot)->get_speed();
        d->PC->inventory.erase(d->PC->inventory.begin() + slot);
        clear();
    }
}

void takeoff_item(dungeon_t *d){
    clear();
    printw("Please select a slot from your equipment \n");
    print_equipment(d);
    char temp = getch();
    if(temp == 27) {clear(); return;}
    int slot = (int)temp - 48;
    slot += -1;
    if(d->PC->equipment.size() < slot){
        printw("empty slot!");
        clear();
    }else{
        if(d->PC->inventory.size() < 10){
            d->PC->inventory.push_back(d->PC->equipment.at(slot));
            d->PC->speed = d->PC->speed - d->PC->equipment.at(slot)->get_speed();
            d->PC->equipment.erase(d->PC->equipment.begin() + slot);
        }else{
            printw("Inventory is full! \n");
                    while(getch() != 27){
            }
        }
        clear();
    }    
}

void drop_item(dungeon_t *d){
    clear();
    printw("Please select a slot from your inventory \n");
    print_inventory(d);
    char temp = getch();
    if(temp == 27) {clear(); return;}

    int slot = (int)temp - 48;
    slot += -1;
    if(d->PC->inventory.size() < slot){
        printw("empty slot!");
        clear();
    }else{
        d->objmap[d->PC->position[dim_y]][d->PC->position[dim_x]] = d->PC->inventory.at(slot);
        d->PC->inventory.erase(d->PC->inventory.begin() + slot);
        clear();
    }
}

void expunge_item(dungeon_t *d){
    clear();
    printw("Please select a slot from your inventory \n");
    print_inventory(d);
    char temp = getch();
    if(temp == 27) {clear(); return;}
    int slot = (int)temp - 48;
    slot += -1;
    if(d->PC->inventory.size() < slot){
        printw("empty slot!");
        clear();
    }else{
        d->PC->inventory.erase(d->PC->inventory.begin() + slot);
        clear();
    }    
}

void list_inventory(dungeon_t *d){
    clear();
    print_inventory(d);
    printw("\n Press any key to go back \n");
    getch();
    clear();
}

void list_equipment(dungeon_t *d){
    clear();
    print_equipment(d);
    printw("\n Press any key to go back \n");
    getch();
    clear();
}

void inspect_item(dungeon_t *d){
    clear();
    printw("Select an item to inspect \n");
    print_inventory(d);
    char temp = getch();
    if(temp == 27) {clear(); return;}
    int slot = (int)temp - 48;
    slot += -1;
    if(d->PC->inventory.size() < slot){
        printw("empty slot!");
        clear();
    }else{
        clear();
        printw("%s \n",d->PC->inventory.at(slot)->get_description());
        printw("\n Press any key to go back \n");
        getch();
        clear();
     }
    
    
}