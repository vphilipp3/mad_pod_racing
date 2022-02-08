#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <bits/types/struct_timeval.h>
#include <sys/time.h>


/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

#define SOLUTION_LEN 6
#define SOLUTIONS_NBR 10000
#define MUT_SOLUTIONS_NBR 15


//#define OFFLINE 1
#ifndef OFFLINE
#define ONLINE
#else
#define PRINT_PLAY 1
#define PRINT_COL 1
#define PRINT_BOUNCE 1
#define PRINT_LOAD 1
#define PRINT_MOVE 1
#define PRINT_SCOREP 1
#endif

//#define PRINT_SCOREP 1
// #define PRINT_COL 1
// #define PRINT_BOUNCE 1

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) > (b) ? (b) : (a))

 typedef struct Coordonnees Coordonnees;
 struct Coordonnees {
     float x;
     float y;
 };

typedef struct Action Action;
struct Action {
    Coordonnees target;
    float thrust;
    bool shield;
    bool boost;
};

#define POD_TYPE 0 
#define CHECKPOINT_TYPE 1

typedef struct Unit Unit;
struct Unit {
    Coordonnees position;
    Coordonnees speed;
    Coordonnees prev_position;
    char type;
    int radius;
};

#define USER_TYPE 0
#define OPPONENT_TYPE 1
#define TIMEOUT 100
#define POD_RADIUS 400

typedef struct Pod Pod;
struct Pod {
    Unit unit;
    float distance_to_opp[2];
    float angle_to_opp[2];
    float angle;
    int nextcheckpoint;
    bool boost_available;
    int shield;
    Action action;
    int timeout;
    int checkpoints_checked;
    int team;
    int rank;
    int ckp_nbr;
    int dist_to_ckp;
    int timout_to_last_ckp;
};

typedef struct Race Race;
struct Race {
    int checkpoint_nbr;
    int lap_nbr;
    int current_lap;
    int current_checkpoint;
    Unit checkpoints[20]; 
    int worst_teammate;
    int best_opp;
    int worst_opp;
    int best_teammate;
};

Pod Players[4];
Pod Players_bkp[4];

typedef struct Collision Collision;
struct Collision {
    Unit* unit_1;
    Unit* unit_2;
    Pod* pod_1;
    Pod* pod_2;
    double t;
};

typedef struct Move Move;
struct Move {
    float angle;
    int thrust;
    bool shield;
};

typedef struct TeamMoves TeamMoves;
struct TeamMoves {
    Move move_1;
    Move move_2;
};

typedef struct Solution Solution;
struct Solution {
    long score;
    int len;
    TeamMoves teamMoves[SOLUTION_LEN];
};


Pod shadow;
Race race;
//Players players;
bool first_run = true;
static struct timeval lap_start_time;
static struct timeval lap_check;
unsigned long delta_us = 0;
bool debug = false;
bool expla = false;

Move move_1;
Move move_2;

void update();
void init();
void process(Pod *current_pod);
Coordonnees closest(Coordonnees* pt, Coordonnees* vect_a, Coordonnees* vect_b);
float getAngle(Coordonnees p, Coordonnees this);
float diffAngle(Coordonnees p, Coordonnees this, float angle);
long naive_solution(Solution* c);
void rank_players();


static struct timeval last_check;

void reset_watchdog(){
    gettimeofday(&last_check, NULL);
}

void check_watchdog(char* txt){
    #ifdef OFFLINE
    int val = 500000;
    #endif
    #ifdef ONLINE
    int val = 72000;
    #endif
    struct timeval check;
    gettimeofday(&check, NULL);
    unsigned long delta = (unsigned long) 1000000 * (check.tv_sec - last_check.tv_sec) + (check.tv_usec - last_check.tv_usec);
    if(delta > val){
        fprintf(stderr, "WATCHDOG !!! (%lu) %s\n",delta, txt);
        reset_watchdog();
    }
}


void mutate(Move* this, float amplitude) {

    float ramin = this->angle - 36.0 * amplitude;
    float ramax = this->angle + 36.0 * amplitude;

    if (ramin < -18.0) {
        ramin = -18.0;
    }

    if (ramax > 18.0) {
        ramax = 18.0;
    }

    this->angle = ramin + (ramax - ramin) * rand() / ( RAND_MAX + 1.0);
    //this->angle = random(ramin, ramax);

    int pmin = this->thrust - 100 * amplitude;
    int pmax = this->thrust + 100 * amplitude;

    if (pmin < 0) {
        pmin = 0;
    }

    if (pmax > 0) {
        pmax = 100;
    }

    if(rand()/(RAND_MAX + 1.0)<0.01){
        this->shield = true;
    }

    //float rd = 0.5 + rand() / (RAND_MAX + 1.0);
    float rd = 3.0 * (rand()/(RAND_MAX + 1.0));
    if(rd>=1){
        rd = 1;
    } else {
        rd = rd;
    }
    //this->thrust = pmin + (pmax - pmin) * rand() / RAND_MAX;
    this->thrust = (int) 100.0 * rd;
    
    //this->thrust = random(pmin, pmax);
}

void mutate_s(Solution* s, float amplitude){
    float rd = rand();
    for(int i = 0; i < s->len; i++){
        mutate(&s->teamMoves[i].move_1, amplitude);
        if(s->teamMoves[i].move_1.shield){
            Players[0].shield = -3;
        }
        mutate(&s->teamMoves[i].move_2, amplitude);
        if(s->teamMoves[i].move_2.shield){
            Players[1].shield = -3;
        }
    }

}


