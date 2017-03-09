#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <curses.h>

#include "dungeon.h"
#include "pc.h"
#include "npc.h"
#include "move.h"

const char *victory =
  "\n                                       o\n"
  "                                      $\"\"$o\n"
  "                                     $\"  $$\n"
  "                                      $$$$\n"
  "                                      o \"$o\n"
  "                                     o\"  \"$\n"
  "                oo\"$$$\"  oo$\"$ooo   o$    \"$    ooo\"$oo  $$$\"o\n"
  "   o o o o    oo\"  o\"      \"o    $$o$\"     o o$\"\"  o$      \"$  "
  "\"oo   o o o o\n"
  "   \"$o   \"\"$$$\"   $$         $      \"   o   \"\"    o\"         $"
  "   \"o$$\"    o$$\n"
  "     \"\"o       o  $          $\"       $$$$$       o          $  ooo"
  "     o\"\"\n"
  "        \"o   $$$$o $o       o$        $$$$$\"       $o        \" $$$$"
  "   o\"\n"
  "         \"\"o $$$$o  oo o  o$\"         $$$$$\"        \"o o o o\"  "
  "\"$$$  $\n"
  "           \"\" \"$\"     \"\"\"\"\"            \"\"$\"            \""
  "\"\"      \"\"\" \"\n"
  "            \"oooooooooooooooooooooooooooooooooooooooooooooooooooooo$\n"
  "             \"$$$$\"$$$$\" $$$$$$$\"$$$$$$ \" \"$$$$$\"$$$$$$\"  $$$\""
  "\"$$$$\n"
  "              $$$oo$$$$   $$$$$$o$$$$$$o\" $$$$$$$$$$$$$$ o$$$$o$$$\"\n"
  "              $\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\""
  "\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"$\n"
  "              $\"                                                 \"$\n"
  "              $\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\""
  "$\"$\"$\"$\"$\"$\"$\"$\n"
  "                                   You win!\n\n";

const char *tombstone =
  "\n\n\n\n                /\"\"\"\"\"/\"\"\"\"\"\"\".\n"
  "               /     /         \\             __\n"
  "              /     /           \\            ||\n"
  "             /____ /   Rest in   \\           ||\n"
  "            |     |    Pieces     |          ||\n"
  "            |     |               |          ||\n"
  "            |     |   A. Luser    |          ||\n"
  "            |     |               |          ||\n"
  "            |     |     * *   * * |         _||_\n"
  "            |     |     *\\/* *\\/* |        | TT |\n"
  "            |     |     *_\\_  /   ...\"\"\"\"\"\"| |"
  "| |.\"\"....\"\"\"\"\"\"\"\".\"\"\n"
  "            |     |         \\/..\"\"\"\"\"...\"\"\""
  "\\ || /.\"\"\".......\"\"\"\"...\n"
  "            |     |....\"\"\"\"\"\"\"........\"\"\"\"\""
  "\"^^^^\".......\"\"\"\"\"\"\"\"..\"\n"
  "            |......\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"......"
  "..\"\"\"\"\"....\"\"\"\"\"..\"\"...\"\"\".\n\n"
  "            You're dead.  Better luck in the next life.\n\n\n";

void usage(char *name)
{
  fprintf(stderr,
          "Usage: %s [-r|--rand <seed>] [-l|--load [<file>]]\n"
          "          [-s|--save [<file>]] [-i|--image <pgm file>]\n"
          "          [-p|--pc <y> <x>] [-n|--nummon <count>]\n",
          name);

  exit(-1);
}
int getRenderX(dungeon_t *d){

   if(d->pc.position[dim_x] - 41 < 0){
     return 0;
   }else if(d->pc.position[dim_x] + 41 > 165){
   	 return (165-81);
   }else{
     return (d->pc.position[dim_x] - 41);
   }
}

int getRenderY(dungeon_t *d){

   if(d->pc.position[dim_y] - 11 < 0){
     return 0;
   }else if(d->pc.position[dim_y] + 11 > 105){
     return (105 - 21);
   }else{
   	return (d->pc.position[dim_y] - 11);
	}
}

