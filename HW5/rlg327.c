#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <endian.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/time.h>
#include <errno.h>

#include "heap.h"

/* Returns true if random float in [0,1] is less than *
 * numerator/denominator.  Uses only integer math.    */
# define rand_under(numerator, denominator) \
  (rand() < ((RAND_MAX / denominator) * numerator))

/* Returns random integer in [min, max]. */
# define rand_range(min, max) ((rand() % (((max) + 1) - (min))) + (min))

typedef struct corridor_path {
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
} corridor_path_t;

typedef enum dim {
  dim_x,
  dim_y,
  num_dims
} dim_t;



//priority queue globals
typedef int16_t pair_t[num_dims];

#define DUNGEON_X              160
#define DUNGEON_Y              105
#define MIN_ROOMS              25
#define MAX_ROOMS              40
#define ROOM_MIN_X             7
#define ROOM_MIN_Y             5
#define ROOM_MAX_X             20
#define ROOM_MAX_Y             15
#define SAVE_DIR               ".rlg327"
#define DUNGEON_SAVE_FILE      "dungeon"
#define DUNGEON_SAVE_SEMANTIC  "RLG327-S2017"
#define DUNGEON_SAVE_VERSION   0U
#define INFINITY               100000
#define UNDEFINED              -2
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))
  
#define mappair(pair) (d->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (d->map[y][x])
#define hardnesspair(pair) (d->hardness[pair[dim_y]][pair[dim_x]])
#define hardnessxy(x, y) (d->hardness[y][x])

typedef enum __attribute__ ((__packed__)) terrain_type {
  ter_debug,
  ter_wall,
  ter_wall_immutable,
  ter_floor,
  ter_floor_room,
  ter_floor_hall,
  ter_pc,
} terrain_type_t;

struct block {
  int x;
  int y;
  int distance;
  int visited;
  int weight;
  terrain_type_t type;
};


typedef struct room {
  pair_t position;
  pair_t size;
} room_t;



typedef struct pc{
	pair_t position;

}pc_t;

typedef struct dungeon {
  uint32_t num_rooms;
  room_t *rooms;
  terrain_type_t map[DUNGEON_Y][DUNGEON_X];
  /* Since hardness is usually not used, it would be expensive to pull it *
   * into cache every time we need a map cell, so we store it in a        *
   * parallel array, rather than using a structure to represent the       *
   * cells.  We may want a cell structure later, but from a performanace  *
   * perspective, it would be a bad idea to ever have the map be part of  *
   * that structure.  Pathfinding will require efficient use of the map,  *
   * and pulling in unnecessary data with each map cell would add a lot   *
   * of overhead to the memory system.                                    */
  uint8_t hardness[DUNGEON_Y][DUNGEON_X];
  pc_t pc;
  int nummon;
  uint32_t distance[DUNGEON_Y][DUNGEON_X];
  int eventMap[DUNGEON_Y][DUNGEON_X];
  int distanceNonTunnel[DUNGEON_Y][DUNGEON_X];
  int distanceTunnel[DUNGEON_Y][DUNGEON_X];
} dungeon_t;

typedef struct event {
	int characteristic;
	int lastSeenX;
	int lastSeenY;
	int currentPosX;
	int currentPosY;
	int speed;
    int sequence;
    int isPC;
    int nextTurn;
    int isAlive;
    void (*movePtr)(dungeon_t *,struct event *,int);

}event_t;

static uint32_t in_room(dungeon_t *d, int16_t y, int16_t x)
{
  int i;

  for (i = 0; i < d->num_rooms; i++) {
    if ((x >= d->rooms[i].position[dim_x]) &&
        (x < (d->rooms[i].position[dim_x] + d->rooms[i].size[dim_x])) &&
        (y >= d->rooms[i].position[dim_y]) &&
        (y < (d->rooms[i].position[dim_y] + d->rooms[i].size[dim_y]))) {
      return 1;
    }
  }

  return 0;
}

static int32_t corridor_path_cmp(const void *key, const void *with) {
  return ((corridor_path_t *) key)->cost - ((corridor_path_t *) with)->cost;
}

static void dijkstra_corridor(dungeon_t *d, pair_t from, pair_t to)
{
  static corridor_path_t path[DUNGEON_Y][DUNGEON_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < DUNGEON_Y; y++) {
      for (x = 0; x < DUNGEON_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      path[y][x].cost = INT_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, corridor_path_cmp, NULL);

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (mapxy(x, y) != ter_wall_immutable) {
        path[y][x].hn = heap_insert(&h, &path[y][x]);
      } else {
        path[y][x].hn = NULL;
      }
    }
  }

  while ((p = heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != from[dim_x]) || (y != from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y]) {
        if (mapxy(x, y) != ter_floor_room) {
          mapxy(x, y) = ter_floor_hall;
          hardnessxy(x, y) = 0;
        }
      }
      heap_delete(&h);
      return;
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost >
         p->cost + hardnesspair(p->pos))) {
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost =
        p->cost + hardnesspair(p->pos);
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]    ].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost >
         p->cost + hardnesspair(p->pos))) {
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost =
        p->cost + hardnesspair(p->pos);
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost >
         p->cost + hardnesspair(p->pos))) {
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost =
        p->cost + hardnesspair(p->pos);
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost >
         p->cost + hardnesspair(p->pos))) {
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost =
        p->cost + hardnesspair(p->pos);
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]    ].hn);
    }
  }
}

/* This is a cut-and-paste of the above.  The code is modified to  *
 * calculate paths based on inverse hardnesses so that we get a    *
 * high probability of creating at least one cycle in the dungeon. */
static void dijkstra_corridor_inv(dungeon_t *d, pair_t from, pair_t to)
{
  static corridor_path_t path[DUNGEON_Y][DUNGEON_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < DUNGEON_Y; y++) {
      for (x = 0; x < DUNGEON_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      path[y][x].cost = INT_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, corridor_path_cmp, NULL);

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (mapxy(x, y) != ter_wall_immutable) {
        path[y][x].hn = heap_insert(&h, &path[y][x]);
      } else {
        path[y][x].hn = NULL;
      }
    }
  }

  while ((p = heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != from[dim_x]) || (y != from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y]) {
        if (mapxy(x, y) != ter_floor_room) {
          mapxy(x, y) = ter_floor_hall;
          hardnessxy(x, y) = 0;
        }
      }
      heap_delete(&h);
      return;
    }