void gen_solutions_mut(Solution* s, int nbr, int solution_len){
    int s_n = 0;
    int a[2] = {0,0};

    Coordonnees next_target[2];

    if(race.best_teammate==0){
        next_target[0] = race.checkpoints[Players[0].nextcheckpoint].position;
        next_target[1] = race.checkpoints[(Players[race.best_opp].nextcheckpoint+1)%race.checkpoint_nbr].position;
    } else {
        next_target[0] = race.checkpoints[(Players[race.best_opp].nextcheckpoint+1)%race.checkpoint_nbr].position;
        next_target[1] = race.checkpoints[Players[1].nextcheckpoint].position;
    }

    a[0] = diffAngle(next_target[0],
                 Players[0].unit.position,
                 Players[0].angle);

    a[1] = diffAngle(next_target[1],
                Players[0].unit.position,
                Players[0].angle);


    for(int o = 0; o < 2; o++){
        for(int j = 0; j<SOLUTION_LEN; j++){
            int turn_a, turn_t;
            if(a[o]>=18){
                if(a[o]>90){
                    turn_t = 0;
                } else {
                    turn_t = 100;
                }
                turn_a = 18;
                a[o] -= 18;
            } else if (a[o]<18 && a[o]>-18){
                turn_a = a[o];
                a[o] = 0;
                turn_t = 100;
            } else {
                if(a[o]<-90){
                    turn_t = 0;
                } else {
                    turn_t = 100;
                }
                turn_a = -18;
                a[o] += 18;
            }
            if(o==0){
                s[s_n].teamMoves[j].move_1.angle = turn_a;
                s[s_n].teamMoves[j].move_1.thrust = turn_t;
            } else if (o==1){
                s[s_n].teamMoves[j].move_2.angle = turn_a;
                s[s_n].teamMoves[j].move_2.thrust = turn_t;
            }
        }
    }

    a[0] = diffAngle(next_target[0],
                 Players[0].unit.position,
                 Players[0].angle);

    a[1] = diffAngle(next_target[1],
                Players[0].unit.position,
                Players[0].angle);

    s[s_n].len = solution_len;
    s_n += 1;

    for(int o = 0; o < 2; o++){
        for(int j = 0; j<SOLUTION_LEN; j++){
            int turn_a, turn_t;
            if(a[o]>=18){
                turn_t = 0;
                turn_a = 18;
                a[o] -= 18;
            } else if (a[o]>5) {
                turn_a = a[o];
                a[o] = 0;
                turn_t = 0;
            } else if (a[o]<=-18){
                turn_t = 0;
                turn_a = -18;
                a[o] += 18;
            } else if (a[o]<-5) {
                turn_a = a[o];
                a[o] = 0;
                turn_t = 0;
            } else {
                turn_a = 0;
                turn_t = 100;
            }
            if(o==0){
                s[s_n].teamMoves[j].move_1.angle = turn_a;
                s[s_n].teamMoves[j].move_1.thrust = turn_t;
            } else if (o==1){
                s[s_n].teamMoves[j].move_2.angle = turn_a;
                s[s_n].teamMoves[j].move_2.thrust = turn_t;
            }
        }
    }
    s[s_n].len = solution_len;

    s_n += 1;

    naive_solution(&s[s_n]);
    s[s_n].len = solution_len;
    check_watchdog("p naive_solution");

    s_n += 1;

    #ifdef PRINT_NAIVE
    fprintf(stderr, "print naive solution 1\n");
    for(int j = 0; j<SOLUTION_LEN; j++){
        fprintf(stderr, "[%f:%d][%f:%d]\n",
            s[0].teamMoves[j].move_1.angle,
            s[0].teamMoves[j].move_1.thrust,
            s[0].teamMoves[j].move_2.angle,
            s[0].teamMoves[j].move_2.thrust);
    }

    fprintf(stderr, "print naive solution 2\n");
    for(int j = 0; j<SOLUTION_LEN; j++){
        fprintf(stderr, "[%f:%d][%f:%d]\n",
            s[1].teamMoves[j].move_1.angle,
            s[1].teamMoves[j].move_1.thrust,
            s[1].teamMoves[j].move_2.angle,
            s[1].teamMoves[j].move_2.thrust);
    }

    fprintf(stderr, "print naive solution 3\n");
    for(int j = 0; j<SOLUTION_LEN; j++){
        fprintf(stderr, "[%f:%d][%f:%d]\n",
            s[2].teamMoves[j].move_1.angle,
            s[2].teamMoves[j].move_1.thrust,
            s[2].teamMoves[j].move_2.angle,
            s[2].teamMoves[j].move_2.thrust);
    }
    #endif

    int angles[5] = {0,18,-18,5,-5};
    int thurst[5] = {100,100,100,100,100};
    int c = 0;
    
    for(int i = s_n; i<nbr; i++){
        for(int j = 0; j<MUT_SOLUTIONS_NBR; j++){
            int jt = j;

            // check_watchdog("Gen solutions xx");

            s[i].teamMoves[j].move_1.angle = angles[((c)/((int) pow(5, (int) jt))) % 5];
            s[i].teamMoves[j].move_1.thrust = thurst[((c)/((int) pow(5, (int) jt))) % 5];

            s[i].teamMoves[j].move_2.angle = 0;
            s[i].teamMoves[j].move_2.thrust = 100.0;
            
        }
        s[i].len = solution_len;
        // for(int j = 8; j<8; j++){ // 0:0 (0) 0:1 (1) 1:1 (2) 1:2 (3) 2:2 (4) c/2 (c+1)/2
        // 0:0 0:1 0:2 0:3 1:0 1:1 1:2 1:3
        //     s[i].teamMoves[j].move_1.angle = 0;
        //     s[i].teamMoves[j].move_1.thrust = 100;
  
        //     s[i].teamMoves[j].move_2.angle = 0;
        //     s[i].teamMoves[j].move_2.thrust = 100.0;
        //     s[i].len = solution_len;
        // }
        c++;
    }
}


void gen_solutions(Solution* s, int nbr, int solution_len){
    //Players[0].angle
    int s_n = 0;
    int a[2] = {0,0};

    a[0] = diffAngle(race.checkpoints[Players[0].nextcheckpoint].position,
                 Players[0].unit.position,
                 Players[0].angle);
    a[1] = diffAngle(race.checkpoints[Players[1].nextcheckpoint].position,
                Players[0].unit.position,
                Players[0].angle);
    for(int o = 0; o < 2; o++){
        for(int j = 0; j<SOLUTION_LEN; j++){
            int turn_a, turn_t;
            if(a[o]>=18){
                if(a[o]>90){
                    turn_t = 0;
                } else {
                    turn_t = 100;
                }
                turn_a = 18;
                a[o] -= 18;
            } else if (a[o]<18 && a[o]>-18){
                turn_a = a[o];
                a[o] = 0;
                turn_t = 100;
            } else {
                if(a[o]<-90){
                    turn_t = 0;
                } else {
                    turn_t = 100;
                }
                turn_a = -18;
                a[o] += 18;
            }
            if(o==0){
                s[s_n].teamMoves[j].move_1.angle = turn_a;
                s[s_n].teamMoves[j].move_1.thrust = turn_t;
            } else if (o==1){
                s[s_n].teamMoves[j].move_2.angle = turn_a;
                s[s_n].teamMoves[j].move_2.thrust = turn_t;
            }
        }
    }

    a[0] = diffAngle(race.checkpoints[Players[0].nextcheckpoint].position,
                 Players[0].unit.position,
                 Players[0].angle);
    a[1] = diffAngle(race.checkpoints[Players[1].nextcheckpoint].position,
                Players[0].unit.position,
                Players[0].angle);

    s[s_n].len = solution_len;
    s_n += 1;

    for(int o = 0; o < 2; o++){
        for(int j = 0; j<SOLUTION_LEN; j++){
            int turn_a, turn_t;
            if(a[o]>=18){
                turn_t = 0;
                turn_a = 18;
                a[o] -= 18;
            } else if (a[o]>5) {
                turn_a = a[o];
                a[o] = 0;
                turn_t = 0;
            } else if (a[o]<=-18){
                turn_t = 0;
                turn_a = -18;
                a[o] += 18;
            } else if (a[o]<-5) {
                turn_a = a[o];
                a[o] = 0;
                turn_t = 0;
            } else {
                turn_a = 0;
                turn_t = 100;
            }
            if(o==0){
                s[s_n].teamMoves[j].move_1.angle = turn_a;
                s[s_n].teamMoves[j].move_1.thrust = turn_t;
            } else if (o==1){
                s[s_n].teamMoves[j].move_2.angle = turn_a;
                s[s_n].teamMoves[j].move_2.thrust = turn_t;
            }
        }
    }
    s[s_n].len = solution_len;

    s_n += 1;

    naive_solution(&s[s_n]);
    s[s_n].len = solution_len;
    check_watchdog("p naive_solution");

    s_n += 1;

    #ifdef PRINT_NAIVE
    fprintf(stderr, "print naive solution 1\n");
    for(int j = 0; j<SOLUTION_LEN; j++){
        fprintf(stderr, "[%f:%d][%f:%d]\n",
            s[0].teamMoves[j].move_1.angle,
            s[0].teamMoves[j].move_1.thrust,
            s[0].teamMoves[j].move_2.angle,
            s[0].teamMoves[j].move_2.thrust);
    }

    fprintf(stderr, "print naive solution 2\n");
    for(int j = 0; j<SOLUTION_LEN; j++){
        fprintf(stderr, "[%f:%d][%f:%d]\n",
            s[1].teamMoves[j].move_1.angle,
            s[1].teamMoves[j].move_1.thrust,
            s[1].teamMoves[j].move_2.angle,
            s[1].teamMoves[j].move_2.thrust);
    }

    fprintf(stderr, "print naive solution 3\n");
    for(int j = 0; j<SOLUTION_LEN; j++){
        fprintf(stderr, "[%f:%d][%f:%d]\n",
            s[2].teamMoves[j].move_1.angle,
            s[2].teamMoves[j].move_1.thrust,
            s[2].teamMoves[j].move_2.angle,
            s[2].teamMoves[j].move_2.thrust);
    }
    #endif

    int angles[5] = {0,18,-18,5,-5};
    int thurst[5] = {100,100,100,100,100};
    int c = 0;
    
    for(int i = s_n; i<nbr; i++){
        for(int j = 0; j<SOLUTION_LEN; j++){
            int jt = j;

            check_watchdog("Gen solutions xx");

            s[i].teamMoves[j].move_1.angle = angles[(int) ((c)/((int) pow(5, (int) jt))) % 5];
            s[i].teamMoves[j].move_1.thrust = thurst[(int) ((c)/((int) pow(5, (int) jt))) % 5];

            s[i].teamMoves[j].move_2.angle = 0;
            s[i].teamMoves[j].move_2.thrust = 100;
            
        }
        s[i].len = solution_len;
        // for(int j = 8; j<8; j++){ // 0:0 (0) 0:1 (1) 1:1 (2) 1:2 (3) 2:2 (4) c/2 (c+1)/2
        // 0:0 0:1 0:2 0:3 1:0 1:1 1:2 1:3
        //     s[i].teamMoves[j].move_1.angle = 0;
        //     s[i].teamMoves[j].move_1.thrust = 100;
  
        //     s[i].teamMoves[j].move_2.angle = 0;
        //     s[i].teamMoves[j].move_2.thrust = 100.0;
        //     s[i].len = solution_len;
        // }
        c++;
    }
}