int main(int argc, char *argv[])
{
  dungeon_t d;
  time_t seed;
  struct timeval tv;
  uint32_t i;
  uint32_t do_load, do_save, do_seed, do_image, do_place_pc;
  uint32_t long_arg;
  char *save_file;
  char *load_file;
  char *pgm_file;

  memset(&d, 0, sizeof (d));

  /* Default behavior: Seed with the time, generate a new dungeon, *
   * and don't write to disk.                                      */
  do_load = do_save = do_image = do_place_pc = 0;
  do_seed = 1;
  save_file = load_file = NULL;
  d.max_monsters = MAX_MONSTERS;
  initscr();
  cbreak();
 // int starty = (LINES - 24) / 2;	
  //int startx = (COLS - 81) / 2;	
  WINDOW *local_win = newwin(24,81,0,0);
  keypad(local_win,TRUE);

  box(local_win,0,0);
  wrefresh(local_win);


  /* The project spec requires '--load' and '--save'.  It's common  *
   * to have short and long forms of most switches (assuming you    *
   * don't run out of letters).  For now, we've got plenty.  Long   *
   * forms use whole words and take two dashes.  Short forms use an *
    * abbreviation after a single dash.  We'll add '--rand' (to     *
   * specify a random seed), which will take an argument of it's    *
   * own, and we'll add short forms for all three commands, '-l',   *
   * '-s', and '-r', respectively.  We're also going to allow an    *
   * optional argument to load to allow us to load non-default save *
   * files.  No means to save to non-default locations, however.    *
   * And the final switch, '--image', allows me to create a dungeon *
   * from a PGM image, so that I was able to create those more      *
   * interesting test dungeons for you.                             */
 
 if (argc > 1) {
    for (i = 1, long_arg = 0; i < argc; i++, long_arg = 0) {
      if (argv[i][0] == '-') { /* All switches start with a dash */
        if (argv[i][1] == '-') {
          argv[i]++;    /* Make the argument have a single dash so we can */
          long_arg = 1; /* handle long and short args at the same place.  */
        }
        switch (argv[i][1]) {
        case 'r':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-rand")) ||
              argc < ++i + 1 /* No more arguments */ ||
              !sscanf(argv[i], "%lu", &seed) /* Argument is not an integer */) {
            usage(argv[0]);
          }
          do_seed = 0;
          break;
        case 'l':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-load"))) {
            usage(argv[0]);
          }
          do_load = 1;
          if ((argc > i + 1) && argv[i + 1][0] != '-') {
            /* There is another argument, and it's not a switch, so *
             * we'll treat it as a save file and try to load it.    */
            load_file = argv[++i];
          }
          break;
        case 's':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-save"))) {
            usage(argv[0]);
          }
          do_save = 1;
          if ((argc > i + 1) && argv[i + 1][0] != '-') {
            /* There is another argument, and it's not a switch, so *
             * we'll treat it as a save file and try to load it.    */
            save_file = argv[++i];
          }
          break;
        case 'i':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-image"))) {
            usage(argv[0]);
          }
          do_image = 1;
          if ((argc > i + 1) && argv[i + 1][0] != '-') {
            /* There is another argument, and it's not a switch, so *
             * we'll treat it as a save file and try to load it.    */
            pgm_file = argv[++i];
          }
          break;
        case 'n':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-nummon")) ||
              argc < ++i + 1 /* No more arguments */ ||
              !sscanf(argv[i], "%hu", &d.max_monsters)) {
            usage(argv[0]);
          }
          break;
        case 'p':
          if ((!long_arg && argv[i][2])            ||
              (long_arg && strcmp(argv[i], "-pc")) ||
              argc <= i + 2                        ||
              argv[i + 1][0] == '-'                ||
              argv[i + 2][0] == '-') {
            usage(argv[0]);
          }
          do_place_pc = 1;
          if ((d.pc.position[dim_y] = atoi(argv[++i])) < 1 ||
              d.pc.position[dim_y] > DUNGEON_Y - 2         ||
              (d.pc.position[dim_x] = atoi(argv[++i])) < 1 ||
              d.pc.position[dim_x] > DUNGEON_X - 2) {
            fprintf(stderr, "Invalid PC position.\n");
            usage(argv[0]);
          }
          break;
        default:
          usage(argv[0]);
        }
      } else { /* No dash */
        usage(argv[0]);
      }
    }
  }

  if (do_seed) {
    /* Allows me to generate more than one dungeon *
     * per second, as opposed to time().           */
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }

//  printf("Seed is %ld.\n", seed);
  srand(seed);

  init_dungeon(&d);

  if (do_load) {
    read_dungeon(&d, load_file);
  } else if (do_image) {
    read_pgm(&d, pgm_file);
  } else {
    gen_dungeon(&d);
  }

  config_pc(&d);
  gen_monsters(&d);
  int look_mode = 0;

  wmove(local_win,0,0);
  pair_t * previous = malloc(sizeof(pair_t));

  render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
  wrefresh(local_win);

    while (pc_is_alive(&d) && dungeon_has_npcs(&d)) {

   int ch = wgetch(local_win);

   if(ch == 'L'){
   	look_mode = 1;
   }else if(ch == 7 || ch == 'y'){
   	//moves upper left
   	if(look_mode){
   		//do nothing
   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,-1,-1);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}

   }else if(ch == 8 || ch == 'k'){
   	//moves up in both control and look
   	if(look_mode){
   		wmove(local_win,0,0);
   		render_dungeon(&d,0,-1,local_win,1,previous);
   		wrefresh(local_win);


   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,0,-1);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}
   }else if(ch == 9 || ch == 'u'){
   	//moves upper right
   	if(look_mode){
   		//do nothing
   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,1,-1);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}
   }else if(ch == 3 || ch == 'n'){
   	//moves lower right
   	if(look_mode){
   		//do nothing
   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,1,1);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}
   }else if(ch == 2 || ch == 'j'){
   	//moves down
   	if(look_mode){
   		wmove(local_win,0,0);
   		render_dungeon(&d,0,1,local_win,1,previous);
   		wrefresh(local_win);

   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,0,1);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}
   }else if(ch == 1 || ch == 'b'){
   	//moves lower left
   	if(look_mode){
   		//do nothing
   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,-1,1);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}
   }else if(ch == 5 || ch == ' '){
   	//rest
   	   	if(look_mode){
   		//do nothing
   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,0,0);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}
   }else if(ch == '>'){
   	//down stairs
   	if(d.map[d.pc.position[dim_y]][d.pc.position[dim_x]] == ter_floor_stair_down){
   		pc_delete(d.pc.pc);
   		delete_dungeon(&d);
   		init_dungeon(&d);
   		gen_dungeon(&d);
   		config_pc(&d);
   		gen_monsters(&d);
   		wmove(local_win,0,0);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,0,0);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);   		
   	}

   }else if(ch == '<'){
   	//up stairs
   	if(d.map[d.pc.position[dim_y]][d.pc.position[dim_x]] == ter_floor_stair_up){
   		pc_delete(d.pc.pc);
   		delete_dungeon(&d);
   		init_dungeon(&d);
   		gen_dungeon(&d);
   		config_pc(&d);
   		gen_monsters(&d);
   		wmove(local_win,0,0);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,0,0);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);   		
   	}

   }else if(ch == 27){
   	//escape
   	if(look_mode){
   		look_mode = 0;
   		wmove(local_win,0,0);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}else{
   		//do nothing
   	}
   }else if(ch == 'Q'){
   	//quit game
   	  pc_delete(d.pc.pc);
      delete_dungeon(&d);

      endwin();
   	return 0;
   }else if(ch == 4 || ch == 'h'){
   	//left
   	if(look_mode){
   		wmove(local_win,0,0);
   		render_dungeon(&d,-1,0,local_win,1,previous);
   		wrefresh(local_win);
   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,-1,0);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}

   }else if(ch == 6 || ch == 'l'){
   	//right
   	if(look_mode){
   		wmove(local_win,0,0);
   		render_dungeon(&d,1,0,local_win,1,previous);
   		wrefresh(local_win);
   	}else{
   		wmove(local_win,0,0);
   		do_moves(&d,1,0);
   		render_dungeon(&d,getRenderX(&d),getRenderY(&d),local_win,0,previous);
   		wrefresh(local_win);
   	}
   }
  } 





  if (do_save) {
    write_dungeon(&d, save_file);
  }

  // printf(pc_is_alive(&d) ? victory : tombstone);
  // printf("\nYou defended your life in the face of %u deadly beasts.\n"
  //        "You avenged the cruel and untimely murders of %u peaceful dungeon residents.\n",
  //        d.pc.kills[kill_direct], d.pc.kills[kill_avenged]);

  pc_delete(d.pc.pc);

  delete_dungeon(&d);

  endwin();

  return 0;
}