#define hardnesspair_inv(p) (in_room(d, p[dim_y], p[dim_x]) ? \
                             224                            : \
                             (255 - hardnesspair(p)))

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost >
         p->cost + hardnesspair_inv(p->pos))) {
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost =
        p->cost + hardnesspair_inv(p->pos);
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]    ].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost >
         p->cost + hardnesspair_inv(p->pos))) {
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost =
        p->cost + hardnesspair_inv(p->pos);
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost >
         p->cost + hardnesspair_inv(p->pos))) {
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost =
        p->cost + hardnesspair_inv(p->pos);
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost >
         p->cost + hardnesspair_inv(p->pos))) {
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost =
        p->cost + hardnesspair_inv(p->pos);
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]    ].hn);
    }
  }
}

/* Chooses a random point inside each room and connects them with a *
 * corridor.  Random internal points prevent corridors from exiting *
 * rooms in predictable locations.                                  */
static int connect_two_rooms(dungeon_t *d, room_t *r1, room_t *r2)
{
  pair_t e1, e2;

  e1[dim_y] = rand_range(r1->position[dim_y],
                         r1->position[dim_y] + r1->size[dim_y] - 1);
  e1[dim_x] = rand_range(r1->position[dim_x],
                         r1->position[dim_x] + r1->size[dim_x] - 1);
  e2[dim_y] = rand_range(r2->position[dim_y],
                         r2->position[dim_y] + r2->size[dim_y] - 1);
  e2[dim_x] = rand_range(r2->position[dim_x],
                         r2->position[dim_x] + r2->size[dim_x] - 1);

  /*  return connect_two_points_recursive(d, e1, e2);*/
  dijkstra_corridor(d, e1, e2);

  return 0;
}

static int create_cycle(dungeon_t *d)
{
  /* Find the (approximately) farthest two rooms, then connect *
   * them by the shortest path using inverted hardnesses.      */

  int32_t max, tmp, i, j, p, q;
  pair_t e1, e2;

  for (i = max = 0; i < d->num_rooms - 1; i++) {
    for (j = i + 1; j < d->num_rooms; j++) {
      tmp = (((d->rooms[i].position[dim_x] - d->rooms[j].position[dim_x])  *
              (d->rooms[i].position[dim_x] - d->rooms[j].position[dim_x])) +
             ((d->rooms[i].position[dim_y] - d->rooms[j].position[dim_y])  *
              (d->rooms[i].position[dim_y] - d->rooms[j].position[dim_y])));
      if (tmp > max) {
        max = tmp;
        p = i;
        q = j;
      }
    }
  }

  /* Can't simply call connect_two_rooms() because it doesn't *
   * use inverse hardnesses, so duplicate it here.            */
  e1[dim_y] = rand_range(d->rooms[p].position[dim_y],
                         (d->rooms[p].position[dim_y] +
                          d->rooms[p].size[dim_y] - 1));
  e1[dim_x] = rand_range(d->rooms[p].position[dim_x],
                         (d->rooms[p].position[dim_x] +
                          d->rooms[p].size[dim_x] - 1));
  e2[dim_y] = rand_range(d->rooms[q].position[dim_y],
                         (d->rooms[q].position[dim_y] +
                          d->rooms[q].size[dim_y] - 1));
  e2[dim_x] = rand_range(d->rooms[q].position[dim_x],
                         (d->rooms[q].position[dim_x] +
                          d->rooms[q].size[dim_x] - 1));

  dijkstra_corridor_inv(d, e1, e2);

  return 0;
}

static int connect_rooms(dungeon_t *d)
{
  uint32_t i;

  for (i = 1; i < d->num_rooms; i++) {
    connect_two_rooms(d, d->rooms + i - 1, d->rooms + i);
  }

  create_cycle(d);

  return 0;
}

int gaussian[5][5] = {
  {  1,  4,  7,  4,  1 },
  {  4, 16, 26, 16,  4 },
  {  7, 26, 41, 26,  7 },
  {  4, 16, 26, 16,  4 },
  {  1,  4,  7,  4,  1 }
};

typedef struct queue_node {
  int x, y;
  struct queue_node *next;
} queue_node_t;