long distance2(Coordonnees* p1, Coordonnees* p2) {
    return ((long) p1->x - p2->x)*((long) p1->x - p2->x) + ((long) p1->y - p2->y)*((long) p1->y - p2->y);
}

float distance(Coordonnees* p1, Coordonnees* p2) {
    return sqrt(distance2(p1, p2));
}

void init(){

    fprintf(stderr, "INIT\n");

    int count;

    scanf("%d", &race.lap_nbr);
    scanf("%d", &race.checkpoint_nbr);

    fprintf(stderr, "%d laps, %d checkpoints\n", race.lap_nbr, race.checkpoint_nbr);

    for(count = 0; count < race.checkpoint_nbr; count++)
    {
        scanf("%f%f", &race.checkpoints[count].position.x, &race.checkpoints[count].position.y);
        fprintf(stderr, "n°%d %f %f\n", count, race.checkpoints[count].position.x, race.checkpoints[count].position.y);
    }

    update();

    for(count = 0; count < 2; count++){
        Players[count].unit.prev_position.x = Players[count].unit.position.x;
        Players[count].unit.prev_position.y = Players[count].unit.position.y;
        Players[count].unit.radius = POD_RADIUS;
        Players[count].unit.type = POD_TYPE;
        Players[count].timeout = TIMEOUT;
        Players[count].checkpoints_checked = 0;
        Players[count].team = USER_TYPE;
        Players[count].boost_available = true;
        Players[count].rank = 0;
        Players[count].ckp_nbr = 0;
        Players[count].timout_to_last_ckp = 100;
        Players[count].shield = 0;
    }
    for(count = 2; count < 4; count++){
        Players[count].unit.prev_position.x = Players[count].unit.position.x;
        Players[count].unit.prev_position.y = Players[count].unit.position.y;
        Players[count].unit.radius = POD_RADIUS;
        Players[count].unit.type = POD_TYPE;
        Players[count].timeout = TIMEOUT;
        Players[count].checkpoints_checked = 0;
        Players[count].team = OPPONENT_TYPE;
        Players[count].rank = 0;
        Players[count].ckp_nbr = 0;
        Players[count].timout_to_last_ckp = 100;
        Players[count].shield = 0;
    }
    
}

void update(){
    int count;
    for(count = 0; count < 2; count++){
        scanf("%f%f%f%f%f%d", 
            &Players[count].unit.position.x,
            &Players[count].unit.position.y,
            &Players[count].unit.speed.x,
            &Players[count].unit.speed.y,
            &Players[count].angle,
            &Players[count].nextcheckpoint
            );

            if(Players_bkp[count].nextcheckpoint != Players[count].nextcheckpoint){
                Players[count].ckp_nbr += 1;
            }

            Players[count].dist_to_ckp = distance(&Players[count].unit.position, &race.checkpoints[Players[count].nextcheckpoint].position);


        //Players[count].angle -= 90;
        //Players[count].angle *= -Players[count].angle;
        
        if(Players[count].angle>=360.0){
            Players[count].angle -= 360.0;
        } else if(Players[count].angle<0.0){
            Players[count].angle += 360.0;
        }

        // fprintf(stderr, "x:%f y:%f vx:%f vy:%f a:%f ckp_id:%d \n", 
        //     Players[count].unit.position.x,
        //     Players[count].unit.position.y,
        //     Players[count].unit.speed.x,
        //     Players[count].unit.speed.y,
        //     Players[count].angle,
        //     Players[count].nextcheckpoint);

        Players[count].action.boost = false; 
        //Players[count].action.shield = false; 
    }
    for(count = 2; count < 4; count++){
        scanf("%f%f%f%f%f%d", 
            &Players[count].unit.position.x,
            &Players[count].unit.position.y,
            &Players[count].unit.speed.x,
            &Players[count].unit.speed.y,
            &Players[count].angle,
            &Players[count].nextcheckpoint
            );

            if(Players_bkp[count].nextcheckpoint != Players[count].nextcheckpoint){
                Players[count].ckp_nbr += 1;
            }

            Players[count].dist_to_ckp = distance(&Players[count].unit.position, &race.checkpoints[Players[count].nextcheckpoint].position);



        //Players[count].angle -= 90;
        //Players[count].angle = -Players[count].angle;

        if(Players[count].angle>=360.0){
            Players[count].angle -= 360.0;
        } else if(Players[count].angle<0.0){
            Players[count].angle += 360.0;
        }

        // fprintf(stderr, "x:%f y:%f vx:%f vy:%f a:%f ckp_id:%d \n", 
        //     Players[count].unit.position.x,
        //     Players[count].unit.position.y,
        //     Players[count].unit.speed.x,
        //     Players[count].unit.speed.y,
        //     Players[count].angle,
        //     Players[count].nextcheckpoint);
    }
       
    rank_players();
}

int compare (const void * a, const void * b)
{
  return (((Pod*)a)->ckp_nbr - ((Pod*)b)->ckp_nbr );
}

// int main ()
// {
//   int n;
//   qsort (values, 6, sizeof(Pod), compare);
//   for (n=0; n<6; n++)
//      printf ("%d ",values[n]);
//   return 0;
// }

bool collision(Unit* u_1, Unit* u_2, Collision* r_c) {
    //#define PRINT_COL

    static Unit* prev_u_1;
    static Unit* prev_u_2;
    static float prev_t = -1;

    if(prev_t == -1){
        prev_u_1 = u_1;
        prev_u_2 = u_2;
    }

      // Square of the distance
    long dist = (long) distance2(&u_1->position, &u_2->position) + 1;

    // Sum of the radii squared
    long sr = (u_1->radius + u_2->radius)*(u_1->radius + u_2->radius);

    // We take everything squared to avoid calling sqrt uselessly. It is better for performances

    if (dist < sr) {
        // Objects are already touching each other. We have an immediate collision.
        r_c->t = 0.0;
        r_c->unit_1 = u_1;
        r_c->unit_2 = u_2;

        #ifdef PRINT_COL
        fprintf(stderr, "Immediate collision (%d-%d) [%d:%d]\n", dist, sr, u_1, u_2);
        fprintf(stderr, "Units touching 1 --> (%f:%f)(%f:%f)\n", 
            u_1->position.x, 
            u_1->position.y, 
            u_1->speed.x, 
            u_1->speed.y);
        fprintf(stderr, "Units touching 2 --> (%f:%f)(%f:%f)\n", 
            u_2->position.x, 
            u_2->position.y, 
            u_2->speed.x, 
            u_2->speed.y);
        #endif 

        if(debug){
            fprintf(stderr, "Immediate collision (%d-%d) [%d:%d]\n", dist, sr, u_1, u_2);
            fprintf(stderr, "Units touching 1 --> (%f:%f)(%f:%f)\n", 
                u_1->position.x, 
                u_1->position.y, 
                u_1->speed.x, 
                u_1->speed.y);
            fprintf(stderr, "Units touching 2 --> (%f:%f)(%f:%f)\n", 
                u_2->position.x, 
                u_2->position.y, 
                u_2->speed.x, 
                u_2->speed.y);
        }

        if(!(u_1 == prev_u_1 && u_2 == prev_u_2 && prev_t == 0.0)){
            prev_u_1 = u_1;
            prev_u_2 = u_2;
            prev_t = 0.0;
            return true;
        }        
    }

    // Optimisation. Objects with the same speed will never collide
    if (u_1->speed.x == u_2->speed.x && u_1->speed.y == u_2->speed.y) {
        return false;
    }

    // We place ourselves in the reference frame of u. u is therefore stationary and is at (0,0)
    float x = u_1->position.x - u_2->position.x;
    float y = u_1->position.y - u_2->position.y;

    Coordonnees myp;
    myp.x = x;
    myp.y = y;

    float vx = u_1->speed.x - u_2->speed.x;
    float vy = u_1->speed.y - u_2->speed.y;

    Coordonnees up;
    up.x = 0;
    up.y = 0;

    Coordonnees pt;
    pt.x = x + vx;
    pt.y = y + vy;

    // We look for the closest point to u (which is in (0,0)) on the line described by our speed vector
    Coordonnees p = closest(&up, &myp, &pt);

    // Square of the distance between u and the closest point to u on the line described by our speed vector
    float pdist = distance2(&p, &up);

    // Square of the distance between us and that point
    float mypdist = distance2(&p, &myp);

    // If the distance between u and this line is less than the sum of the radii, there might be a collision
    if (pdist < sr) {
     // Our speed on the line
        float length = sqrt(vx*vx + vy*vy);

        // We move along the line to find the point of impact
        float backdist = sqrt(sr - pdist);
        p.x = p.x - backdist * (vx / length);
        p.y = p.y - backdist * (vy / length);

        // If the point is now further away it means we are not going the right way, therefore the collision won't happen
        if (distance2(&p, &myp) > mypdist) {
            return false;
        }

        pdist = distance(&myp, &p);

        // The point of impact is further than what we can travel in one turn
        if (pdist > length) {
            return false;
        }

        // Time needed to reach the impact point
        float t = pdist / length;

        #ifdef PRINT_COL
        fprintf(stderr, "Delayed collision (%f)(%f:%f) [%d:%d]\n", t, p.x, p.y, u_1, u_2);
        fprintf(stderr, "Units touching 1 --> (%f:%f)(%f:%f)\n", 
            u_1->position.x, 
            u_1->position.y, 
            u_1->speed.x, 
            u_1->speed.y);
        fprintf(stderr, "Units touching 2 --> (%f:%f)(%f:%f)\n", 
            u_2->position.x, 
            u_2->position.y, 
            u_2->speed.x, 
            u_2->speed.y);
        #endif

        if(debug){
            fprintf(stderr, "Delayed collision (%f)(%f:%f) [%d:%d]\n", t, p.x, p.y, u_1, u_2);
            fprintf(stderr, "Units touching 1 --> (%f:%f)(%f:%f)\n", 
                u_1->position.x, 
                u_1->position.y, 
                u_1->speed.x, 
                u_1->speed.y);
            fprintf(stderr, "Units touching 2 --> (%f:%f)(%f:%f)\n", 
                u_2->position.x, 
                u_2->position.y, 
                u_2->speed.x, 
                u_2->speed.y);
        }

        r_c->t = t;
        r_c->unit_1 = u_1;
        r_c->unit_2 = u_2;

        if(!(u_1 == prev_u_1 && u_2 == prev_u_2 && t == prev_t)){
            prev_u_1 = u_1;
            prev_u_2 = u_2;
            prev_t = t;
            return true;
        }        
    }

    return false;
}