static int smooth_hardness(dungeon_t *d)
{
  int32_t i, x, y;
  int32_t s, t, p, q;
  queue_node_t *head, *tail, *tmp;
  uint8_t hardness[DUNGEON_Y][DUNGEON_X];
#ifdef DUMP_HARDNESS_IMAGES
  FILE *out;
#endif
  memset(&hardness, 0, sizeof (hardness));

  /* Seed with some values */
  for (i = 1; i < 255; i += 5) {
    do {
      x = rand() % DUNGEON_X;
      y = rand() % DUNGEON_Y;
    } while (hardness[y][x]);
    hardness[y][x] = i;
    if (i == 1) {
      head = tail = malloc(sizeof (*tail));
    } else {
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

#ifdef DUMP_HARDNESS_IMAGES
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", DUNGEON_X, DUNGEON_Y);
  fwrite(&hardness, sizeof (hardness), 1, out);
  fclose(out);
#endif

  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = hardness[y][x];

    if (x - 1 >= 0 && y - 1 >= 0 && !hardness[y - 1][x - 1]) {
      hardness[y - 1][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y - 1;
    }
    if (x - 1 >= 0 && !hardness[y][x - 1]) {
      hardness[y][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y;
    }
    if (x - 1 >= 0 && y + 1 < DUNGEON_Y && !hardness[y + 1][x - 1]) {
      hardness[y + 1][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y + 1;
    }
    if (y - 1 >= 0 && !hardness[y - 1][x]) {
      hardness[y - 1][x] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y - 1;
    }
    if (y + 1 < DUNGEON_Y && !hardness[y + 1][x]) {
      hardness[y + 1][x] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y + 1;
    }
    if (x + 1 < DUNGEON_X && y - 1 >= 0 && !hardness[y - 1][x + 1]) {
      hardness[y - 1][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y - 1;
    }
    if (x + 1 < DUNGEON_X && !hardness[y][x + 1]) {
      hardness[y][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y;
    }
    if (x + 1 < DUNGEON_X && y + 1 < DUNGEON_Y && !hardness[y + 1][x + 1]) {
      hardness[y + 1][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y + 1;
    }

    tmp = head;
    head = head->next;
    free(tmp);
  }

  /* And smooth it a bit with a gaussian convolution */
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < DUNGEON_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < DUNGEON_X) {
            s += gaussian[p][q];
            t += hardness[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      d->hardness[y][x] = t / s;
    }
  }
  /* Let's do it again, until it's smooth like Kenny G. */
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < DUNGEON_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < DUNGEON_X) {
            s += gaussian[p][q];
            t += hardness[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      d->hardness[y][x] = t / s;
    }
  }

#ifdef DUMP_HARDNESS_IMAGES
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", DUNGEON_X, DUNGEON_Y);
  fwrite(&hardness, sizeof (hardness), 1, out);
  fclose(out);

  out = fopen("smoothed.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", DUNGEON_X, DUNGEON_Y);
  fwrite(&d->hardness, sizeof (d->hardness), 1, out);
  fclose(out);
#endif

  return 0;
}

static int empty_dungeon(dungeon_t *d)
{
  uint8_t x, y;

  smooth_hardness(d);
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      mapxy(x, y) = ter_wall;
      if (y == 0 || y == DUNGEON_Y - 1 ||
          x == 0 || x == DUNGEON_X - 1) {
        mapxy(x, y) = ter_wall_immutable;
        hardnessxy(x, y) = 255;
      }
    }
  }

  return 0;
}

static int place_rooms(dungeon_t *d)
{
  pair_t p;
  uint32_t i;
  int success;
  room_t *r;
  uint8_t hardness[DUNGEON_Y][DUNGEON_X];
  uint32_t x, y;
  struct timeval tv, start;

  /* Placing rooms is 2D bin packing.  Bin packing (2D or otherwise) is NP-  *
   * Complete, meaning (among other things) that there is no known algorithm *
   * to solve the problem in less than exponential time.  There are          *
   * hacks and approximation algorithms that can function more efficiently,  *
   * but we're going to forgoe those in favor of using a timeout.  If we     *
   * can't place all of our rooms in 1 second (and some change), we'll abort *
   * this attempt and start over.                                            */
  gettimeofday(&start, NULL);

  memcpy(&hardness, &d->hardness, sizeof (hardness));

  for (success = 0; !success; ) {
    success = 1;
    for (i = 0; success && i < d->num_rooms; i++) {
      r = d->rooms + i;
      r->position[dim_x] = 1 + rand() % (DUNGEON_X - 2 - r->size[dim_x]);
      r->position[dim_y] = 1 + rand() % (DUNGEON_Y - 2 - r->size[dim_y]);
      for (p[dim_y] = r->position[dim_y] - 1;
           success && p[dim_y] < r->position[dim_y] + r->size[dim_y] + 1;
           p[dim_y]++) {
        for (p[dim_x] = r->position[dim_x] - 1;
             success && p[dim_x] < r->position[dim_x] + r->size[dim_x] + 1;
             p[dim_x]++) {
          if (mappair(p) >= ter_floor) {
            gettimeofday(&tv, NULL);
            if ((tv.tv_sec - start.tv_sec) > 1) {
              memcpy(&d->hardness, &hardness, sizeof (hardness));
              return 1;
            }
            success = 0;
            /* empty_dungeon() regenerates the hardness map, which   *
             * is prohibitively expensive to do in a loop like this, *
             * so instead, we'll use a copy.                         */
            memcpy(&d->hardness, &hardness, sizeof (hardness));
            for (y = 1; y < DUNGEON_Y - 1; y++) {
              for (x = 1; x < DUNGEON_X - 1; x++) {
                mapxy(x, y) = ter_wall;
              }
            }
          } else if ((p[dim_y] != r->position[dim_y] - 1)              &&
                     (p[dim_y] != r->position[dim_y] + r->size[dim_y]) &&
                     (p[dim_x] != r->position[dim_x] - 1)              &&
                     (p[dim_x] != r->position[dim_x] + r->size[dim_x])) {
            mappair(p) = ter_floor_room;
            hardnesspair(p) = 0;
          }
        }
      }
    }
  }

  return 0;
}

static int add_pc(dungeon_t *d,int random){
	if(random){
		d->pc.position[dim_y] = d->rooms[0].position[dim_y];
		d->pc.position[dim_x] = d->rooms[0].position[dim_x];
	}

	d->map[d->pc.position[dim_y]][d->pc.position[dim_x]] = ter_pc;
	return 0;
}

static int make_rooms(dungeon_t *d)
{
  uint32_t i;

  for (i = MIN_ROOMS; i < MAX_ROOMS && rand_under(6, 8); i++)
    ;
  d->num_rooms = i;
  if (d->rooms) {
    /* If we're restarting due to a timeout in place_rooms(), we need to     *
     * free our rooms array or we'll lose the pointer (could use realloc()). */
    free(d->rooms);
  }
  d->rooms = malloc(sizeof (*d->rooms) * d->num_rooms);

  for (i = 0; i < d->num_rooms; i++) {
    d->rooms[i].size[dim_x] = ROOM_MIN_X;
    d->rooms[i].size[dim_y] = ROOM_MIN_Y;
    while (rand_under(3, 4) && d->rooms[i].size[dim_x] < ROOM_MAX_X) {
      d->rooms[i].size[dim_x]++;
    }
    while (rand_under(3, 4) && d->rooms[i].size[dim_y] < ROOM_MAX_Y) {
      d->rooms[i].size[dim_y]++;
    }
  }

  return 0;
}

int gen_dungeon(dungeon_t *d)
{
  do {
    make_rooms(d);
  } while (place_rooms(d));
  connect_rooms(d);


  return 0;
}



void render_dungeon(dungeon_t *d)
{
  pair_t p;

  for (p[dim_y] = 0; p[dim_y] < DUNGEON_Y; p[dim_y]++) {
    for (p[dim_x] = 0; p[dim_x] < DUNGEON_X; p[dim_x]++) {
  if(d->eventMap[p[dim_y]][p[dim_x]] == -1 || d->eventMap[p[dim_y]][p[dim_x]] == 69){
      switch (mappair(p)) {
      case ter_wall:
      case ter_wall_immutable:
        putchar(' ');
        break;
      case ter_floor:
      case ter_pc:
      	putchar('@');
      	break;
      case ter_floor_room:
        putchar('.');
        break;
      case ter_floor_hall:
        putchar('#');
        break;
      case ter_debug:
        putchar('*');
        fprintf(stderr, "Debug character at %d, %d\n", p[dim_y], p[dim_x]);
        break;
      		}
  		}else{
  			putchar('M');
  		}
    }
    putchar('\n');
  }
}

void delete_dungeon(dungeon_t *d)
{
  free(d->rooms);
}

void init_dungeon(dungeon_t *d)
{
  d->rooms = NULL;
  empty_dungeon(d);
}

int write_dungeon_map(dungeon_t *d, FILE *f)
{
  uint32_t x, y;

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      fwrite(&d->hardness[y][x], sizeof (unsigned char), 1, f);
    }
  }

  return 0;
}

int write_rooms(dungeon_t *d, FILE *f)
{
  uint32_t i;
  uint8_t p;

  for (i = 0; i < d->num_rooms; i++) {
    /* write order is xpos, ypos, width, height */
    p = d->rooms[i].position[dim_x];
    fwrite(&p, 1, 1, f);
    p = d->rooms[i].position[dim_y];
    fwrite(&p, 1, 1, f);
    p = d->rooms[i].size[dim_x];
    fwrite(&p, 1, 1, f);
    p = d->rooms[i].size[dim_y];
    fwrite(&p, 1, 1, f);
  }

  return 0;
}

uint32_t calculate_dungeon_size(dungeon_t *d)
{
  return (20 /* The semantic, version, and size */     +
          (DUNGEON_X * DUNGEON_Y) /* The hardnesses */ +
          (d->num_rooms * 4) /* Four bytes per room */);
}

int makedirectory(char *dir)
{
  char *slash;

  for (slash = dir + strlen(dir); slash > dir && *slash != '/'; slash--)
    ;

  if (slash == dir) {
    return 0;
  }

  if (mkdir(dir, 0700)) {
    if (errno != ENOENT && errno != EEXIST) {
      fprintf(stderr, "mkdir(%s): %s\n", dir, strerror(errno));
      return 1;
    }
    if (*slash != '/') {
      return 1;
    }
    *slash = '\0';
    if (makedirectory(dir)) {
      *slash = '/';
      return 1;
    }

    *slash = '/';
    if (mkdir(dir, 0700) && errno != EEXIST) {
      fprintf(stderr, "mkdir(%s): %s\n", dir, strerror(errno));
      return 1;
    }
  }

  return 0;
}

int write_dungeon(dungeon_t *d, char *file)
{
  char *home;
  char *filename;
  FILE *f;
  size_t len;
  uint32_t be32;

  if (!file) {
    if (!(home = getenv("HOME"))) {
      fprintf(stderr, "\"HOME\" is undefined.  Using working directory.\n");
      home = ".";
    }

    len = (strlen(home) + strlen(SAVE_DIR) + strlen(DUNGEON_SAVE_FILE) +
           1 /* The NULL terminator */                                 +
           2 /* The slashes */);

    filename = malloc(len * sizeof (*filename));
    sprintf(filename, "%s/%s/", home, SAVE_DIR);
    makedirectory(filename);
    strcat(filename, DUNGEON_SAVE_FILE);

    if (!(f = fopen(filename, "w"))) {
      perror(filename);
      free(filename);

      return 1;
    }
    free(filename);
  } else {
    if (!(f = fopen(file, "w"))) {
      perror(file);
      exit(-1);
    }
  }

  /* The semantic, which is 12 bytes, 0-11 */
  fwrite(DUNGEON_SAVE_SEMANTIC, 1, strlen(DUNGEON_SAVE_SEMANTIC), f);

  /* The version, 4 bytes, 12-15 */
  be32 = htobe32(DUNGEON_SAVE_VERSION);
  fwrite(&be32, sizeof (be32), 1, f);

  /* The size of the file, 4 bytes, 16-19 */
  be32 = htobe32(calculate_dungeon_size(d));
  fwrite(&be32, sizeof (be32), 1, f);

  /* The dungeon map, 16800 bytes, 20-16819 */
  write_dungeon_map(d, f);

  /* And the rooms, num_rooms * 4 bytes, 16820-end */
  write_rooms(d, f);

  fclose(f);

  return 0;
}

int read_dungeon_map(dungeon_t *d, FILE *f)
{
  uint32_t x, y;

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      fread(&d->hardness[y][x], sizeof (d->hardness[y][x]), 1, f);
      if (d->hardness[y][x] == 0) {
        /* Mark it as a corridor.  We can't recognize room cells until *
         * after we've read the room array, which we haven't done yet. */
        d->map[y][x] = ter_floor_hall;
      } else if (d->hardness[y][x] == 255) {
        d->map[y][x] = ter_wall_immutable;
      } else {
        d->map[y][x] = ter_wall;
      }
    }
  }


  return 0;
}

int read_rooms(dungeon_t *d, FILE *f)
{
  uint32_t i;
  uint32_t x, y;
  uint8_t p;

  for (i = 0; i < d->num_rooms; i++) {
    fread(&p, 1, 1, f);
    d->rooms[i].position[dim_x] = p;
    fread(&p, 1, 1, f);
    d->rooms[i].position[dim_y] = p;
    fread(&p, 1, 1, f);
    d->rooms[i].size[dim_x] = p;
    fread(&p, 1, 1, f);
    d->rooms[i].size[dim_y] = p;
    /* After reading each room, we need to reconstruct them in the dungeon. */
    for (y = d->rooms[i].position[dim_y];
         y < d->rooms[i].position[dim_y] + d->rooms[i].size[dim_y];
         y++) {
      for (x = d->rooms[i].position[dim_x];
           x < d->rooms[i].position[dim_x] + d->rooms[i].size[dim_x];
           x++) {
        mapxy(x, y) = ter_floor_room;
      }
    }
  }

  return 0;
}

int calculate_num_rooms(uint32_t dungeon_bytes)
{
  return ((dungeon_bytes -
          (20 /* The semantic, version, and size */       +
           (DUNGEON_X * DUNGEON_Y) /* The hardnesses */)) /
          4 /* Four bytes per room */);
}