double compute_abs_angle(int start_x, int start_y, int end_x, int end_y){
    double angle;

    int dot = start_x*end_x + start_y*end_y;      // dot product between [x1, y1] and [x2, y2]
    int det = start_x*end_y - start_y*end_x;      // determinant

    angle = atan2(det, dot);

    angle = angle*180/3.14; // [-180;+180]
    return angle;
}


Coordonnees closest(Coordonnees* pt, Coordonnees* vect_a, Coordonnees* vect_b) {

    Coordonnees result;

    float da = vect_b->y - vect_a->y;
    float db = vect_a->x - vect_b->x;
    float c1 = da*vect_a->x + db*vect_a->y;
    float c2 = -db*pt->x + da*pt->y;
    float det = da*da + db*db;
    float cx = 0;
    float cy = 0;

    if (det != 0) {
        result.x = (da*c1 - db*c2) / det;
        result.y = (da*c2 + db*c1) / det;
    } else {
        // The point is already on the line
        result.x = pt->x;
        result.y = pt->y;
    }

    return result;
}


void move(Pod* current_pod, float t){
    #ifdef PRINT_MOVE
    fprintf(stderr, "Moving [%d](%f:%f) [%d:%d]\n", current_pod, current_pod->unit.speed.x * t, current_pod->unit.speed.y * t);
    #endif
    // Movement: The speed vector is added to the position of the pod. If a collision would occur at this point, the pods rebound off each other.
    
    current_pod->unit.position.x += current_pod->unit.speed.x * t;
    current_pod->unit.position.y += current_pod->unit.speed.y * t;
}

void after_moved(Pod* current_pod){


    // Friction: the current speed vector of each pod is multiplied by 0.85
    current_pod->unit.speed.x *= 0.85;
    current_pod->unit.speed.y *= 0.85;

    // The speed's values are truncated and the position's values are rounded to the nearest integer.
    current_pod->unit.speed.x = (float) ((int) current_pod->unit.speed.x);
    current_pod->unit.speed.y = (float) ((int) current_pod->unit.speed.y);
    current_pod->unit.position.x = round(current_pod->unit.position.x);
    current_pod->unit.position.y = round(current_pod->unit.position.y);

    for(int cnt_1 = 0; cnt_1 < 4; cnt_1++){
        Players[cnt_1].dist_to_ckp = distance(&Players[cnt_1].unit.position, &race.checkpoints[Players[cnt_1].nextcheckpoint].position);
    }
        
    rank_players();

    current_pod->timeout -= 1;
}

void rank_players(){
    int ranks[4] = {0,-1,-1,-1};
    for(int cnt_1 = 1; cnt_1 < 4; cnt_1++){
        for(int cnt_2 = 0; cnt_2 < 4; cnt_2++){
            if(ranks[cnt_2] == - 1 || 
                Players[cnt_1].ckp_nbr*50000 - Players[cnt_1].dist_to_ckp >= 
                    Players[ranks[cnt_2]].ckp_nbr*50000 - Players[cnt_2].dist_to_ckp){
                for(int cnt_3 = 2 ; cnt_3 >= cnt_2; cnt_3--){
                    ranks[cnt_3+1]=ranks[cnt_3];
                }
                ranks[cnt_2] = cnt_1;
                break;
            }
        }
    }
    
    for(int cnt_1 = 0; cnt_1 < 4; cnt_1++){
        Players[ranks[cnt_1]].rank = cnt_1+1;
    }

    race.worst_teammate = (Players[0].rank<=Players[1].rank)?1:1;
    race.best_opp = (Players[2].rank<=Players[3].rank)?2:3;
    race.worst_opp = (race.best_opp==2)?3:2;
    race.best_teammate = (race.worst_teammate==0)?1:0;
}

void boost(Pod* current_pod, float thrust){
    // Acceleration: the pod's facing vector is multiplied by the given thrust value. The result is added to the current speed vector.
    
    if(current_pod->shield==0){
        float r_angle = current_pod->angle*M_PI/180;
        current_pod->unit.speed.x += cos(r_angle)*current_pod->action.thrust;
        current_pod->unit.speed.y += sin(r_angle)*current_pod->action.thrust;
    }
}

float getAngle(Coordonnees p, Coordonnees this) {
    float d = distance(&p, &this);
    float dx = (p.x - this.x) / d;
    float dy = (p.y - this.y) / d;

    // Simple trigonometry. We multiply by 180.0 / PI to convert radiants to degrees.
    float a = acos(dx) * 180.0 / M_PI;

    // If the point I want is below me, I have to shift the angle for it to be correct
    if (dy < 0) {
        a = 360.0 - a;
    }

    return a;
}

float diffAngle(Coordonnees p, Coordonnees this, float angle) {
    float a = getAngle(p, this);

    // To know whether we should turn clockwise or not we look at the two ways and keep the smallest
    // The ternary operators replace the use of a modulo operator which would be slower
    float right = angle <= a ? a - angle : 360.0 - angle + a;
    float left = angle >= a ? angle - a : angle + 360.0 - a;

    if (right < left) {
        return right;
    } else {
        // We return a negative angle if we must rotate to left
        return -left;
    }
}

void rotate(Pod* current_pod, Coordonnees* target){
    // Rotation: the pod rotates to face the target point, with a maximum of 18 degrees (except for the 1rst round).
    // float req_rotation_angle = compute_abs_angle(target->x - current_pod->unit.position.x, 
    //                                             target->y - current_pod->unit.position.y, 0, 1) 
    //                                 - current_pod->angle;
    float req_rotation_angle = diffAngle(*target, current_pod->unit.position, current_pod->angle);
    current_pod->angle += min(18,max(-18,req_rotation_angle));

    if(current_pod->angle>360){
        current_pod->angle -= 360;
    } else if(current_pod->angle<0){
        current_pod->angle += 360;
    }
}

bool FIRST_RUN = true;
bool BOOST_AVAILABLE = true;
int prev_x, prev_y;