int read_dungeon(dungeon_t *d, char *file)
{
  char semantic[13];
  uint32_t be32;
  FILE *f;
  char *home;
  size_t len;
  char *filename;
  struct stat buf;

  if (!file) {
    if (!(home = getenv("HOME"))) {
      fprintf(stderr, "\"HOME\" is undefined.  Using working directory.\n");
      home = ".";
    }

    len = (strlen(home) + strlen(SAVE_DIR) + strlen(DUNGEON_SAVE_FILE) +
           1 /* The NULL terminator */                                 +
           2 /* The slashes */);

    filename = malloc(len * sizeof (*filename));
    sprintf(filename, "%s/%s/%s", home, SAVE_DIR, DUNGEON_SAVE_FILE);

    if (!(f = fopen(filename, "r"))) {
      perror(filename);
      free(filename);
      exit(-1);
    }

    if (stat(filename, &buf)) {
      perror(filename);
      exit(-1);
    }

    free(filename);
  } else {
    if (!(f = fopen(file, "r"))) {
      perror(file);
      exit(-1);
    }
    if (stat(file, &buf)) {
      perror(file);
      exit(-1);
    }
  }

  d->num_rooms = 0;

  fread(semantic, sizeof(semantic) - 1, 1, f);
  semantic[12] = '\0';
  if (strncmp(semantic, DUNGEON_SAVE_SEMANTIC, 12)) {
    fprintf(stderr, "Not an RLG327 save file.\n");
    exit(-1);
  }
  fread(&be32, sizeof (be32), 1, f);
  if (be32toh(be32) != 0) { /* Since we expect zero, be32toh() is a no-op. */
    fprintf(stderr, "File version mismatch.\n");
    exit(-1);
  }
  fread(&be32, sizeof (be32), 1, f);
  if (buf.st_size != be32toh(be32)) {
    fprintf(stderr, "File size mismatch.\n");
    exit(-1);
  }
  read_dungeon_map(d, f);
  d->num_rooms = calculate_num_rooms(buf.st_size);
  d->rooms = malloc(sizeof (*d->rooms) * d->num_rooms);
  read_rooms(d, f);

  fclose(f);

  return 0;
}

int read_pgm(dungeon_t *d, char *pgm)
{
  FILE *f;
  char s[80];
  uint8_t gm[103][158];
  uint32_t x, y;
  uint32_t i;

  if (!(f = fopen(pgm, "r"))) {
    perror(pgm);
    exit(-1);
  }

  if (!fgets(s, 80, f) || strncmp(s, "P5", 2)) {
    fprintf(stderr, "Expected P5\n");
    exit(-1);
  }
  if (!fgets(s, 80, f) || s[0] != '#') {
    fprintf(stderr, "Expected comment\n");
    exit(-1);
  }
  if (!fgets(s, 80, f) || strncmp(s, "158 103", 5)) {
    fprintf(stderr, "Expected 158 103\n");
    exit(-1);
  }
  if (!fgets(s, 80, f) || strncmp(s, "255", 2)) {
    fprintf(stderr, "Expected 255\n");
    exit(-1);
  }

  fread(gm, 1, 158 * 103, f);

  fclose(f);

  /* In our gray map, treat black (0) as corridor, white (255) as room, *
   * all other values as a hardness.  For simplicity, treat every white *
   * cell as its own room, so we have to count white after reading the  *
   * image in order to allocate the room array.                         */
  for (d->num_rooms = 0, y = 0; y < 103; y++) {
    for (x = 0; x < 158; x++) {
      if (!gm[y][x]) {
        d->num_rooms++;
      }
    }
  }
  d->rooms = malloc(sizeof (*d->rooms) * d->num_rooms);

  for (i = 0, y = 0; y < 103; y++) {
    for (x = 0; x < 158; x++) {
      if (!gm[y][x]) {
        d->rooms[i].position[dim_x] = x + 1;
        d->rooms[i].position[dim_y] = y + 1;
        d->rooms[i].size[dim_x] = 1;
        d->rooms[i].size[dim_y] = 1;
        i++;
        d->map[y + 1][x + 1] = ter_floor_room;
        d->hardness[y + 1][x + 1] = 0;
      } else if (gm[y][x] == 255) {
        d->map[y + 1][x + 1] = ter_floor_hall;
        d->hardness[y + 1][x + 1] = 0;
      } else {
        d->map[y + 1][x + 1] = ter_wall;
        d->hardness[y + 1][x + 1] = gm[y][x];
      }
    }
  }

  for (x = 0; x < 160; x++) {
    d->map[0][x] = ter_wall_immutable;
    d->hardness[0][x] = 255;
    d->map[104][x] = ter_wall_immutable;
    d->hardness[104][x] = 255;
  }
  for (y = 1; y < 104; y++) {
    d->map[y][0] = ter_wall_immutable;
    d->hardness[y][0] = 255;
    d->map[y][159] = ter_wall_immutable;
    d->hardness[y][159] = 255;
  }

  return 0;
}

int weightForHardness(int x, int y, dungeon_t * d){
  if(d->hardness[y][x] < 85 && d->hardness[y][x] >= 0) return 1;
  if(d->hardness[y][x] > 84 && d->hardness[y][x] < 171) return 2;
  if(d->hardness[y][x] > 171 && d->hardness[y][x] < 255) return 3;

  return INFINITY;
  
}

struct block extractMin(struct block queue[105][160],int *count){
	int i,j;
	int minimum = INFINITY; 
	struct block temp;
	for(i = 0;i<DUNGEON_Y;i++){
		for(j = 0; j<DUNGEON_X;j++){
			if(queue[i][j].visited != 1 && queue[i][j].distance < minimum){
				minimum = queue[i][j].distance;
				temp = queue[i][j];
			}
		}
	}
	(*count)--;
	temp.visited = 1;
	queue[temp.y][temp.x] = temp;
	return temp;
}

struct block extractMinNonTunnel(struct block queue[105][160],int *count){
	int i,j;
	int minimum = INFINITY; 
	struct block temp;
	for(i = 0;i<DUNGEON_Y;i++){
		for(j = 0; j<DUNGEON_X;j++){
			if(queue[i][j].visited != 1 && queue[i][j].distance < minimum && queue[i][j].type != ter_wall_immutable && queue[i][j].type != ter_wall){
				minimum = queue[i][j].distance;
				temp = queue[i][j];
			}
		}
	}
	(*count)--;
	temp.visited = 1;
	queue[temp.y][temp.x] = temp;
	return temp;
}

void calculateDistance(dungeon_t *d){
  int i,j;
  struct block queue[105][160];
  int *count = malloc(sizeof(int));
  int firstRun = 0;
  *count = 16800;


  for(i=0;i<DUNGEON_Y;i++){
  	for(j = 0;j<DUNGEON_X;j++){
  		queue[i][j].x = j;
  		queue[i][j].y = i;
  		queue[i][j].distance = INFINITY;
  		queue[i][j].weight = weightForHardness(j,i,d);
  		queue[i][j].visited = 0;
  		queue[i][j].type = d->map[i][j];

  	}
  }

  while(*count != 0){
  	struct block bl;
  	if(firstRun == 0){
  		bl = queue[d->pc.position[dim_y]][d->pc.position[dim_x]];
  		bl.distance = 0;
  		bl.visited = 1;
  		queue[d->pc.position[dim_y]][d->pc.position[dim_x]] = bl;
  	}else{
  		bl = extractMin(queue,count);
  	}
    firstRun = 1;

    int startPosX = (bl.x - 1 < 0) ? bl.x : bl.x-1;
    int startPosY = (bl.y - 1 < 0) ? bl.y : bl.y-1;
    int endPosX =   (bl.x + 1 > 159) ? bl.x : bl.x+1;
    int endPosY =   (bl.y + 1 > 104) ? bl.y : bl.y+1;


    // See how many are alive
    int rowNum,colNum;
    for (rowNum=startPosX; rowNum<=endPosX; rowNum++) {
        for (colNum=startPosY; colNum<=endPosY; colNum++) {
          // All the neighbors will be grid[rowNum][colNum]
          struct block u = queue[colNum][rowNum];
          if(u.visited == 0){
          	if(u.weight == INFINITY){
          		u.visited = 1;
          		u.distance = bl.distance + bl.weight;
          		(*count) --;
          	}else{
          		int alt = bl.distance + bl.weight;
          		if(alt < u.distance|| u.distance == INFINITY){
          			u.distance = alt;
          		}
          	}
          }
          queue[colNum][rowNum] = u;
   		}
	}
}
for(i = 1;i<DUNGEON_Y-1;i++){
	printf("\n");
	for(j = 1;j<DUNGEON_X-1;j++){
		if(queue[i][j].type == ter_pc){
			//printf("%c",'@');
			d->distanceTunnel[i][j] = -1;
		}else{
			d->distanceTunnel[i][j] = queue[i][j].distance;
			//printf("%d",(queue[i][j].distance % 10));
		}
	}
}
printf("\n");
printf("\n");
free(count);
}

void calculateDistanceNonTunnel(dungeon_t *d){
	 int i,j;
  struct block queue[105][160];
  int *count = malloc(sizeof(int));
  int firstRun = 0;
  *count = 16800;


  for(i=0;i<DUNGEON_Y;i++){
  	for(j = 0;j<DUNGEON_X;j++){
  		queue[i][j].x = j;
  		queue[i][j].y = i;
  		queue[i][j].distance = INFINITY;
  		queue[i][j].weight = weightForHardness(j,i,d);
  		queue[i][j].visited = 0;
  		queue[i][j].type = d->map[i][j];
  	}
  }

  while(*count != 0){
  	struct block bl;
  	if(firstRun == 0){
  		bl = queue[d->pc.position[dim_y]][d->pc.position[dim_x]];
  		bl.distance = 0;
  		bl.visited = 1;
  		queue[d->pc.position[dim_y]][d->pc.position[dim_x]] = bl;
  	}else{
  		bl = extractMinNonTunnel(queue,count);
  	}
    firstRun = 1;

    int startPosX = (bl.x - 1 < 0) ? bl.x : bl.x-1;
    int startPosY = (bl.y - 1 < 0) ? bl.y : bl.y-1;
    int endPosX =   (bl.x + 1 > 159) ? bl.x : bl.x+1;
    int endPosY =   (bl.y + 1 > 104) ? bl.y : bl.y+1;


    // See how many are alive
    int rowNum,colNum;
    for (rowNum=startPosX; rowNum<=endPosX; rowNum++) {
        for (colNum=startPosY; colNum<=endPosY; colNum++) {
          // All the neighbors will be grid[rowNum][colNum]
          struct block u = queue[colNum][rowNum];
          if(u.visited == 0){
          	if(u.weight == INFINITY||u.type == ter_wall_immutable||u.type == ter_wall){
          		u.visited = 1;
          		u.distance = bl.distance + bl.weight;
          		(*count) --;
          	}else{
          		int alt = bl.distance + bl.weight;
          		if(alt < u.distance|| u.distance == INFINITY){
          			u.distance = alt;
          		}
          	}
          }
          queue[colNum][rowNum] = u;
   		}
	}
}
for(i = 1;i<DUNGEON_Y-1;i++){
	//printf("\n");
	for(j = 1;j<DUNGEON_X-1;j++){
		if(queue[i][j].type == ter_wall ||queue[i][j].type == ter_wall_immutable){
			d->distanceNonTunnel[i][j] = -2;
			//printf("%c",' ');
		}else if(queue[i][j].type == ter_pc){
			d->distanceNonTunnel[i][j] = -1;
			//printf("%c",'@');
		}else{
			d->distanceNonTunnel[i][j] = queue[i][j].distance;
			//printf("%d",(queue[i][j].distance % 10));
		}
	}
}
printf("\n");
printf("\n");
free(count);
}

// void printDungeon(dungeon_t *d){
// 	int i,j;
// 	for(i = 0;i < DUNGEON_Y;i++){
// 		for(j = 0; j < DUNGEON_X;j++){
// 			if(d->eventMap[i][j] == -1){
// 				switch (mappair(p)) {
//       			case ter_wall:
//      	 		case ter_wall_immutable:
//         		putchar(' ');
//        			break;
//      			case ter_floor:
//       			case ter_pc:
//       			putchar('@');
//       			break;
//       			case ter_floor_room:
//       			putchar('.');
//       			break;
//      			case ter_floor_hall:
//         		putchar('#');
//         		break;
//       			case ter_debug:
//       			putchar('*');
//         		fprintf(stderr, "Debug character at %d, %d\n", p[dim_y], p[dim_x]);
//         		break;
//      			 }
// 				printf("%s\n", );
// 			}
// 		}
// 	}
// }