void bounce_checkpoint(Pod* current_pod, Unit* p) {
    #ifdef PRINT_BOUNCE
    fprintf(stderr, "Bounce checkpoint %d \n",current_pod->nextcheckpoint);
    #endif
    if(race.checkpoints[current_pod->nextcheckpoint].position.x == p->position.x){
        current_pod->nextcheckpoint += 1;
        current_pod->nextcheckpoint = (current_pod->nextcheckpoint >= race.checkpoint_nbr)?0:current_pod->nextcheckpoint;
        current_pod->timout_to_last_ckp = current_pod->timeout;
        current_pod->timeout = 100;
        current_pod->ckp_nbr += 1;
        current_pod->checkpoints_checked += 1;
    }
}


void bounce_pod(Pod* p, Pod* current_pod) {


    #ifdef PRINT_BOUNCE
    fprintf(stderr, "Bounce [%d:%d]\n", &current_pod->unit, &p->unit);
    fprintf(stderr, "Units 1 --> (%f:%f)(%f:%f)\n", 
        current_pod->unit.position.x, 
        current_pod->unit.position.y, 
        current_pod->unit.speed.x, 
        current_pod->unit.speed.y);
    fprintf(stderr, "Units 2 --> (%f:%f)(%f:%f)\n", 
        p->unit.position.x, 
        p->unit.position.y, 
        p->unit.speed.x, 
        p->unit.speed.y);
    #endif

    // If a pod has its shield active its mass is 10 otherwise it's 1
    float m1 = current_pod->action.shield ? 10 : 1;
    float m2 = p->action.shield ? 10 : 1;
    float mcoeff = (m1 + m2) / (m1 * m2);

    float nx = current_pod->unit.position.x - p->unit.position.x;
    float ny = current_pod->unit.position.y - p->unit.position.y;

    // Square of the distance between the 2 pods. This value could be hardcoded because it is always 800²
    float nxnysquare = nx*nx + ny*ny;

    float dvx = current_pod->unit.speed.x - p->unit.speed.x;
    float dvy = current_pod->unit.speed.y - p->unit.speed.y;

    // fx and fy are the components of the impact vector. product is just there for optimisation purposes
    float product = nx*dvx + ny*dvy;
    float fx = (nx * product) / (nxnysquare * mcoeff);
    float fy = (ny * product) / (nxnysquare * mcoeff);

    // We apply the impact vector once
    current_pod->unit.speed.x -= fx / m1;
    current_pod->unit.speed.y -= fy / m1;
    p->unit.speed.x += fx / m2;
    p->unit.speed.y += fy / m2;

    // If the norm of the impact vector is less than 120, we normalize it to 120
    float impulse = sqrt(fx*fx + fy*fy);
    if (impulse < 120.0) {
        fx = fx * 120.0 / impulse;
        fy = fy * 120.0 / impulse;
    }

    // We apply the impact vector a second time
    current_pod->unit.speed.x -= fx / m1;
    current_pod->unit.speed.y -= fy / m1;
    p->unit.speed.x += fx / m2;
    p->unit.speed.y += fy / m2;


    current_pod->unit.position.x -= (fx / m1) / 50; //(current_pod->unit.speed.x>0)?10:-10;
    current_pod->unit.position.x -= ((fx / m1)>0)?1:-1;
    current_pod->unit.position.y -= (fy / m1) / 50; //(current_pod->unit.speed.x>0)?10:-10;
    current_pod->unit.position.y -= ((fy / m1)>0)?1:-1;
    //current_pod->unit.position.y += (current_pod->unit.speed.y>0)?10:-10;
    //p->unit.position.y += (p->unit.speed.y>0)?10:-10;
    //p->unit.position.x += (p->unit.speed.x>0)?10:-10;
    p->unit.position.x += (fx / m1) / 50; //(current_pod->unit.speed.x>0)?10:-10;
    p->unit.position.x += ((fx / m1)>0)?1:-1;
    p->unit.position.y += (fy / m1) / 50; //(current_pod->unit.speed.x>0)?10:-10;
    p->unit.position.y += ((fy / m1)>0)?1:-1;

    // This is one of the rare places where a Vector class would have made the code more readable.
    // But this place is called so often that I can't pay a performance price to make it more readable.

    #ifdef PRINT_BOUNCE

    fprintf(stderr, "Bounce (after)\n");
    fprintf(stderr, "Units 1 --> (%f:%f)(%f:%f)\n", 
        current_pod->unit.position.x, 
        current_pod->unit.position.y, 
        current_pod->unit.speed.x, 
        current_pod->unit.speed.y);
    fprintf(stderr, "Units 2 --> (%f:%f)(%f:%f)\n", 
        p->unit.position.x, 
        p->unit.position.y, 
        p->unit.speed.x, 
        p->unit.speed.y);

    #endif
}

//#define PRINT_PLAY
void play(Pod pods[], int pods_len, Unit checkpoints[], int checkpoints_len) {
    #ifdef PRINT_PLAY
    fprintf(stderr, "PLAY \n");
    #endif
    check_watchdog("PLAY");
    // This tracks the time during the turn. The goal is to reach 1.0
    float t = 0.0;
    int cnt = 0;

    Collision prev_col;
    bool first_col = true;

    while (t < 1.0) {
        cnt++;
        if(cnt>=200 && (cnt%100) == 0){
            fprintf(stderr, "CNT : %d\n",cnt);
            debug = true;
        }
        check_watchdog("While t < 0");
        #ifdef PRINT_PLAY
        fprintf(stderr, "t %f\n", t);
        #endif

        Collision firstCollision;
        firstCollision.pod_1 = NULL;
        firstCollision.pod_2 = NULL;
        int firstCollision_type;
        firstCollision.t = -1;

        // We look for all the collisions that are going to occur during the turn
        for (int i = 0; i < pods_len; i++) {
            // Collision with another pod?
            for (int j = i + 1; j < pods_len; j++) {
                //Collision col = pods[i].collision(pods[j]);
                Collision col; 
                if(collision(&pods[i].unit, &pods[j].unit, &col)){
                    if(first_col == true || 
                        !(prev_col.pod_1 == &(pods[i]) 
                            && prev_col.pod_2 == &(pods[j]) 
                            && prev_col.t == col.t)
                        ){
                        // If the collision occurs earlier than the one we currently have we keep it
                        if (col.t + t < 1.0 && (firstCollision.t == -1 || col.t < firstCollision.t)) {
                            firstCollision = col;
                            firstCollision.pod_1 = &(pods[i]);
                            firstCollision.pod_2 = &(pods[j]);
                            firstCollision_type = POD_TYPE;
                        }
                        first_col = false;
                    } else {
                        if(debug){
                            fprintf(stderr, "Discard col \n");
                        }
                    }
                }
            }

            // Collision with another checkpoint?
            // It is unnecessary to check all checkpoints here. We only test the pod's next checkpoint.
            // We could look for the collisions of the pod with all the checkpoints, but if such a collision happens it wouldn't impact the game in any way
            Collision col;

            // If the collision happens earlier than the current one we keep it
            if (collision(&pods[i].unit, &checkpoints[pods[i].nextcheckpoint], &col)){
                if(col.t + t < 1.0 && (firstCollision.t == -1 || col.t < firstCollision.t)) {
                    firstCollision = col;
                    firstCollision.pod_1 = &(pods[i]);
                    firstCollision_type = CHECKPOINT_TYPE;
                }
            } 
        }

        if (firstCollision.t == -1) {
            // No collision, we can move the pods until the end of the turn
            for (int i = 0; i < pods_len; ++i) {
                //pods[i].move(1.0 - t);

                move(&pods[i], 1.0 - t);
            }

            // End of the turn
            t = 1.0;
        } else {
            if(debug){
                fprintf(stderr, "PrevCol: [%d;%d](%f)\n",prev_col.pod_1,prev_col.pod_2,prev_col.t);
                fprintf(stderr, "firstCol: [%d;%d](%f)\n",firstCollision.pod_1,firstCollision.pod_2,firstCollision.t);
            }
            prev_col = firstCollision;
            // Move the pods to reach the time `t` of the collision
            for (int i = 0; i < pods_len; ++i) {
                move(&pods[i], firstCollision.t*0.99);
                //pods[i].move(firstCollision.t);
            }
            if(firstCollision_type == CHECKPOINT_TYPE){
                bounce_checkpoint(firstCollision.pod_1, firstCollision.unit_2);
            } else {
                bounce_pod(firstCollision.pod_1, firstCollision.pod_2);
            }
            // Play out the collision
            //firstCollision.unit_1.bounce(firstCollision.unit_2);

            t += firstCollision.t;
            // fprintf(stderr, "%f [%f:%f][%f:%f]\n",
            //         t,
            //         firstCollision.pod_1->unit.position.x,
            //         firstCollision.pod_1->unit.position.y,
            //         firstCollision.pod_2->unit.position.x,
            //         firstCollision.pod_2->unit.position.y
            //         );
 
       }
    }

    for (int i = 0; i < pods_len; ++i) {
        after_moved(&pods[i]);
    }
}