int inSameRoom(dungeon_t *d,struct event NPC, struct event PC){
	int x = NPC.currentPosX;
	int y = NPC.currentPosY;
	int i;
	for(i = 0;i<d->num_rooms;i++){
		struct room currRoom = d->rooms[i];
		if((x > currRoom.position[dim_x] && x < currRoom.position[dim_x] + currRoom.size[dim_x]) && (y > currRoom.position[dim_y] && y < currRoom.position[dim_y] + currRoom.size[dim_y])){
			//the NPC is in currRoom
			if((PC.currentPosX > currRoom.position[dim_x] && PC.currentPosX < currRoom.position[dim_x] + currRoom.size[dim_x]) && (PC.currentPosY > currRoom.position[dim_y] && PC.currentPosY < currRoom.position[dim_y] + currRoom.size[dim_y])){
				//the PC is in currRoom
				return 1;
			}

		}
	}

	return 0;
}

int moveShortestPath(dungeon_t *d,struct event events[], struct event currEvent,int nextEventIndex,int isTunnel){
	if(isTunnel){
		return 0;

	}else{
		int i,j;
		int minX,minY;
		int min = 1000000;
		for(i = currEvent.currentPosX - 1;i < currEvent.currentPosX + 1;i++){
			for(j = currEvent.currentPosY - 1;j < currEvent.currentPosY + 1;j++){
				if(!(d->map[j][i] == ter_wall || d->map[j][i] == ter_wall_immutable)){
					if(d->distanceNonTunnel[j][i] < min){
						min = d->distanceNonTunnel[j][i];
						minX = i;
						minY = j;
					}
				}
			}
		}
	    
	    for(i = 0;i < d->nummon;i++){
	    	if(events[i].currentPosX == minX && events[i].currentPosY == minY && events[i].isAlive){
	    		if(events[i].isPC){
	    			//game over
	    			printf("%s\n","GAME OVER");
	    			exit(1);
	    		}else{
	    			d->eventMap[minY][minX] = -1;
	    			d->eventMap[events[i].currentPosY][events[i].currentPosX] = -1;
	    			events[i].isAlive = 0;
	    			currEvent.isAlive = 0;
	    			events[nextEventIndex] = currEvent;
	    			return 0;
	    		}
	    	}
	    }
	    d->eventMap[currEvent.currentPosY][currEvent.currentPosX] = -1;

	    currEvent.currentPosX = minX;
	    currEvent.currentPosY = minY;
	    d->eventMap[currEvent.currentPosY][currEvent.currentPosX] = currEvent.characteristic;
	    events[nextEventIndex] = currEvent;
	    return 1;

	}


}
int getNextEvent(dungeon_t * d,struct event events[],int tick){
	int i,j;
	int min = 10000;
	int mintick = 100000;
	int foundNext = 0;
	struct event temp;
	for(i = 0;i < d->nummon + 1;i++){
		struct event currEvent = events[i];
		if(tick >= currEvent.nextTurn && currEvent.isAlive){
			temp = currEvent;
			if(min > currEvent.sequence && mintick > currEvent.nextTurn){
				temp = currEvent;
				min = currEvent.sequence;
				mintick = currEvent.nextTurn;
				foundNext = 1;
				j = i;
			}
		}
	}
	if(foundNext){
		temp.nextTurn = temp.nextTurn + (1000/temp.speed);
		events[j] = temp;
		return j;
	}else{
		return -1;
	}

}

void moveNothing(dungeon_t *d,struct event queue[],int nextEventIndex){
	printf("%s\n", "Called Nothing");

}


void movePC(dungeon_t *d,struct event queue[],int nextEventIndex){
	//keep in same spot for now
	render_dungeon(d);
	//calculateDistance(d);
	//calculateDistanceNonTunnel(d);

}

void moveIntel(dungeon_t *d,struct event events[],int nextEventIndex){
	// if(inSameRoom(d,events[nextEventIndex],events[0])){
	// 	moveShortestPath(d,events,events[nextEventIndex],nextEventIndex,0);
	// }

	
}