long evaluate(Pod* p) {

    // float a = diffAngle(race.checkpoints[((p->nextcheckpoint+1) % race.checkpoint_nbr)].position,
    //             p->unit.position,
    //             p->angle);
    
    // if (a > 180){
    //     a -= 360;
    //     a *= -1;
    // } else if (a < -180){
    //     a += 360;
    // } else if (a < 0){
    //     a *= -1;
    // }
    // a = 0;

    // float speed = sqrt(p->unit.speed.x*p->unit.speed.x + p->unit.speed.y*p->unit.speed.y)/100;
    // speed = 0;
    Coordonnees p2 = race.checkpoints[p->nextcheckpoint].position;
    Coordonnees n_ckp = race.checkpoints[(p->nextcheckpoint+1)%race.checkpoint_nbr].position;
    n_ckp.x -= p2.x;
    n_ckp.y -= p2.y;
    float k = 550/sqrt(n_ckp.x*n_ckp.x+n_ckp.y*n_ckp.y);
    p2.x += k*n_ckp.x;
    p2.y += k*n_ckp.y;
    //p2.x += p->unit.


    return p->checkpoints_checked*50000*(0.98+0.02*min(100.0-p->timeout,8)/8.0) 
    - distance(&p->unit.position,  &p2);// - a + speed
    /*- distance(&p->unit.position,  &race.checkpoints[((p->nextcheckpoint+1) % race.checkpoint_nbr)].position)/4;*/
    // - l'angle entre la direction du pod et le prochain ckp (ssi distance inf. a X)
}

long evaluation(){
    int worst_teammate = (Players[0].rank<=Players[1].rank)?1:1;
    int best_opp = (Players[2].rank<=Players[3].rank)?2:3;
    int worst_opp = (best_opp==2)?3:2;
    int best_teammate = (worst_teammate==0)?1:0;


    if(expla){
        fprintf(stderr, "Best opp:%d Hunter:%d \n", best_opp, worst_teammate);
    }

    Coordonnees next_target;

    if(distance2(&Players[best_opp].unit.position, &race.checkpoints[Players[best_opp].nextcheckpoint].position) >
        distance2(&Players[worst_teammate].unit.position, &race.checkpoints[Players[best_opp].nextcheckpoint].position)){
        next_target = Players[best_opp].unit.position; //.nextcheckpoint;
        next_target.x += Players[best_opp].unit.speed.x ;
        next_target.y += Players[best_opp].unit.speed.y ;
        if(expla){
            fprintf(stderr, "Hunter in front of target, interception ! [%d,%d] \n", (int)next_target.x, (int)next_target.y);
        }
    } else {
        Coordonnees half = race.checkpoints[((Players[best_opp].nextcheckpoint+1)%race.checkpoint_nbr)].position;
        half.x += race.checkpoints[Players[best_opp].nextcheckpoint].position.x;
        half.y += race.checkpoints[Players[best_opp].nextcheckpoint].position.y;
        half.x /= 2;
        half.y /= 2;

        next_target = half;
        if(expla){
            fprintf(stderr, "Hunter behind target ! [%d,%d] \n", (int)next_target.x, (int)next_target.y);
        }
    }



    Coordonnees tgt = Players[best_opp].unit.position;
    

    long pts = 0;

    if(distance2(&Players[best_opp].unit.position, &Players[worst_teammate].unit.position)<800*800){
        tgt.x += Players[best_opp].unit.speed.x * 0;
        tgt.y += Players[best_opp].unit.speed.y * 0;
        pts += distance(&Players[worst_teammate].unit.position, &tgt);
        if(expla){
            fprintf(stderr, "Target in view ! [%d,%d] \n", (int)tgt.x, (int)tgt.y);
        }
    } else if(distance2(&Players[worst_teammate].unit.position, &next_target)<1500*1500){
        //pts += distance(&Players[worst_teammate].unit.position, &Players[best_opp].unit.position);
        pts += diffAngle(Players[best_opp].unit.position, 
                        Players[worst_teammate].unit.position, 
                        Players[worst_teammate].angle);
        pts -= 10000;
        if(expla){
            fprintf(stderr, "Stby at rdv-point ! [%d,%d] \n", (int)next_target.x, (int)next_target.y);
        }
    } else {
        tgt.x += Players[best_opp].unit.speed.x * 2;
        tgt.y += Players[best_opp].unit.speed.y * 2;
        pts += distance(&Players[worst_teammate].unit.position, &next_target);
        if(expla){
            fprintf(stderr, "Far, go to rdv point ! [%d,%d] \n", (int)next_target.x, (int)next_target.y);
        }
    }

    float a = diffAngle(Players[best_opp].unit.position, 
                        Players[worst_teammate].unit.position, 
                        Players[worst_teammate].angle);
    float a2 = diffAngle(Players[best_teammate].unit.position, 
                        race.checkpoints[(Players[best_teammate].nextcheckpoint+1)%race.checkpoint_nbr].position, 
                        Players[best_teammate].angle);
    float a3 = diffAngle(Players[best_teammate].unit.position,
                        race.checkpoints[(Players[best_teammate].nextcheckpoint)].position, 
                        Players[best_teammate].angle);

    return 
        (evaluate(&Players[best_teammate]) - evaluate(&Players[best_opp]))*50
        - distance(&Players[worst_teammate].unit.position, 
                    &race.checkpoints[(Players[best_opp].nextcheckpoint)%race.checkpoint_nbr].position)
        - 5*pts 
        - a2*1 
        - a3*0;
}



void apply(Pod* current_pod, Move* mv){
    //fprintf(stderr, "a(%f) th(%d)\n", mv->angle, mv->thrust);

    float a = current_pod->angle + mv->angle;
    //a += 180;
    //a = ((int) a ) % 360;
    if (a >= 360.0) {
        a = a - 360.0;
    } else if (a < 0.0) {
        a += 360.0;
    }
    // Look for a point corresponding to the angle we want
    // Multiply by 10000.0 to limit rounding errors
    a = a * M_PI / 180.0;
    float px = current_pod->unit.position.x + cos(a) * 10000.0;
    float py = current_pod->unit.position.y + sin(a) * 10000.0;

    //rotate(current_pod, &current_pod->action.target);
    //boost(current_pod, current_pod->action.boost);
    current_pod->action.shield = mv->shield;
    current_pod->action.thrust = mv->thrust;
    current_pod->action.target.x = px;
    current_pod->action.target.y = py;
}

#define PLAYERS 4


// Copy the players in order to roll back to turn start situation after a simulation
void store(){
    for(int i = 0; i < PLAYERS; i++){
        Players_bkp[i] = Players[i];
        Players_bkp[i].unit = Players[i].unit;
        Players_bkp[i].unit.position = Players[i].unit.position;
        Players_bkp[i].unit.prev_position = Players[i].unit.prev_position;
        Players_bkp[i].unit.speed = Players[i].unit.speed;
        Players_bkp[i].action = Players[i].action;
        Players_bkp[i].action.target = Players[i].action.target;
    }
}

// Roll back the players to turn start situation after a simulation
void load(){
    #ifdef PRINT_LOAD
    fprintf(stderr,"LOAD \n");
    #endif
    for(int i = 0; i < PLAYERS; i++){
        Players[i] = Players_bkp[i];
        Players[i].unit = Players_bkp[i].unit;
        Players[i].unit.position = Players_bkp[i].unit.position;
        Players[i].unit.prev_position = Players_bkp[i].unit.prev_position;
        Players[i].unit.speed = Players_bkp[i].unit.speed;
        Players[i].action = Players_bkp[i].action;
        Players[i].action.target = Players_bkp[i].action.target;
    }
}


long naive_solution(Solution* c){

    check_watchdog("Start naive_solution");

    float a[2] = {0.0,0.0};
    float d[2] = {0.0,0.0};

    for(int j = 0; j<SOLUTION_LEN; j++){

        a[0] = diffAngle(race.checkpoints[Players[0].nextcheckpoint].position,
         Players[0].unit.position,
         Players[0].angle);
        a[1] = diffAngle(race.checkpoints[Players[1].nextcheckpoint].position,
                    Players[0].unit.position,
                    Players[0].angle);

        d[0] = distance(&Players[0].unit.position, &race.checkpoints[Players[0].nextcheckpoint].position);
        d[1] = distance(&Players[0].unit.position, &race.checkpoints[Players[1].nextcheckpoint].position);

        int turn_a, turn_t;
        for(int o = 0; o < 2; o++){
            if(a[o]>=18){
                if(a[o]>90){
                    turn_t = 0;
                } else {
                    turn_t = 100;
                }
                turn_a = 18;
                a[o] -= 18;
            } else if(a[o]<18 && a[o]>=-18) {
                turn_a = a[o];
                a[o] = 0;
                turn_t = 100;
            } else if(a[o]<-18) {
                if(a[o]<-90){
                    turn_t = 0;
                } else {
                    turn_t = 100;
                }
                turn_a = -18;
                a[o] += 18;
            }
            if(c==0){
                c->teamMoves[j].move_1.angle = turn_a;
                c->teamMoves[j].move_1.thrust = turn_t;
            } else if (c==1){
                c->teamMoves[j].move_2.angle = turn_a;
                c->teamMoves[j].move_2.thrust = turn_t;
            }
        }

        apply(&Players[0], &c->teamMoves[j].move_1);
        apply(&Players[1], &c->teamMoves[j].move_2);
        rotate(&Players[0], &Players[0].action.target);
        boost(&Players[0], Players[0].action.boost);
        rotate(&Players[1], &Players[1].action.target);
        boost(&Players[1], Players[1].action.boost);

        play(Players, PLAYERS, race.checkpoints, race.checkpoint_nbr);
    }

    long score = evaluation();

    load();

    return score;
}

Move simple_bot(Pod* p){

    Move mv;
    float a = diffAngle(race.checkpoints[p->nextcheckpoint].position,
                        p->unit.position, 
                        p->angle);

    if(a > 90){
        mv.thrust = 0;
    } else {
        mv.thrust = 100;
    }

    mv.angle = a;

    return mv;
}

long score_players(Solution* s){
    #ifdef PRINT_SCOREP
    fprintf(stderr, "Score players (len: %d)\n",s->len);
    #endif

    // Play out the turns
    for (int i = 0; i < s->len; i++) {

        if(Players[0].shield<0){
            s->teamMoves[0].move_1.shield = true;
            Players[0].shield += 1;
        }
        if(Players[1].shield<0){
            s->teamMoves[1].move_2.shield = true;
            Players[1].shield += 1;
        }
        Move p2 = simple_bot(&Players[2]);
        apply(&Players[2], &p2);
        Move p3 = simple_bot(&Players[3]);
        apply(&Players[3], &p3);

        // Apply the moves to the pods before playing
        apply(&Players[0], &s->teamMoves[i].move_1);
        apply(&Players[1], &s->teamMoves[i].move_2);

        // go toward the next checkpoint

        rotate(&Players[0], &Players[0].action.target);
        boost(&Players[0], Players[0].action.boost);
        rotate(&Players[1], &Players[1].action.target);
        boost(&Players[1], Players[1].action.boost);
        rotate(&Players[2], &Players[2].action.target);
        boost(&Players[2], Players[2].action.boost);
        rotate(&Players[3], &Players[3].action.target);
        boost(&Players[3], Players[3].action.boost);

        play(Players, PLAYERS, race.checkpoints, race.checkpoint_nbr);
    }

    // Compute the score
    long score = evaluation();
    s->score = score;
    //fprintf(stderr, "S: %d", (int) score);
    // Reset everyone to their initial states
    load();

    //fprintf(stderr, "(%d)\n", (int) score);

    return score;
}

void output(Pod* p, Move* move) {

    double d = distance(&p->unit.position, &race.checkpoints[p->nextcheckpoint].position);

    float a = p->angle + move->angle;
    float a_ckp = diffAngle(race.checkpoints[p->nextcheckpoint].position,
                 p->unit.position,
                 p->angle);

    bool boost = (p == &Players[0] && d>4000 && a_ckp*a_ckp < 100);
    //a += 180;
    //a = ((int) a ) % 360;
    if (a >= 360.0) {
        a = a - 360.0;
    } else if (a < 0.0) {
        a += 360.0;
    }
    fprintf(stderr, "print sol a(%f:%f:%f) thrust(%d)\n",p->angle, move->angle, a, move->thrust);
    // Look for a point corresponding to the angle we want
    // Multiply by 10000.0 to limit rounding errors
    a = a * M_PI / 180.0;
    float px = p->unit.position.x + cos(a) * 10000.0;
    float py = p->unit.position.y + sin(a) * 10000.0;
    FILE* fptr = fopen("output.csv","a");
    if(fptr == NULL)
    {
        fprintf(stderr, "Error!");   
        exit(1);             
    }
    if (move->shield) {
        #ifdef ONLINE
        printf("%d %d SHIELD\n", (int) round(px), (int) round(py));
        fprintf(stderr, "SHIELLLLLLDDDDDDDDD !!!!!!!!!!!!!!!!! (%d) \n", p->shield);
        #else
        fprintf(fptr, "%d %d SHIELD\n", (int) round(px), (int) round(py));
        fprintf(stderr, "%d %d SHIELD\n", (int) round(px), (int) round(py));
        #endif
    } else if (boost && p->boost_available) {
        p->boost_available = false;
        // fprintf(stderr, "print sol\n");
        fprintf(fptr, "print sol\n");
        #ifdef ONLINE
        printf("%d %d BOOST\n", (int) round(px), (int) round(py), (int) move->thrust);
        #else
        fprintf(fptr, "%d %d %d\n", (int) round(px), (int) round(py), (int) move->thrust);
        fprintf(stderr, "%d %d %d\n", (int) round(px), (int) round(py), (int) move->thrust);
        #endif
    } else {
        // fprintf(stderr, "print sol\n");
        fprintf(fptr, "print sol\n");
        #ifdef ONLINE
        printf("%d %d %d\n", (int) round(px), (int) round(py), (int) move->thrust);
        #else
        fprintf(fptr, "%d %d %d\n", (int) round(px), (int) round(py), (int) move->thrust);
        fprintf(stderr, "%d %d %d\n", (int) round(px), (int) round(py), (int) move->thrust);
        #endif
    }
    fclose(fptr);
}

void print_timestamp(){
    gettimeofday(&lap_check, NULL);
    delta_us = (unsigned long) 1000000 * (lap_check.tv_sec - lap_start_time.tv_sec) + (lap_check.tv_usec - lap_start_time.tv_usec);
    //fprintf(stderr, "(%lu)\n", delta_us);
}


void printSol(Solution* s){
    for(int j = 0; j<SOLUTION_LEN; j++){
        fprintf(stderr, "[%f:%d][%f:%d]\n",
            s->teamMoves[j].move_1.angle,
            s->teamMoves[j].move_1.thrust,
            s->teamMoves[j].move_2.angle,
            s->teamMoves[j].move_2.thrust);
    }
}


void crossover_s(Solution* s1, Solution* s2){
    for(int i = 0; i < s1->len; i++){
        if(rand()>0.5){
            s1->teamMoves[i].move_1 = s2->teamMoves[i].move_1;
        }
        if(rand()>0.5){
            s1->teamMoves[i].move_2 = s2->teamMoves[i].move_2;
        }
    }
}