void moveTele(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveIntelTele(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveTunnel(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveIntelTunnel(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveTeleTunnel(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveIntelTeleTunnel(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveErratic(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveIntelErratic(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveTeleErratic(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveIntelTeleErratic(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveTunnelErratic(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}


void moveIntelTunnelErratic(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}


void moveTeleTunnelErratic(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void moveIntelTeleTunnelErratic(dungeon_t *d,struct event queue[],int nextEventIndex){
	
}

void generateCharacteristics(struct event events[],dungeon_t *d){
  int i,j;

  for(i=0;i<DUNGEON_Y;i++){
  	for(j=0;j<DUNGEON_X;j++){
  		d->eventMap[i][j] = -1;
  	}
  }
  d->eventMap[d->pc.position[dim_y]][d->pc.position[dim_x]] = 69;

  srand(time(NULL));
  // for(i = 0;i<150;i++){
  // 	int asdf = (rand() % 10);
  // 	printf("%d\n",asdf);
  // }
    for(i = 1;i < d->nummon;i++){
       int intel = rand() % 2;
       int telepath = rand() % 2;
       int tunnel = rand() % 2;
       int erratic = rand() % 2;
       struct event genEvent;
       int temp = 0;

       if(intel){
       	BIT_SET(temp,0);
       }
       if(telepath){
       	BIT_SET(temp,1);
       }
       if(tunnel){
       	BIT_SET(temp,2);
       }
       if(erratic){
       	BIT_SET(temp,3);
      }

       genEvent.characteristic = temp;
       genEvent.isPC = 0;  

       int select = (rand() % d->num_rooms - 1) + 1;
       printf("%d\n",d->num_rooms);
       printf("%d\n",select);
   	   int height = d->rooms[select].size[dim_y];
   	   int width = d->rooms[select].size[dim_x];
   	   int randHeight = rand() % height;
   	   int randWidth = rand() % width;

   	   while(d->eventMap[randHeight][randWidth]!= -1){
   	   		randHeight = rand() % height;
   	   		randWidth = rand() % width;
   	   }
   	   genEvent.currentPosX = randWidth + d->rooms[select].position[dim_x];
   	   genEvent.lastSeenX = -1;
   	   genEvent.lastSeenY = -1;
   	   genEvent.isAlive = 1;
   	   genEvent.currentPosY = randHeight + d->rooms[select].position[dim_y];
   	   genEvent.sequence = i;

   	   
   	   int speed = (rand() % 16) + 5;
   	   genEvent.speed = speed;
   	   int nextTurn = 1000 / speed;
   	   genEvent.nextTurn = nextTurn;

   	   d->eventMap[genEvent.currentPosY][genEvent.currentPosX] = genEvent.characteristic;

   	   switch(temp){
   	   	case 0:
   	   		genEvent.movePtr = &moveNothing;
   	   		break;

   	   	case 1:
   	   		genEvent.movePtr = &moveIntel;
   	   		break;

   	   	case 2:
   	   		genEvent.movePtr = &moveTele;
   	   		break;

   	   	case 3:
   	   		genEvent.movePtr = &moveIntelTele;
   	   		break;

   	   	case 4:
   	   		genEvent.movePtr = &moveTunnel;
   	   		break;

   	   	case 5:
   	   		genEvent.movePtr = &moveIntelTunnel;
   	   		break;

   	   	case 6:
   	   		genEvent.movePtr = &moveTeleTunnel;
   	   		break;

   	   	case 7:
   	   		genEvent.movePtr = &moveIntelTeleTunnel;
   	   		break;

   	   	case 8:
   	   		genEvent.movePtr = &moveErratic;
   	   		break;

   	   	case 9:
   	   		genEvent.movePtr = &moveIntelErratic;
   	   		break;

   	   	case 10:
   	   		genEvent.movePtr = &moveTeleErratic;
   	   		break;

   	   	case 11:
   	   		genEvent.movePtr = &moveIntelTeleErratic;
   	   		break;

   	   	case 12:
   	   		genEvent.movePtr = &moveTunnelErratic;
   	   		break;

   	   	case 13:
   	   		genEvent.movePtr = &moveIntelTunnelErratic;
   	   		break;

   	   	case 14:
   	   		genEvent.movePtr = &moveTeleTunnelErratic;
   	   		break;

   	   	case 15:
   	   		genEvent.movePtr = &moveIntelTeleTunnelErratic;
   	   		break;

   	   	default:
   	   		genEvent.movePtr = &moveNothing;
   	   }

   	   events[i] = genEvent;

   }
   struct event pcEvent;
   pcEvent.currentPosX = d->pc.position[dim_x];
   pcEvent.currentPosY = d->pc.position[dim_y];
   pcEvent.speed = 10;
   pcEvent.sequence = 0;
   pcEvent.characteristic = 0;
   pcEvent.lastSeenY = -1;
   pcEvent.lastSeenX = -1;
   pcEvent.isPC = 1;
   pcEvent.isAlive = 1;
   pcEvent.nextTurn = 100;
   pcEvent.movePtr = &movePC;
   events[0] = pcEvent;
   //(events[0].movePtr)(d,events);

}



void runSimulation(dungeon_t *d, struct event events[]){
	int tick = 0;
	while(tick!=1000){
		int nextEventIndex = getNextEvent(d,events,tick);
		if(nextEventIndex != -1){
			struct event nextEvent = events[nextEventIndex];
			(nextEvent.movePtr)(d,events,nextEventIndex);

		}
		tick++;
	}
}

void usage(char *name)
{
  fprintf(stderr,
          "Usage: %s [-r|--rand <seed>] [-l|--load [<file>]]\n"
          "          [-s|--save [<file>]] [-i|--image <pgm file>]\n",
          name);

  exit(-1);
}

int main(int argc, char *argv[])
{
  dungeon_t d;
  time_t seed;
  struct timeval tv;
  uint32_t i;
  uint32_t do_load, do_save, do_seed, do_image, do_pc,do_nummon;
  uint32_t long_arg;
  char *save_file;
  char *load_file;
  char *pgm_file;

  /* Default behavior: Seed with the time, generate a new dungeon, *
   * and don't write to disk.                                      */
  do_load = do_save = do_image = do_pc = do_nummon = 0;
  do_seed = 1;
  save_file = load_file = NULL;

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

        case 'p':
          if ((!long_arg && argv[i][2])            ||
              (long_arg && strcmp(argv[i], "-pc")) ||
              argc <= i + 2){
            usage(argv[0]);
          }
          do_pc = 1;
          if ((d.pc.position[dim_y] = atoi(argv[++i])) < 1 ||
              d.pc.position[dim_y] > DUNGEON_Y - 2         ||
              (d.pc.position[dim_x] = atoi(argv[++i])) < 1 ||
              d.pc.position[dim_x] > DUNGEON_X - 2) {
            fprintf(stderr, "Invalid PC position.\n");
            usage(argv[0]);
          }
          break;    
        case 'n':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-nummon")) || argc <= i + 1) {
              usage(argv[0]);
          }
          do_nummon = 1;
          if(atoi(argv[++i]) > 0){
          	d.nummon = atoi(argv[i]);
          }else{
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

  printf("Seed is %ld.\n", seed);
  srand(seed);

  init_dungeon(&d);

  if (do_load) {
    read_dungeon(&d, load_file);
  } else if (do_image) {
    read_pgm(&d, pgm_file);
  } else {
    gen_dungeon(&d);
  }

  if(do_pc){
  	add_pc(&d,0);
  	calculateDistance(&d);
  	calculateDistanceNonTunnel(&d);
  }else{
  	add_pc(&d,1);
  	calculateDistance(&d);
  	calculateDistanceNonTunnel(&d);
  }
  if(do_nummon){
  	struct event events[d.nummon + 1];
  	generateCharacteristics(events,&d);
  	runSimulation(&d,events);
  }
  render_dungeon(&d);


  if (do_save) {
    write_dungeon(&d, save_file);
  }

  delete_dungeon(&d);

  return 0;
}

//Priority Queue Implementation