void test_solutions(){


    Solution s[SOLUTIONS_NBR];

    for(int j = 0; j<SOLUTIONS_NBR;++j){
        for(int i = 0; i<SOLUTION_LEN; ++i){
            s[j].teamMoves[i].move_1.angle = 0.0;
            s[j].teamMoves[i].move_1.thrust = 0;
            s[j].teamMoves[i].move_2.angle = 0.0;
            s[j].teamMoves[i].move_2.thrust = 0;
        }
    }
    long mScore;
    print_timestamp();
    fprintf(stderr, "gen solutions\n");
    gen_solutions(s,SOLUTIONS_NBR,SOLUTION_LEN);

    int i = 0;
    int best_solution = 0;

    while (delta_us < 70000 && i < SOLUTIONS_NBR) {
        //Solution solution = s[i];
        //fprintf(stderr, "test solution n°%d (%da%dt)\n",i, (int) solution.teamMoves[0].move_1.angle, (int) solution.teamMoves[0].move_1.thrust);
        print_timestamp();
        long score = score_players(&s[i]);
        //fprintf(stderr, "Scr %d %d\n", mScore, score);
        if (i == 0 || score > mScore) {
            mScore = score;
            best_solution = i;
            fprintf(stderr, "new best solution (%d: %ld) (%lu) (i:%d)\n", best_solution, mScore, delta_us,i);
            for(int j = 0; j<SOLUTION_LEN; j++){
                fprintf(stderr, "[%f:%d][%f:%d]\n",
                    s[best_solution].teamMoves[j].move_1.angle,
                    s[best_solution].teamMoves[j].move_1.thrust,
                    s[best_solution].teamMoves[j].move_2.angle,
                    s[best_solution].teamMoves[j].move_2.thrust);
            }
        }
        print_timestamp();
        
        if(i<3){
            fprintf(stderr, "Solution %d score: %ld\n", i, score);
        }
        i++;
    }

    fprintf(stderr, "print best solution (%d: %ld) (%lu) (i:%d)\n", best_solution, mScore, delta_us,i);
    expla = true;
    score_players(&s[best_solution]);
    expla = false;
    // for(int j = 0; j<SOLUTION_LEN; j++){
    //     fprintf(stderr, "[%f:%d][%f:%d]\n",
    //         s[best_solution].teamMoves[j].move_1.angle,
    //         s[best_solution].teamMoves[j].move_1.thrust,
    //         s[best_solution].teamMoves[j].move_2.angle,
    //         s[best_solution].teamMoves[j].move_2.thrust);
    // }
    output(&Players[0], &s[best_solution].teamMoves[0].move_1);
    output(&Players[1], &s[best_solution].teamMoves[0].move_2);
    #ifdef OFFLINE
    move_1 = s[best_solution].teamMoves[0].move_1;
    move_2 = s[best_solution].teamMoves[0].move_2;
    #endif
}

void test_solutions_mutations(){
    
    Solution s[SOLUTIONS_NBR];

    for(int j = 0; j<SOLUTIONS_NBR;++j){
        for(int i = 0; i<SOLUTION_LEN; ++i){
            s[j].teamMoves[i].move_1.angle = 0.0;
            s[j].teamMoves[i].move_1.thrust = 0;
            s[j].teamMoves[i].move_2.angle = 0.0;
            s[j].teamMoves[i].move_2.thrust = 0;
        }
    }
    long mScore = -500000000;
    print_timestamp();
    fprintf(stderr, "gen solutions\n");
    gen_solutions_mut(s,MUT_SOLUTIONS_NBR,SOLUTION_LEN);
    int i = 0;
    int best_solution = 0;

    static bool first_test = true ;
    static Solution prev_best;

    if(!first_test){
        for(int m = 1; m<prev_best.len; m++){
            prev_best.teamMoves[m-1] = prev_best.teamMoves[m];
        }
        prev_best.len - 1;
        s[SOLUTIONS_NBR - 1] = prev_best;
    } else {
        first_test  = false;
    }
    

    while (delta_us < 70000) {
        long score = -5000000000;
        int minimumScore_index = -1;
        long minimumScore = 500000000;

        if(i>=MUT_SOLUTIONS_NBR){
            int rand_s = MUT_SOLUTIONS_NBR*(rand()/(RAND_MAX+1.0));
            Solution sol = s[rand_s];
            //fprintf(stderr,"Print mutation\n");
            //printSol(&sol);
            //mutate_s(&sol, (float) ((float) delta_us / 80000)/3);
            
            if(rand()>0.5){
                mutate_s(&sol, ((float) ((long) (((70000-delta_us)*4+1)/70000)))*0.2);
            } else {
                int s2 = rand_s + (int) (MUT_SOLUTIONS_NBR-1)*(rand()/(RAND_MAX+1.0));
                s2 = s2 %MUT_SOLUTIONS_NBR;
                crossover_s(&sol, &s[s2]);
            }
            //printSol(&sol);
            score = score_players(&sol);
            if(score == 0){
                fprintf(stderr, "Score Error\n");
            }
            //fprintf(stderr, "Score: %ld\n",score);
            // We find the smallest score in the solutions pool
            for(int j = 0; j < MUT_SOLUTIONS_NBR; j++){
                if(s[j].score < minimumScore){
                    minimumScore = s[j].score;
                    minimumScore_index = j;
                }
            }

            // and if the new score is higher, we replace the solution
            if(score > minimumScore){
                s[minimumScore_index] = sol;
            }
        } else {
            score = score_players(&s[i]);
        }

        if (score > mScore) {
            mScore = score;
            best_solution = (i>=MUT_SOLUTIONS_NBR)?minimumScore_index:i;
        }

        print_timestamp();

        i++;
    }

    expla = true;
    score_players(&s[best_solution]);
    expla = false;

    fprintf(stderr, "print best solution (%d: %ld) (%lu) (i:%d)\n", best_solution, mScore, delta_us,i);
    fprintf(stderr, "n°1: %dx%dy %da %d\n",
                    (int) Players[0].unit.position.x,
                    (int) Players[0].unit.position.y,
                    (int) Players[0].angle,
                    (int) Players[0].nextcheckpoint);
    fprintf(stderr, "n°2: %dx%dy %da %d\n",
                    (int) Players[1].unit.position.x,
                    (int) Players[1].unit.position.y,
                    (int) Players[1].angle,
                    (int) Players[1].nextcheckpoint);
    // for(int j = 0; j<SOLUTION_LEN; j++){
    //     fprintf(stderr, "[%f:%d][%f:%d]\n",
    //         s[best_solution].teamMoves[j].move_1.angle,
    //         s[best_solution].teamMoves[j].move_1.thrust,
    //         s[best_solution].teamMoves[j].move_2.angle,
    //         s[best_solution].teamMoves[j].move_2.thrust);
    // }
    output(&Players[0], &s[best_solution].teamMoves[0].move_1);
    output(&Players[1], &s[best_solution].teamMoves[0].move_2);
    #ifdef OFFLINE
    move_1 = s[best_solution].teamMoves[0].move_1;
    move_2 = s[best_solution].teamMoves[0].move_2;
    #endif
}





int main()
{

    // game loop
    while (1) {

        if(first_run){
            fprintf(stderr, "init\n");
            init();
            first_run = false;

            printf("%d %d 100\n",
                (int) race.checkpoints[1].position.x,
                (int) race.checkpoints[1].position.y);

            printf("%d %d 100\n",
                (int) race.checkpoints[1].position.x,
                (int) race.checkpoints[1].position.y);

        } else {
            // fprintf(stderr, "update\n");
            #ifdef ONLINE
            update();
            #endif

            #ifdef OFFLINE
            apply(&Players[0], &move_1);
            apply(&Players[1], &move_2);
            rotate(&Players[0], &Players[0].action.target);
            boost(&Players[0], Players[0].action.boost);
            rotate(&Players[1], &Players[1].action.target);
            boost(&Players[1], Players[1].action.boost);

            play(Players, PLAYERS, race.checkpoints, race.checkpoint_nbr);

            // for (int i = 0; i < 4; ++i) {
            //     move(&Players[i], 1.0);
            //     after_moved(&Players[i]);
            // }

            for(int count = 0; count < 4; count++){
                fprintf(stderr, "x:%f y:%f vx:%f vy:%f a:%f ckp_id:%d \n", 
                    Players[count].unit.position.x,
                    Players[count].unit.position.y,
                    Players[count].unit.speed.x,
                    Players[count].unit.speed.y,
                    Players[count].angle,
                    Players[count].nextcheckpoint);
            }

            static int cnt = 0;

            FILE* f2ptr = fopen("output_pos.csv","a");
            for(int count = 0; count < 4; count++){
                fprintf(f2ptr, "%d;%d;%d;%d;%d;%d;%d;%d\n",
                    count, 
                    cnt,
                    (int) Players[count].unit.position.x,
                    (int) Players[count].unit.position.y,
                    (int) Players[count].unit.speed.x,
                    (int) Players[count].unit.speed.y,
                    (int) Players[count].angle,
                    (int) Players[count].nextcheckpoint);
            }
            fclose(f2ptr);
            cnt += 1;



            #endif


            gettimeofday(&lap_start_time, NULL);
            print_timestamp();
            reset_watchdog();


            // fprintf(stderr, "store players\n");

            store();

            // fprintf(stderr, "test solutions\n");
            test_solutions_mutations();
        }

    }

    return 0;
}