/*
 * Box2D todo; 
 *   - Sound plays at the right location - not just load locations
 *   - get ai working right
 *   - make body between joints so chain can't go through walls
 *   - joint remove at a given index - not just all at once
 *   - fix bug where touch two peopel at once game crashes, because of null pointer.
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <GLUT/glut.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <Box2D/Box2D.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include <mxml.h>
#include <sqlite3.h>
#include "freetype_imp.h"
#include "vector2.h"
#include "physics.h"
#include "load_png.h"
#include "load_sound.h"
#include "sound_list.h"
#include "ai.h"
#include "game.h"
#include "chain.h"


#define ZOMBIE 0
#define Z_P 1
#define PERSON 2
#define P_Z 3
#define SAFE 4
#define DONE 5
#define SPIKE 6

#define STINK_PARTICLE_NUM 2

//Different Emotional states
#define NORMAL 0
#define SCARED 1

#define ATTACHED 0
#define NOT_ATTACHED 1

#define PERSON_RAD 2
#define PERSON_MASS 0.5

#define VIEWRATIO 100
#define MAX_TIME 3


void gm_update_view(game gm, b2Vec2 center);

//Collision Filtering Zombies

const uint16 k_hero_cat = 0x0002;
const uint16 k_safe_person_cat = 0x0003;
const uint16 k_chain_person_cat = 0x0004;
const uint16 k_person_cat = 0x0005;
const uint16 k_zombie_cat = 0x0006;
const uint16 k_safe_zone_cage_cat = 0x0007;
const uint16 k_safe_zone_sensor_cat = 0x0008;
const uint16 k_open_portal_cat = 0x0009;
const uint16 k_open_portal_sensor_cat = 0x000A;
const uint16 k_closed_portal_cat = 0x000B;
const uint16 k_walls_cat = 0x000C;

const uint16 k_hero_mask = k_hero_cat | 
                           k_safe_person_cat | 
                           k_chain_person_cat | 
                           k_person_cat | 
                           k_zombie_cat | 
                           k_safe_zone_sensor_cat | 
                           k_open_portal_sensor_cat | 
                           k_closed_portal_cat | 
                           k_walls_cat;

const uint16 k_safe_person_mask = k_safe_person_cat | 
    k_chain_person_cat | 
    k_person_cat | 
    k_safe_zone_cage_cat | 
    k_closed_portal_cat | 
    k_walls_cat;

const uint16 k_chain_person_mask = k_hero_cat | 
    k_safe_person_cat | 
    k_chain_person_cat | 
    k_person_cat | 
    k_zombie_cat | 
    k_safe_zone_sensor_cat | 
    k_closed_portal_cat | 
    k_walls_cat;

const uint16 k_person_mask = k_hero_cat | 
    k_safe_person_cat | 
    k_chain_person_cat | 
    k_person_cat | 
    k_zombie_cat | 
    k_safe_zone_cage_cat | 
    k_closed_portal_cat | 
    k_walls_cat;

const uint16 k_zombie_mask = k_person_mask;

const uint16 k_safe_zone_cage_mask = k_person_cat;

const uint16 k_safe_zone_sensor_mask = k_hero_cat | k_chain_person_cat;

const uint16 k_open_portal_sensor_mask = k_hero_cat;
const uint16 k_open_portal_mask = 0;

const uint16 k_closed_portal_mask = k_hero_cat | k_safe_person_cat | k_chain_person_cat | k_person_cat | k_zombie_cat;

const uint16 k_walls_mask = k_hero_cat | k_safe_person_cat | k_chain_person_cat | k_person_cat | k_zombie_cat;

enum{
    al_saved1_buf,
	al_saved2_buf,
    al_wall_buf,
    al_pwall_buf,
    al_scared_buf,
    al_pdeath_buf,
    al_attached1_buf,
	al_attached2_buf,
	al_attached3_buf,
    al_hdeath_buf,
    al_p_z_buf,
    al_buf_num
};

//For the whoami type
enum{
    t_hero,
    t_portal,
    t_person,
    t_safezone,
    t_wall,
    t_spike,
    t_zombie,
    t_gravity_area
};

enum{
    t_circle,
    t_rect
};

enum{
    gl_hero0,
    gl_hero25,
    gl_hero50,
    gl_hero100,
	gl_smart_zombie,
	gl_zombie_square,
	gl_shield,
	gl_portal_closed,
	gl_portal_done,
	gl_spike_ball,
	gl_gravity,
    gl_num
};



//A dummy type to cast to 
//to check what the pointer is 
//pointing to
//This means this has to be the first things in all 
//the other struct so the pointer goes to the right place in
//the memory
typedef struct {
    int whoami;
    b2Vec2 grav;
} actor_type; //actor_type

typedef struct {
    int whoami;
    object o;
    float tr;
    int sensor_touch;
	b2Body* bod;
	b2Body* cage;
} _safe_zone; //actor_type

typedef struct {
    int whoami;
    b2Vec2 grav;
    b2Vec2 dim;
	b2Body* bod;
} _gravity_area; //actor_type

struct ppl {
    int whoami;
    b2Vec2 grav;

	object o;
	float timer;
	int state; 
	int emo; 
	int ready;
    double mx_f;

    int shape;
    float h,w;

    int chase;
    int parent_id;

    bool path_on;
    AiPath * path;

	b2Body * bod;
    
    ppl(){
        path = NULL;
    }
};

typedef struct {
    int whoami;
    b2Vec2 grav;

	object o;
	
	float timer;
	int state; 
	
	int spring_state;
	int person_id;
	
	float nrg;

	b2Body* bod;
} _hero;

typedef struct {
    int whoami;
	object o;
	char * lvl_path;
	char * name;
	int save_count; 
    int max_saved;
    int open;
    b2Body * bod;
} _portal;


typedef struct {
    object n[STINK_PARTICLE_NUM];
    double time[STINK_PARTICLE_NUM];
    int id;
} stink_struct;

typedef struct gametype {
	char * res_path;
	char * res_buf;
	char * db_path;
	
    vector2 ms;  /*mouse location in world*/
	int mpx; /* Mouse location on SDL context */
	int mpy; /* Mouse location on SDL context */
	int m;		 /*Whether to use mouse or not */
	vector2 ak;  /*arrow key presses*/
	int n;  /*checks if nrg key is being pressed*/
	
	vector2 vmin; /*sceen size */
	vector2 vmax; /*sceen size */
	int screenx;
    int screeny;
	double viewratio;
	int zoom;
	
	
	double timer;
	double total_time;

    int c; //Varibale to keep track if the c key is being pressed down.
	
	double h,w; //Height and width of the level
	ppl person[255]; //Zombies and people array
	int person_num; //How many there are
	_hero hero;

    int gravity_area_num;
    _gravity_area gravity_area[100];

    //Keeps track of who is in the hero chain
    Chain chain;

	_safe_zone safe_zone;
	int save_count;  //How many people you have to save to win the level

	line walls[1000];
	int wall_num;

    //This is for the overworld
	_portal portal[100];
	int portal_num;
    int onpt; //Portal index hero is on


    //Experiental Particle thingy for zombies
    stink_struct stnk[100];
    int stnk_num;

    int gm_state;

    //Sound
	ALuint buf[al_buf_num];
    s_list saved_src;
	
    //Textures
	GLuint h_tex[gl_num];


    //Make move these to an array like above
    //figured that out after did this.
	GLuint zombie_tex;
	GLuint person_tex;
	GLuint safe_tex;
	GLuint eye_tex;
	GLuint safezone_tex;
	GLuint hero_tex;
	GLuint heroattached_tex;
	GLuint herosafe_tex;
	GLuint person_s_tex;
	GLuint p_z_tex;
	GLuint h_z_tex;
	GLuint hzombie_tex;
	GLuint bk;
	GLuint rope_tex;
	GLuint blank_tex;
    GLuint extra_tex;
    GLuint stink_tex;

    rat_font * font;


    //Box2D Stuff

	b2World* m_world;
	b2Body* b_walls;
	
} gametype;

MyContactListener CL;

void stink_add(game gm, int id);
void stink_step(game gm, double dt);
void stink_render(game gm);
void ai_functions(game gm);
void gm_update_mouse(game gm);
void draw_line(b2Vec2 p1, b2Vec2 p2, GLuint tex);


game gm_init(char * res_path){
  gametype * gm;
  gm = (game)malloc(sizeof(gametype));
  if(!gm) {return NULL;}

  gm->res_path = (char*)malloc(strlen(res_path)*sizeof(char)+sizeof(char)*50);
  gm->res_buf  = (char*)malloc(strlen(res_path)*sizeof(char)+sizeof(char)*50);
  strcpy(gm->res_path, res_path);
  printf("game.c - path mem: %ld\n", strlen(res_path)*sizeof(char)+sizeof(char)*50);
	
  gm->viewratio = VIEWRATIO;
  gm->zoom = 0;
  gm->timer = 0;
  gm->c=0;
  gm->m=0;
  gm->n=0;
  gm->onpt=-1;


  b2Vec2 gravity(0.0f, 0.0f);
  gm->m_world = new b2World(gravity);
  
  CL.SetGM(gm);

  return gm;
}


void gm_set_db_string(game gm, char * db_path){
  gm->db_path = (char*)malloc(strlen(db_path)*sizeof(char)+sizeof(char)*2);
  strcpy(gm->db_path, db_path);
}

int gm_init_textures(game gm){
    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/zombie.png");
    load_texture(gm->res_buf, &gm->zombie_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/person_s.png");
    load_texture(gm->res_buf, &gm->person_s_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/person.png");
    load_texture(gm->res_buf, &gm->person_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/extra.png");
    load_texture(gm->res_buf, &gm->safe_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/eye.png");
    load_texture(gm->res_buf, &gm->eye_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/safezone.png");
    load_texture(gm->res_buf, &gm->safezone_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/hero.png");
    load_texture(gm->res_buf, &gm->hero_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/p_z.png");
    load_texture(gm->res_buf, &gm->p_z_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/h_z.png");
    load_texture(gm->res_buf, &gm->h_z_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/hzombie.png");
    load_texture(gm->res_buf, &gm->hzombie_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/heroattached.png");
    load_texture(gm->res_buf, &gm->heroattached_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/herosafe.png");
    load_texture(gm->res_buf, &gm->herosafe_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/rope.png");
    load_texture(gm->res_buf, &gm->rope_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/blank.png");
    load_texture(gm->res_buf, &gm->blank_tex);


    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/extra.png");
    load_texture(gm->res_buf, &gm->extra_tex);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/stink.png");
    load_texture(gm->res_buf, &gm->stink_tex);

/******/
    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/heron0.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_hero0]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/heron25.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_hero25]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/heron50.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_hero50]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/heron100.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_hero100]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/zombie_smart.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_smart_zombie]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/shield.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_shield]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/portalclosed.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_portal_closed]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/portaldone.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_portal_done]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/spikeball.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_spike_ball]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/zombie_square.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_zombie_square]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/gravity.png");
    load_texture(gm->res_buf, &gm->h_tex[gl_gravity]);

    gm->font = rat_init();
    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/imgs/FreeUniversal.ttf");
    rat_load_font(gm->font, gm->res_buf, 28);
}	

void gm_init_sounds(game gm){
	alGenBuffers(al_buf_num, gm->buf);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf,"/snd/saved1.ogg"); 
    snd_load_file(gm->res_buf, gm->buf[al_saved1_buf]);
	
    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/snd/saved2.ogg");
    snd_load_file(gm->res_buf, gm->buf[al_saved2_buf]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/snd/wall.ogg");
    snd_load_file(gm->res_buf, gm->buf[al_wall_buf]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/snd/pdeath.ogg");
    snd_load_file(gm->res_buf, gm->buf[al_pdeath_buf]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/snd/attached1.ogg");
    snd_load_file(gm->res_buf, gm->buf[al_attached1_buf]);
	
    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/snd/attached2.ogg");
    snd_load_file(gm->res_buf, gm->buf[al_attached2_buf]);
	
    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/snd/attached3.ogg");
    snd_load_file(gm->res_buf, gm->buf[al_attached3_buf]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/snd/scared.ogg");
    snd_load_file(gm->res_buf, gm->buf[al_scared_buf]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/snd/p_z.ogg");
    snd_load_file(gm->res_buf, gm->buf[al_p_z_buf]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/snd/hdeath.ogg");
    snd_load_file(gm->res_buf, gm->buf[al_hdeath_buf]);

    strcpy(gm->res_buf, gm->res_path);
    strcat(gm->res_buf, "/snd/pwall.ogg");
    snd_load_file(gm->res_buf, gm->buf[al_pwall_buf]);

	gm->saved_src = s_init();

	ALfloat	listenerOri[]={0.0,1.0,0.0, 0.0,0.0,1.0};
	alListenerfv(AL_ORIENTATION,listenerOri);
	alListenerf(AL_GAIN,12);
	alListener3f(AL_POSITION, gm->hero.o.p.x, 0, gm->hero.o.p.y);
	alListener3f(AL_VELOCITY, 0, 0, 0);
}

void gm_set_view(int width, int height, game gm){
	double ratio = (double)width/(double)height; 
	vector2 w = {gm->viewratio, gm->viewratio};
	w.x = ratio * w.y;
	gm->vmin.x = gm->hero.o.p.x - w.x/2.0f;
	gm->vmin.y = gm->hero.o.p.y - w.y/2.0f;
	gm->vmax.x = gm->hero.o.p.x + w.x/2.0f;
	gm->vmax.y = gm->hero.o.p.y + w.y/2.0f;
	
	gm->screenx = width;
    gm->screeny = height;
}

void gm_update_view(game gm, b2Vec2 center){
    double VIEW_EDGE = 7.0f/16.0f;
	double minx = VIEW_EDGE*(gm->vmax.x - gm->vmin.x) + gm->vmin.x;
	double maxx = gm->vmax.x - VIEW_EDGE*(gm->vmax.x - gm->vmin.x);
	
	if(center.x < minx && gm->vmin.x > 0){
		gm->vmin.x -= minx - center.x;
		gm->vmax.x -= minx - center.x;
	}
	
	else if (center.x > maxx && gm->vmax.x < gm->w){
		gm->vmin.x += center.x - maxx;
		gm->vmax.x += center.x - maxx;
	}
	
	double miny = VIEW_EDGE*(gm->vmax.y - gm->vmin.y) + gm->vmin.y;
	double maxy = gm->vmax.y - VIEW_EDGE*(gm->vmax.y - gm->vmin.y);
	if(center.y < miny  && gm->vmin.y > 0){
		gm->vmin.y -= miny - center.y;
		gm->vmax.y -= miny - center.y;
	}
	
	else if (center.y > maxy  && gm->vmax.y < gm->h){
		gm->vmin.y += center.y - maxy;
		gm->vmax.y += center.y - maxy;
	}
	
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(gm->vmin.x, gm->vmax.x, gm->vmin.y, gm->vmax.y);
    glMatrixMode(GL_MODELVIEW);
}


void gm_reshape(game gm, int width, int height){
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	double ratio = glutGet(GLUT_WINDOW_WIDTH)/(double)glutGet(GLUT_WINDOW_HEIGHT);
	vector2 w = {gm->viewratio, gm->viewratio};
	w.x = ratio * w.y;
	double diff = w.x - (gm->vmax.x - gm->vmin.x);
	gm->vmax.x += diff;
    glLoadIdentity();
    gluOrtho2D(gm->vmin.x, gm->vmax.x, gm->vmin.y, gm->vmax.y);
    glMatrixMode(GL_MODELVIEW);
}

void gm_update_sound(game gm){
	
}

void gm_update(game gm, int width, int height, double dt){
	int i;
	/*Timers */
	if (dt > 0.1f){dt = 0.01;}

    stink_step(gm, dt);	

	if(gm->gm_state == 0){
		gm->timer += dt;
	}
	
	for(i = 0; i < gm->person_num; i++){
		gm->person[i].timer -= dt;
		if(gm->person[i].state == P_Z && gm->person[i].timer <= 0.0f){
			gm->person[i].state = ZOMBIE;
            stink_add(gm, i);
            s_add_snd(gm->saved_src, gm->buf[al_p_z_buf], &gm->person[i].o, 1, 1);
		}
	}
	gm->hero.timer -= dt;
	if(gm->hero.state == P_Z && gm->hero.timer <= 0.0f){
		gm->hero.state = ZOMBIE;
        s_add_snd(gm->saved_src, gm->buf[al_p_z_buf], &gm->hero.o, 0.1, 1);
	}

    //NRG Timer for hero
    if(gm->hero.nrg < 25){
        gm->n = 0;
    }
    else if(gm->n && gm->hero.nrg > 25){
        gm->hero.nrg -= dt*10.0f;
    }
    if(gm->hero.nrg < 100 && gm->hero.state == PERSON){
        gm->hero.nrg += dt*5;
    }

    if(gm->hero.nrg < 0 && gm->hero.state == PERSON){
        gm->hero.nrg = 0;
        gm->hero.spring_state = NOT_ATTACHED;
        gm->hero.timer = MAX_TIME;
        b2Filter flt;
        flt.categoryBits = k_person_cat;
        flt.maskBits = k_person_mask;
        gm->hero.bod->GetFixtureList()->SetFilterData(flt);
        gm->hero.state = P_Z;
        chain_ready_zero(gm);
        s_add_snd(gm->saved_src, gm->buf[al_hdeath_buf], &gm->hero.o,0.1, 1);
    }


    //Ai Stuff
    ai_functions(gm);

	alListener3f(AL_POSITION, gm->hero.o.p.x, gm->hero.o.p.y, 0);
    s_update(gm->saved_src);


    //Box2d stuff
    double msd = v2Len(v2Sub(gm->hero.o.p, gm->ms));
    vector2 msu = v2Unit(v2Sub(gm->hero.o.p, gm->ms));
	
    float hf = (gm->n)?300:100;
    vector2 msf = v2sMul(-1*hf, msu);

    if(msd < gm->hero.o.r){
        msf.x = 0;
        msf.y = 0;
    }

	if(!gm->m){
		msf.x = 0;
		msf.y = 0;	
	}


	b2Vec2 f(gm->ak.x*hf + msf.x, gm->ak.y*hf + msf.y);
	b2Vec2 p = gm->hero.bod->GetPosition();

    f.x += gm->hero.grav.x;
    f.y += gm->hero.grav.y;

	gm->hero.bod->ApplyForce(f, p);


	for(i=0; i<gm->person_num; i++){
        p = gm->person[i].bod->GetPosition();
        f = gm->person[i].bod->GetLinearVelocity();
        f.Normalize();
        f *= gm->person[i].mx_f;
        f.x += gm->person[i].o.f.x;
        f.y += gm->person[i].o.f.y;
        f.x += gm->person[i].grav.x;
        f.y += gm->person[i].grav.y;
        gm->person[i].bod->ApplyForce(f,p);

    }

    gm->chain.qnum = 0;
    gm->chain.dnum = 0;


	gm->m_world->Step(dt, 8, 3);

    //CHeck for joints to create
    chain_update(gm);


    //Check to see if safe zone sensor is active
    if(gm->safe_zone.sensor_touch > 0){
        b2ContactEdge * SensorContact = gm->safe_zone.bod->GetContactList();
		
        while(SensorContact != NULL){
            actor_type * actor = (actor_type*)SensorContact->other->GetFixtureList()->GetUserData();
			if (actor != NULL) {
				if(actor->whoami == t_person){
					ppl * person = (ppl*)actor;
					_safe_zone * safe_zone = &gm->safe_zone;
					if(person->state == PERSON){
						if((person->bod->GetPosition() - safe_zone->bod->GetPosition()).LengthSquared() < (safe_zone->tr - person->o.r)*(safe_zone->tr - person->o.r)){
							b2Filter flt;
							flt.categoryBits = k_person_cat;
							flt.maskBits = k_person_mask;
							person->bod->GetFixtureList()->SetFilterData(flt);
							person->state = SAFE;
                            int rand_num = rand()%2;
                            if (rand_num == 0) {
                                s_add_snd(gm->saved_src, gm->buf[al_saved1_buf], &person->o,1, 1);
                            }
                            else {
                                s_add_snd(gm->saved_src, gm->buf[al_saved2_buf], &person->o,1, 1);
                            }

                            int index = person - gm->person;
                            chain_remove(gm, index);
						}
					}
					else if(person->state == P_Z || person->state == ZOMBIE){
						b2Vec2 pp = person->bod->GetPosition();
						b2Vec2 sp = safe_zone->bod->GetPosition();
						person->bod->ApplyForce(pp-sp, pp);
					}
				}
				else if(actor->whoami == t_hero){
					_hero * person = (_hero*)actor;
					_safe_zone * safe_zone = &gm->safe_zone;
					
					
					if((person->bod->GetPosition() - safe_zone->bod->GetPosition()).LengthSquared() < 
					   (safe_zone->tr - person->o.r)*(safe_zone->tr - person->o.r) && person->state != DONE){
						person->state = SAFE;
					}
					else if(person->state == SAFE){
						person->state = PERSON;
					}
				}	
			}
            SensorContact = SensorContact->next;
        }

    }

    //Check for joints to destroy
    for(i = 0; i < gm->chain.dnum; i++){
      gm->m_world->DestroyBody(gm->chain.delete_q[i]);
    }
	
	/*Advances our view box when zooming in and out */
	double ratio = (double)width/(double)height; 
	vector2 w = {gm->viewratio, gm->viewratio};
	w.x = ratio * w.y;
	
	gm->vmin.x -= gm->zoom*dt*1*w.x; 
	gm->vmin.y -= gm->zoom*dt*1*w.y;
	gm->vmax.x += gm->zoom*dt*1*w.x;
	gm->vmax.y += gm->zoom*dt*1*w.y;
	
	gm->viewratio = gm->vmax.y - gm->vmin.y;
	
	
	
	gm_update_mouse(gm);
}


void gm_render(game gm){
	b2Vec2 center = gm->hero.bod->GetPosition();
    gm_update_view(gm, center);
	int i;
	glColor3f(0.8,0.8,0.8);
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBindTexture( GL_TEXTURE_2D, gm->bk);
	glPushMatrix();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2f(0,0);
	glTexCoord2f(0.0, 1.0); glVertex2f(0, gm->h);
	glTexCoord2f(1.0, 1.0); glVertex2f(gm->w, gm->h);
	glTexCoord2f(1.0, 0.0); glVertex2f(gm->w, 0);
	glEnd();
	glPopMatrix();
		
	glBindTexture( GL_TEXTURE_2D, gm->safezone_tex);
	glPushMatrix();
	glTranslatef(gm->safe_zone.o.p.x, gm->safe_zone.o.p.y, 0);
	glScalef(gm->safe_zone.o.r, gm->safe_zone.o.r,0);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0, -1.0, 0.0);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0, 1.0, 0.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0, 1.0, 0.0);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0, -1.0, 0.0);
	glEnd();
	glPopMatrix();

    float len,hi;
    for(i = 0; i<gm->portal_num; i++){
        glPushMatrix();
        if(gm->portal[i].open && gm->portal[i].max_saved == 0){
            glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_shield]);
        }
        else if(gm->portal[i].open && gm->portal[i].max_saved > 0){
            glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_portal_done]);
        }
        else{
            glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_portal_closed]);
        }
        glTranslatef(gm->portal[i].o.p.x, gm->portal[i].o.p.y, 0);
        glScalef(gm->portal[i].o.r, gm->portal[i].o.r,0);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-1.0, -1.0, 0.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-1.0, 1.0, 0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(1.0, 1.0, 0.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(1.0, -1.0, 0.0);
        glEnd();
        glPopMatrix();
        
        float scale = 0.1;
        if(gm->portal[i].open && gm->portal[i].max_saved ==0){
            glPushMatrix();
            len = rat_font_text_length(gm->font, gm->portal[i].name);
            hi = rat_font_height(gm->font);
            glTranslatef(gm->portal[i].o.p.x - len/2.0f*scale,gm->portal[i].o.p.y+hi/2.0f*scale, 0);
            glScalef(scale,scale,0);
            rat_font_render_text(gm->font,0,0,gm->portal[i].name);
            glPopMatrix();
        }
        else if(gm->portal[i].open && gm->portal[i].max_saved > 0){
            glPushMatrix();
            char sc_buf[6];
            sprintf(sc_buf, "%d", gm->portal[i].max_saved);
            len = rat_font_text_length(gm->font, sc_buf);
            hi = rat_font_height(gm->font);
            glTranslatef(gm->portal[i].o.p.x - len/2.0f*scale,gm->portal[i].o.p.y+hi/2.0f*scale, 0);
            glScalef(scale,scale,0);
            rat_font_render_text(gm->font,0,0,sc_buf);
            glPopMatrix();
        }
        else{
            glPushMatrix();
            char sc_buf[6];
            sprintf(sc_buf, "%d", gm->portal[i].save_count);
            len = rat_font_text_length(gm->font, sc_buf);
            hi = rat_font_height(gm->font);
            glTranslatef(gm->portal[i].o.p.x - len/2.0f*scale,gm->portal[i].o.p.y+hi/2.0f*scale, 0);
            glScalef(scale,scale,0);
            rat_font_render_text(gm->font,0,0,sc_buf);
            glPopMatrix();
        }
    }
    
    for(i = 0; i < gm->gravity_area_num; i++){
        glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_gravity]);
        b2Vec2 p   = gm->gravity_area[i].bod->GetPosition();
        float  rot = gm->gravity_area[i].bod->GetAngle();
		glPushMatrix();
		glTranslatef(p.x, p.y, 0);
        glRotatef(rot*180/M_PI, 0, 0, 1);
        glScalef(gm->gravity_area[i].dim.x, gm->gravity_area[i].dim.y,0);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0, -1.0, 0.0);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0, 1.0, 0.0);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0, 1.0, 0.0);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0, -1.0, 0.0);
		glEnd();
		glPopMatrix();
    }  

	for(i = 0; i<gm->wall_num; i++){
		
        b2Vec2 pt1,pt2;

		pt1.x = gm->walls[i].p1.x;
		pt1.y = gm->walls[i].p1.y;
		pt2.x = gm->walls[i].p2.x;
		pt2.y = gm->walls[i].p2.y;

        draw_line(pt1,pt2,gm->rope_tex);

	}

  
	
	int k;
    glBindTexture( GL_TEXTURE_2D, gm->rope_tex);
    for(k = 0; k < gm->chain.num; k++){
        b2Vec2 p = gm->chain.links[k]->GetPosition();
        float th = gm->chain.links[k]->GetAngle();
        glPushMatrix();
        glTranslatef(p.x, p.y, 0);
        glRotatef(th*180/M_PI, 0, 0, 1);
        glScalef(3.0f,0.2f,0);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-1.0, -1.0, 0.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-1.0, 1.0, 0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(1.0, 1.0, 0.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(1.0, -1.0, 0.0);
        glEnd();
        glPopMatrix();
    }

    stink_render(gm);
	
	switch(gm->hero.state){
		case PERSON:
			glBindTexture( GL_TEXTURE_2D, gm->hero_tex);
			break;
		case P_Z:
			glBindTexture( GL_TEXTURE_2D, gm->h_z_tex);
			break;
		case ZOMBIE:
			glBindTexture( GL_TEXTURE_2D, gm->hzombie_tex);
			break;
		case SAFE:
		case DONE:
			glBindTexture( GL_TEXTURE_2D, gm->herosafe_tex);
			break;
	}
	if(gm->hero.spring_state == ATTACHED){
		glBindTexture( GL_TEXTURE_2D, gm->heroattached_tex);
	}
    if(gm->hero.nrg > 99){
        glBindTexture( GL_TEXTURE_2D, gm->herosafe_tex);
    }
    else if(gm->hero.nrg > 75 && gm->hero.nrg < 99){
        glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_hero100]);
    }
    else if(gm->hero.nrg > 50 && gm->hero.nrg < 75){
        glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_hero50]);
    }
    else if(gm->hero.nrg > 25 && gm->hero.nrg < 50){
        glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_hero25]);
    }
    else if(gm->hero.nrg > 0 && gm->hero.nrg < 25){
        glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_hero0]);
    }
	
	
	b2Vec2 p = gm->hero.bod->GetPosition();
	float rot = gm->hero.bod->GetAngle();
	glPushMatrix();
	glTranslatef(p.x, p.y, 0);
	glScalef(gm->hero.o.r, gm->hero.o.r,0);
	glRotatef(rot*180/M_PI, 0, 0, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0, -1.0, 0.0);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0, 1.0, 0.0);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0, 1.0, 0.0);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0, -1.0, 0.0);
	glEnd();
	glPopMatrix();



	for(i=0; i<gm->person_num; i++){
		if(gm->person[i].state == PERSON && gm->person[i].emo == NORMAL){
			glBindTexture( GL_TEXTURE_2D, gm->person_tex);
		}
        else if(gm->person[i].state == PERSON && gm->person[i].emo == SCARED){
			glBindTexture( GL_TEXTURE_2D, gm->person_s_tex);
		}
		else if(gm->person[i].state == P_Z){
			glBindTexture( GL_TEXTURE_2D, gm->p_z_tex);
		}
		else if(gm->person[i].state == ZOMBIE && gm->person[i].o.r <= 2){
			glBindTexture( GL_TEXTURE_2D, gm->zombie_tex);
		}
		else if(gm->person[i].state == ZOMBIE && gm->person[i].o.r > 2){
			glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_smart_zombie]);
		}
		else if(gm->person[i].state == SAFE){
			glBindTexture( GL_TEXTURE_2D, gm->safe_tex);
		}
		else if(gm->person[i].state == SPIKE){
			glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_spike_ball]);
		}
		if(gm->person[i].shape == t_rect){
			glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_zombie_square]);
        }
        p = gm->person[i].bod->GetPosition();
        rot = gm->person[i].bod->GetAngle();
		glPushMatrix();
		glTranslatef(p.x, p.y, 0);
        glRotatef(rot*180/M_PI, 0, 0, 1);
		if(gm->person[i].shape == t_rect){
            glScalef(gm->person[i].w, gm->person[i].h,0);
        } else{
            glScalef(gm->person[i].o.r, gm->person[i].o.r,0);
        }
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0, -1.0, 0.0);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0, 1.0, 0.0);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0, 1.0, 0.0);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0, -1.0, 0.0);
		glEnd();
		glPopMatrix();
		
	}

    /*
	for(i=0; i<gm->person_num; i++){
        if(gm->person[i].path != NULL){
            int k = 0;
            for(k=0; k<gm->person[i].path->p_num; k++){
                if(k == gm->person[i].path->marker){
                    glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_spike_ball]);
                }else{
                    glBindTexture( GL_TEXTURE_2D, gm->h_tex[gl_shield]);
                }
                b2Vec2 p = gm->person[i].path->p[k];
                glPushMatrix();
                glTranslatef(p.x, p.y, 0);
                glRotatef(180/M_PI, 0, 0, 1);
                glScalef(2,2,0);
                glBegin(GL_QUADS);
                glTexCoord2f(0.0, 0.0);
                glVertex3f(-1.0, -1.0, 0.0);
                glTexCoord2f(0.0, 1.0);
                glVertex3f(-1.0, 1.0, 0.0);
                glTexCoord2f(1.0, 1.0);
                glVertex3f(1.0, 1.0, 0.0);
                glTexCoord2f(1.0, 0.0);
                glVertex3f(1.0, -1.0, 0.0);
                glEnd();
                glPopMatrix();
            }
        }
    }
    */
}

void gm_stats(game gm, double * time, int * people){
	int i, add = 0;
	for(i = 0; i < gm->person_num; i++){
		if(gm->person[i].state == SAFE){
			add++;
		}
	}
    *people = add;
    *time = gm->timer;

}

char * gm_portal(game gm){
    if(gm->onpt >= 0 && gm->c == 1){
      return gm->portal[gm->onpt].lvl_path;
    }else{
        return NULL;
    }
}

char * gm_portal_check(game gm){
    if(gm->onpt >= 0){
      return gm->portal[gm->onpt].name;
    }else{
        return NULL;
    }
}

void gm_message_render(game gm, int width, int height){
	int i, add = 0;
    int check = 0;
	for(i = 0; i < gm->person_num; i++){
		if(gm->person[i].state == SAFE){
			add++;
            check++;
		}
        if(gm->person[i].state == PERSON){
            check++;
        }
	}

	/*Messages*/
	char buf[100];

	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float ratio = (double)width/(double)height;
    height = 600;
    width = height*ratio;
   
    gluOrtho2D(0, width, 0, height);
    glMatrixMode(GL_MODELVIEW);

	/*
    float c[4] = {1,.5,.4,.4};
    rat_set_text_color(gm->font, c);
	sprintf(buf, "Saved %d of %d",add, gm->save_count);	
    float len = rat_font_text_length(gm->font, buf);
    rat_font_render_text(gm->font,20,height-4, buf);
	*/

    float len;
	sprintf(buf, "%.1lf", gm->timer);	
    len = rat_font_text_length(gm->font, buf);
    rat_font_render_text(gm->font,width/2 - 50,height-4, buf);
    

	sprintf(buf, "%.1lf", gm->hero.nrg);	
    len = rat_font_text_length(gm->font, buf);
    rat_font_render_text(gm->font,width/2 + 50,height-4, buf);

	
	/*
    sprintf(buf, "Chain Num: %d", gm->chain.num);	
    len = rat_font_text_length(gm->font, buf);
    rat_font_render_text(gm->font, width - 250,height-4, buf);
	*/

    float co[4] = {0,0,0,0};
    rat_set_text_color(gm->font, co);

	if(add >= gm->save_count && gm->hero.state == SAFE){
        sprintf(buf, "Press space to continue.");	
        len = rat_font_text_length(gm->font, buf);
        rat_font_render_text(gm->font,(width-len)/2,height/2, buf);
    }
    else if(gm->hero.state == DONE){
        gm->gm_state = 1;
        sprintf(buf, "Level Complete");	
        len = rat_font_text_length(gm->font, buf);
        rat_font_render_text(gm->font,(width-len)/2,height/2, buf);
    }
    else if(gm->hero.state == P_Z || gm->hero.state == ZOMBIE){
        gm->gm_state = 1;
        sprintf(buf, "Infected!");	
        len = rat_font_text_length(gm->font, buf);
        rat_font_render_text(gm->font,(width-len)/2,height/2, buf);
    }
    else if(check < gm->save_count){
        gm->gm_state = 1;
        sprintf(buf, "Too many Zombies!");	
        len = rat_font_text_length(gm->font, buf);
        rat_font_render_text(gm->font,(width-len)/2,height/2, buf);
	}
    glBindTexture(GL_TEXTURE_2D, gm->blank_tex);
    for (i = 0; i < gm->save_count; i++)
    {
        glPushMatrix();
        glTranslatef(i*31 + 20, height - 20, 0);
        glScalef(15, 15,0);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-1.0, -1.0, 0.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-1.0, 1.0, 0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(1.0, 1.0, 0.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(1.0, -1.0, 0.0);
        glEnd();
        glPopMatrix();
    }
    glBindTexture(GL_TEXTURE_2D, gm->extra_tex);
    for (i = 0; i < add; i++)
    {
        glPushMatrix();
        glTranslatef(i*31 + 20, height - 20, 0);
        glScalef(12, 12,0);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-1.0, -1.0, 0.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-1.0, 1.0, 0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(1.0, 1.0, 0.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(1.0, -1.0, 0.0);
        glEnd();
        glPopMatrix();
    }

    glPopMatrix();
}

int gm_progress(game gm){
	int i, add = 0;
    int check = 0;
    int person = 0;
	for(i = 0; i < gm->person_num; i++){
		if(gm->person[i].state == SAFE){
			add++;
            check++;
		}
        if(gm->person[i].state == PERSON){
            check++;
            person++;
        }
	}
	if(gm->hero.state == ZOMBIE || check < gm->save_count){
        gm->gm_state = 1;
		return -1;
	}
	if(gm->hero.state == P_Z || check < gm->save_count){
        gm->gm_state = 1;
    }
	if((add >= gm->save_count && gm->hero.state == SAFE && gm->c == 1) ||
        (person == 0 && add >= gm->save_count && gm->hero.state == SAFE)){
		gm->hero.state = DONE;
        b2Filter flt;
        flt.categoryBits = k_person_cat;
        flt.maskBits = k_person_mask;
        gm->hero.bod->GetFixtureList()->SetFilterData(flt);
		return add - gm->save_count + 1;
	}
	
	return 0;
}

void gm_free(game gm){
    int i;
    for(i=0; i<gm->person_num; i++){
        if(gm->person[i].path != NULL){
            free(gm->person[i].path);
            gm->person[i].path = NULL;
        }
    }     
	free(gm);
}

void gm_mouse(game gm, int x, int y){
	gm->mpx = x;
	gm->mpy = y;
}

void gm_mouse_down(game gm, int x, int y){
	gm->mpx = x;
	gm->mpy = y;

    chain_ready_zero(gm);
    gm->c = 1;
    gm->hero.spring_state = NOT_ATTACHED;
}

void gm_mouse_up(game gm, int x, int y){
	gm->mpx = x;
	gm->mpy = y;
    gm->c = 0;
}

b2Vec2 gm_get_hero(game gm){
    return gm->hero.bod->GetPosition();
}

void gm_set_hero(game gm, b2Vec2 hero){
    gm->hero.bod->SetTransform(hero, 0);
}

void gm_update_mouse(game gm){
	double vdiffy = gm->vmax.y - gm->vmin.y;
	double vdiffx = gm->vmax.x - gm->vmin.x;
    
    gm->ms.x = vdiffx / (double)gm->screenx * gm->mpx + gm->vmin.x;
    gm->ms.y = vdiffy / (double)gm->screeny * gm->mpy + gm->vmin.y;
}

void gm_nkey_down(game gm, unsigned char key){
	switch(key) {
		case 'z':
			gm->zoom = 1;
			break;
		
		case 'x':
			gm->zoom = -1;
			break;
			
		case ' ':
			chain_ready_zero(gm);
            gm->c = 1;
			gm->hero.spring_state = NOT_ATTACHED;
			break;

        case 'c':
            gm->c = 1;
			break;
			
        case 'm':
		    gm->m = gm->m ? 0 : 1;
			break;

        case 'n':
		    gm->n = 1;
			break;
	}
}

void gm_nkey_up(game gm, unsigned char key){
	switch(key) {
        case 'n':
		    gm->n = 0;
			break;
		case 'z':
			gm->zoom = 0;
			break;
		
		case 'x':
			gm->zoom = 0;
			break;
		case 'c':
			gm->c = 0;
			break;

		case ' ':
			gm->c = 0;
			break;
	}
	
}


void gm_skey_down(game gm, int key){
	switch(key) {
		case SDLK_LEFT : 
			gm->ak.x = -1;
			break;
		case SDLK_RIGHT : 
			gm->ak.x = 1;
			break;
		case SDLK_UP : 
			gm->ak.y = 1;
			break;
		case SDLK_DOWN : 
			gm->ak.y = -1;
			break;
	}
}

void gm_skey_up(game gm, int key){
	switch (key) {
		case SDLK_LEFT : 
			if(gm->ak.x < 0) {gm->ak.x = 0;}
			break;
		case SDLK_RIGHT: 
			if(gm->ak.x > 0) {gm->ak.x = 0;}
			break;
		case SDLK_UP : 
			if(gm->ak.y > 0) {gm->ak.y = 0;}
			break;
		case SDLK_DOWN : 
			if(gm->ak.y < 0) {gm->ak.y = 0;}
			break;
	}
}

vector2 gm_dim(game gm){
	vector2 dim = gm->hero.o.p;
	return dim;
}

void gm_free_level(game gm){
    int i;
    for(i=0; i<gm->person_num; i++){
        if(gm->person[i].path != NULL){
            free(gm->person[i].path);
        }
    }     
	glDeleteTextures(1, &gm->bk);
}

int gm_load_bk(game gm, char * file){
        if(!load_texture(file, &gm->bk)){
            strcpy(gm->res_buf, gm->res_path);
            strcat(gm->res_buf, "/imgs/bk.png");
            load_texture(gm->res_buf, &gm->bk);
        }
}



void gm_check_portals(game gm, int save_count){
    int i;
    for(i = 0; i<gm->portal_num; i++){
        if(gm->portal[i].save_count > save_count){
            gm->portal[i].open = 0;
            b2Filter flt;
            flt.categoryBits = k_open_portal_sensor_cat;
            flt.maskBits = k_open_portal_sensor_mask;
            flt.groupIndex = 0;
            gm->portal[i].bod->GetFixtureList()->GetNext()->SetFilterData(flt);
            flt.categoryBits = k_open_portal_cat;
            flt.maskBits = k_open_portal_mask;
            gm->portal[i].bod->GetFixtureList()->SetFilterData(flt);
        }else{
            gm->portal[i].open = 1;
        }
        //printf("Portid %d; Open %d;\n", i, gm->portal[i].open);
    }
}

void chain_update(game gm){
    int i;
    for(i = 0; i < gm->chain.qnum; i++){
        int num = gm->chain.make_q[i];

        b2DistanceJointDef jd;

        jd.frequencyHz = 10.0f;
        jd.dampingRatio = 0.5f;


        if(gm->chain.num > 1){
            b2Body * plink = gm->chain.links[gm->chain.num-2];
            b2Joint * djoint = NULL;
            b2JointEdge * list;
            for(list = plink->GetJointList(); list != NULL; list = list->next){
                if(list->other == gm->hero.bod){
                    djoint = list->joint;
                }
            }
            gm->m_world->DestroyJoint(djoint);

            jd.bodyA = plink;
            jd.bodyB = gm->person[num].bod;
            jd.localAnchorA.Set(-3.0f, 0.0f);
            jd.localAnchorB.Set(0.0f, 0.0f);
            jd.bodyA->GetWorldPoint(jd.localAnchorA);
            jd.bodyB->GetWorldPoint(jd.localAnchorB);
            jd.length = 0;
            gm->m_world->CreateJoint(&jd);

        }

        b2Vec2 p1, p2;
        p1 = gm->hero.bod->GetPosition();
        p2 = gm->person[num].bod->GetPosition();

        b2PolygonShape shape;
        shape.SetAsBox(3.0f, 0.2f);

        b2FixtureDef fd;
        fd.shape = &shape;
        fd.density = 0.1f;
        fd.friction = 0.2f;
        fd.filter.categoryBits = k_hero_cat;
        fd.filter.maskBits = k_hero_mask;

        b2BodyDef bd;
        bd.type = b2_dynamicBody;
        bd.position.Set((p2.x - p1.x)/2.0f+p1.x, (p2.y - p1.y)/2.0f+p1.y);
        b2Body * link = gm->m_world->CreateBody(&bd);
        gm->chain.links[gm->chain.num-1] = link;
        link->CreateFixture(&fd);


        jd.bodyA = gm->hero.bod;
        jd.bodyB = link;
        jd.localAnchorA.Set(0.0f, 0.0f);
        jd.localAnchorB.Set(-3.2f, 0.0f);
        p1 = jd.bodyA->GetWorldPoint(jd.localAnchorA);
        p2 = jd.bodyB->GetWorldPoint(jd.localAnchorB);
        jd.length = 0.0f;
        gm->m_world->CreateJoint(&jd);

        jd.bodyA = link;
        jd.bodyB = gm->person[num].bod;
        jd.localAnchorA.Set(3.0f, 0.0f);
        jd.localAnchorB.Set(0.0f, 0.0f);
        p1 = jd.bodyA->GetWorldPoint(jd.localAnchorA);
        p2 = jd.bodyB->GetWorldPoint(jd.localAnchorB);
        jd.length = 0;
        gm->m_world->CreateJoint(&jd);
    }

}

void chain_remove(game gm, int index){
	int k;
	int match = -1;
	for(k = 0; k < gm->chain.num; k++){
		if(gm->chain.ppl[k] == index){
			match = k;
			break;
		}
	}
	if(match == -1){
		return;
	}

    printf("C RM Index: %d chain index: %d\n", index, match);


    gm->chain.delete_q[gm->chain.dnum] = gm->chain.links[match];
    gm->chain.links[match] = NULL;
    gm->chain.dnum++;


	for(k = match; k < gm->chain.num; k++){
		gm->chain.ppl[k] = gm->chain.ppl[k+1];
		gm->chain.links[k] = gm->chain.links[k+1];
	}

    gm->chain.num--;


    if(match > 0){
        gm->m_world->DestroyBody(gm->chain.links[match-1]);

        b2DistanceJointDef jd;

        jd.frequencyHz = 10.0f;
        jd.dampingRatio = 0.5f;

        b2Vec2 p1, p2;
        if(match == gm->chain.num){
            p1 = gm->hero.bod->GetPosition();
        }
        else{
            //Remember we've delete our the guy in our chain so the next guy up is now match
            p1 = gm->person[gm->chain.ppl[match]].bod->GetPosition();
        }

        p2 = gm->person[gm->chain.ppl[match-1]].bod->GetPosition();

        b2PolygonShape shape;
        shape.SetAsBox(3.0f, 0.2f);

        b2FixtureDef fd;
        fd.shape = &shape;
        fd.density = 0.1f;
        fd.friction = 0.2f;
        fd.filter.categoryBits = k_hero_cat;
        fd.filter.maskBits = k_hero_mask;

        b2BodyDef bd;
        bd.type = b2_dynamicBody;
        bd.position.Set((p2.x - p1.x)/2.0f+p1.x, (p2.y - p1.y)/2.0f+p1.y);
        b2Body * link = gm->m_world->CreateBody(&bd);
        gm->chain.links[match-1] = link;
        link->CreateFixture(&fd);


        if(match == gm->chain.num){
            jd.bodyA = gm->hero.bod;
        }
        else{
            jd.bodyA = gm->person[gm->chain.ppl[match]].bod;
        }
        jd.bodyB = link;
        jd.localAnchorA.Set(0.0f, 0.0f);
        jd.localAnchorB.Set(-3.2f, 0.0f);
        p1 = jd.bodyA->GetWorldPoint(jd.localAnchorA);
        p2 = jd.bodyB->GetWorldPoint(jd.localAnchorB);
        jd.length = 0.0f;
        gm->m_world->CreateJoint(&jd);

        jd.bodyA = link;
        jd.bodyB = gm->person[gm->chain.ppl[match-1]].bod;
        jd.localAnchorA.Set(3.0f, 0.0f);
        jd.localAnchorB.Set(0.0f, 0.0f);
        p1 = jd.bodyA->GetWorldPoint(jd.localAnchorA);
        p2 = jd.bodyB->GetWorldPoint(jd.localAnchorB);
        jd.length = 0;
        gm->m_world->CreateJoint(&jd); 
    }


}

void chain_cut(game gm, int index){
	int k;
	int match = -1;
	for(k = 0; k < gm->chain.num; k++){
		if(gm->chain.ppl[k] == index){
			match = k;
			break;
		}
	}
	if(match == -1){return;}
    for(k = match; k >= 0; k--){
        int temp = gm->chain.ppl[k];
        gm->person[temp].ready = 0;
        gm->chain.delete_q[gm->chain.dnum] = gm->chain.links[k];
        gm->chain.links[k] = NULL;
        gm->chain.dnum++;
    }
    gm->chain.num = gm->chain.num - match - 1;
    for(k = 0; k < gm->chain.num; k++){
        gm->chain.ppl[k] = gm->chain.ppl[k + match + 1];
        gm->chain.jt[k] = gm->chain.jt[k + match + 1];
        gm->chain.links[k] = gm->chain.links[k + match + 1];
    }
    if(gm->chain.num == 0){
        gm->hero.spring_state = NOT_ATTACHED;
    }
}

void chain_ready_zero(game gm){
	int k;
	for(k = 0; k < gm->chain.num; k++){
		int h = gm->chain.ppl[k];
        if(gm->chain.links[k] != NULL){
            gm->m_world->DestroyBody(gm->chain.links[k]);
        }
		gm->person[h].ready = 0;
	}
    gm->chain.num = 0;
}


void stink_add(game gm, int id){
    int k;
    int num = gm->stnk_num;
    gm->stnk[num].id = id;    

    for(k = 0; k < STINK_PARTICLE_NUM; k++){
        gm->stnk[num].time[k] = (rand()%100 + 50)/100.0f;
        gm->stnk[num].n[k].p.x = gm->person[id].o.p.x + 2*cos((M_PI*2)/360.0f*(float)(rand()%360));
        gm->stnk[num].n[k].p.y = gm->person[id].o.p.y + 2*sin((M_PI*2)/360.0f*(float)(rand()%360));
        gm->stnk[num].n[k].v.x += rand()%4 - 2;
        gm->stnk[num].n[k].v.y += rand()%4 - 2;
    }
    gm->stnk_num++;
}

void stink_step(game gm, double dt){
    int k;
    int num;

    for(num = 0; num < gm->stnk_num; num++){
        int id = gm->stnk[num].id;
        for(k = 0; k < STINK_PARTICLE_NUM; k++){
            if(gm->stnk[num].time[k] <=0){
                b2Vec2 p = gm->person[id].bod->GetPosition();
                b2Vec2 v = gm->person[id].bod->GetLinearVelocity();
                gm->stnk[num].time[k] = (rand()%100 + 50)/100.0f;
                gm->stnk[num].n[k].p.x = p.x + 2*cos((M_PI*2)/360.0f*(float)(rand()%360));
                gm->stnk[num].n[k].p.y = p.y + 2*sin((M_PI*2)/360.0f*(float)(rand()%360));
                gm->stnk[num].n[k].v.x = v.x/2.0f + rand()%6 - 3;
                gm->stnk[num].n[k].v.y = v.y/2.0f +rand()%6 - 3;
                gm->stnk[num].n[k].th = rand()%360;
            }
            else{
                gm->stnk[num].time[k] -= dt;
                gm->stnk[num].n[k].p.x += gm->stnk[num].n[k].v.x*dt;
                gm->stnk[num].n[k].p.y += gm->stnk[num].n[k].v.y*dt;
            }
        }
    }
}

void stink_render(game gm){
    int k;
    int num;
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    for(num = 0; num < gm->stnk_num; num++){
        for(k = 0; k < STINK_PARTICLE_NUM; k++){
            glColor4f(1,1,1, gm->stnk[num].time[k]*4);
            glBindTexture( GL_TEXTURE_2D, gm->stink_tex);
            glPushMatrix();
            glTranslatef(gm->stnk[num].n[k].p.x, gm->stnk[num].n[k].p.y, 0);
            glRotatef(gm->stnk[num].n[k].th, 0, 0, 1);
            glScalef(8,8,0);
            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 0.0);
            glVertex3f(-1.0, -1.0, 0.0);
            glTexCoord2f(0.0, 1.0);
            glVertex3f(-1.0, 1.0, 0.0);
            glTexCoord2f(1.0, 1.0);
            glVertex3f(1.0, 1.0, 0.0);
            glTexCoord2f(1.0, 0.0);
            glVertex3f(1.0, -1.0, 0.0);
            glEnd();
            glPopMatrix();
        }
    }
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}


void ai_functions(game gm){
    int k;
    for(k=0; k < gm->person_num; k++){
        b2Vec2 p = gm->person[k].bod->GetPosition();
        b2Vec2 v = gm->person[k].bod->GetLinearVelocity();
        gm->person[k].o.f.x = 0;
        gm->person[k].o.f.y = 0;
        gm->person[k].o.p.x = p.x;
        gm->person[k].o.p.y = p.y;
        gm->person[k].o.v.x = v.x;
        gm->person[k].o.v.y = v.y;

        p = gm->hero.bod->GetPosition();
        v = gm->hero.bod->GetLinearVelocity();
        gm->hero.o.p.x = p.x;
        gm->hero.o.p.y = p.y;
        gm->hero.o.v.x = v.x;
        gm->hero.o.v.y = v.y;

        if(gm->person[k].state == ZOMBIE && gm->person[k].chase == 0 && gm->person[k].o.r > 2){
            if(v2Len(v2Sub(gm->person[k].o.p, gm->hero.o.p)) <= 20){
                gm->person[k].chase = 1;
            }
        }
        else if(gm->person[k].state == ZOMBIE && gm->person[k].chase == 1 && gm->person[k].o.r > 2 ){
            if(v2Len(v2Sub(gm->person[k].o.p, gm->hero.o.p)) >= 30){
                gm->person[k].chase = 0;
            }
        }
        if(gm->person[k].chase == 1 && gm->hero.state ==  PERSON){
            ai_chase(&gm->hero.o, &gm->person[k].o, 70.0f, 30.0f);
            //ai_seek(gm->hero.o.p, &gm->person[k].o, 2.0f);
			//ai_avoid(gm->hero.o.p, &gm->person[k].o, 200.0f);
        }
		else if(gm->person[k].state == ZOMBIE && gm->hero.state ==  ZOMBIE){
            //ai_chase(&gm->hero.o, &gm->person[k].o, 70.0f, 30.0f);
            ai_seek(gm->hero.o.p, &gm->person[k].o, 2.0f, 20);
            ai_avoid(gm->hero.o.p, &gm->person[k].o, 200.0f);
        }
		
		if(gm->person[k].path != NULL){
            vector2 target;
            b2Vec2 b2target;
            b2Vec2 pos;

            pos.x = gm->person[k].o.p.x;
            pos.y = gm->person[k].o.p.y;

            gm->person[k].path->updatePath(pos);
            b2target = gm->person[k].path->getTarget();
            target.x = b2target.x;
            target.y = b2target.y;

            ai_seek(target, &gm->person[k].o, 0.7f, 20.0f);
        }

		if(gm->person[k].state == ZOMBIE && gm->person[k].parent_id >= 0){
            //ai_chase(&gm->hero.o, &gm->person[k].o, 70.0f, 30.0f);
            ai_seek(gm->person[gm->person[k].parent_id].o.p, &gm->person[k].o, 2.1f, 20);
            ai_avoid(gm->person[gm->person[k].parent_id].o.p, &gm->person[k].o, 200.0f);
			int j;
			for (j = k + 1; j < gm->person_num; j++) {
				if(gm->person[j].state == ZOMBIE && gm->person[j].parent_id == gm->person[k].parent_id){
					ai_avoid(gm->person[j].o.p, &gm->person[k].o, 50.0f);
				}
			}
        }
        
   }

}

void gm_portal_ct(game gm, int user_id){
    printf("Checking to see which levels have been beat and by how much...\n");
    sqlite3 * sdb;
    sqlite3_stmt * sql;
    const char * extra;
    char stmt[150];
    sprintf(stmt, "SELECT level_id, MAX(people_saved) mp FROM level_stats  WHERE user_id = %d AND complete = 1 GROUP BY level_id;", user_id);
    printf("%s\n", stmt);
    int result; 
    printf("db_path %s\n", gm->db_path);
    result = sqlite3_open(gm->db_path, &sdb);
    printf("Open result: %d; ", result);
    result = sqlite3_prepare_v2(sdb, stmt, sizeof(stmt) + 1 , &sql, &extra);
    printf("prepare result: %d\n", result);

    int i = 0;

    while((result = sqlite3_step(sql)) == SQLITE_ROW){
        for(i=0; i<gm->portal_num; i++){
            if(strcmp((const char*)sqlite3_column_text(sql, 0), gm->portal[i].name) == 0){
                gm->portal[i].max_saved =  sqlite3_column_int(sql, 1);
                printf("Portal %s sc: %d\n", gm->portal[i].name, gm->portal[i].max_saved);
            }
        }
    }
    printf("Step result: %d\n", result);
    sqlite3_finalize(sql);
    sqlite3_close(sdb);   
}

int gm_load_character(game gm, int type, b2Vec2 vel, b2Vec2 pos, mxml_node_t *node, AiPath * path){
    bool fix_rotation = false;
    float damping = 1;
    int num = gm->person_num;
    const char * name;
    const char * elem_type;
    float r, h, w;
    int shape_type;

    elem_type = mxmlGetElement(node);

    b2BodyDef bd;
    b2Body * bod;
    b2FixtureDef fd;

    if(strcmp(elem_type, "circle") == 0){
        shape_type = t_circle;

        bd.type = b2_dynamicBody;
        bd.position = pos;

        bod = gm->m_world->CreateBody(&bd);
        bod->SetLinearVelocity(vel);

        name = mxmlElementGetAttr(node, "r"); 
        sscanf(name, "%f", &r);

        b2CircleShape circle;
        circle.m_radius = r;

        fd.shape = &circle;
        fd.density = 0.04f;
        fd.restitution = 0.5f;
        fd.friction = 0.2f;
    }
    else if(strcmp(elem_type, "rect") == 0){
        float x, y;
        shape_type = t_rect;

        name = mxmlElementGetAttr(node, "x"); 
        sscanf(name, "%f", &x);
        name = mxmlElementGetAttr(node, "y"); 
        sscanf(name, "%f", &y);
        name = mxmlElementGetAttr(node, "height"); 
        sscanf(name, "%f", &h);
        name = mxmlElementGetAttr(node, "width"); 
        sscanf(name, "%f", &w);

        h *= 0.5;
        w *= 0.5;

        pos.x = x;
        pos.y = gm->h - y;

        bd.type = b2_dynamicBody;
        bd.position = pos;

        bod = gm->m_world->CreateBody(&bd);
        bod->SetLinearVelocity(vel);

        b2PolygonShape polygon;
        polygon.SetAsBox(w,h);

        fd.shape = &polygon;
        fd.density = 0.04f;
        fd.restitution = 0.5f;
        fd.friction = 0.01f;
    }


    switch(type){
        case t_hero:
            fd.filter.categoryBits = k_hero_cat;
            fd.filter.maskBits = k_hero_mask;
            fd.filter.categoryBits = k_hero_cat;
            fd.filter.maskBits = k_hero_mask;
            fd.userData = &gm->hero;

            gm->hero.bod = bod;
            gm->hero.grav.x = 0;
            gm->hero.grav.y = 0;
            gm->hero.state = PERSON;
            gm->hero.whoami = t_hero;
            gm->hero.nrg = 100;
            gm->hero.spring_state = NOT_ATTACHED;
            gm->hero.o.p.x = pos.x;
            gm->hero.o.p.y = pos.y;
            gm->hero.o.v.x = 0;
            gm->hero.o.v.y = 0;
            gm->hero.o.r = r;
            gm->hero.o.m = r*r*M_PI/25.2;
            gm->hero.o.snd = 0;
            break;

        case t_spike:
        case t_person:
        case t_zombie:
            fd.density = 0.05f;
            fd.filter.categoryBits = k_person_cat;
            fd.filter.maskBits = k_person_mask;
            fd.userData = &gm->person[num];

            gm->person[num].grav.x = 0;
            gm->person[num].grav.y = 0;
            gm->person[num].bod = bod;
            gm->person[num].state = type == t_zombie ? ZOMBIE : PERSON;
            gm->person[num].emo = NORMAL;
            gm->person[num].whoami = t_person;
            gm->person[num].ready = 0;
            gm->person[num].shape = shape_type;
            gm->person[num].h = h;
            gm->person[num].w = w;
            gm->person[num].o.p.x = pos.x;
            gm->person[num].o.p.y = pos.y;
            gm->person[num].o.v.x = vel.x;
            gm->person[num].o.v.y = vel.y;
            gm->person[num].o.r = r;
            gm->person[num].o.m = r*r*M_PI/25.2;
            if(type == t_zombie){
                stink_add(gm, gm->person_num);
            }
            gm->person[num].mx_f = v2Len(gm->person[num].o.v);
            gm->person[num].chase = 0;
            gm->person[num].parent_id = -1;
            gm->person[num].o.snd = 0;
            gm->person[num].path = path;
            gm->person_num++;
            break;

    }

    bod->CreateFixture(&fd);
    bod->SetFixedRotation(fix_rotation);
    bod->SetLinearDamping(damping);
    bod->SetAngularDamping(damping);
}

int gm_load_gavity_area(game gm, b2Vec2 grav, mxml_node_t *node){
    const char * name;

    float x, y, h, w;
    name = mxmlElementGetAttr(node, "x"); 
    sscanf(name, "%f", &x);
    name = mxmlElementGetAttr(node, "y"); 
    sscanf(name, "%f", &y);
    name = mxmlElementGetAttr(node, "height"); 
    sscanf(name, "%f", &h);
    name = mxmlElementGetAttr(node, "width"); 
    sscanf(name, "%f", &w);

    h *= 0.5;
    w *= 0.5;
    y = gm->h - y; 
    x += w;
    y -= h;


    b2Vec2 pos(x,y);

    b2BodyDef bd;
    bd.position = pos;

    b2Body * bod = gm->m_world->CreateBody(&bd);

    b2PolygonShape polygon;
    polygon.SetAsBox(w,h);

    b2FixtureDef fd;
    fd.shape = &polygon;
    fd.userData = &gm->gravity_area[gm->gravity_area_num];
    fd.density = 0.04f;
    fd.restitution = 0.5f;
    fd.friction = 0.2f;
    fd.isSensor = true;
    fd.filter.categoryBits = k_safe_zone_sensor_cat;
    fd.filter.maskBits = k_safe_zone_sensor_mask;
    bod->CreateFixture(&fd);


    gm->gravity_area[gm->gravity_area_num].bod = bod;
    gm->gravity_area[gm->gravity_area_num].dim.x = w;
    gm->gravity_area[gm->gravity_area_num].dim.y = h;
    gm->gravity_area[gm->gravity_area_num].whoami = t_gravity_area;
    gm->gravity_area[gm->gravity_area_num].grav = grav;
    gm->gravity_area_num++;
}

int gm_load_level_svg(game gm, char * file_path){
    bool fix_rotation = false;
    float damping = 1;

    //Going to load stuff in b2 world
    delete gm->m_world;

    b2Vec2 gravity(0.0f, 0.0f);
    gm->m_world = new b2World(gravity);
    gm->m_world->SetContactListener(&CL); 


	int i;
	FILE *fp;
    mxml_node_t *tree;


    fp = fopen(file_path, "r");
	
	if(fp == NULL)
	{
		printf("File %s unable to load.\n", file_path); 
		return 0;
	}
	
    tree = mxmlLoadFile(NULL, fp,
                        MXML_TEXT_CALLBACK);
    fclose(fp);

    const char *name;

    mxml_node_t *node = tree;
	
	name = mxmlGetElement(node);
    if(strcmp(name, "svg") != 0){
        node = mxmlFindElement(tree, tree, "svg", NULL, NULL, MXML_DESCEND);
    }

	gm->ak.x =0;
	gm->ak.y = 0;
    gm->c = 0;

    gm->chain.num = 0;
    
    gm->safe_zone.o.m = 1000;
    gm->safe_zone.o.v.x = 0;
    gm->safe_zone.o.v.y = 0;
    gm->person_num = 0;
    gm->gravity_area_num = 0;
    gm->wall_num = 0;
    gm->portal_num = 0;
    gm->save_count = 1;
    gm->timer = 0;
    gm->onpt = -1;
    i = 1;
    gm->stnk_num = 0;

    name = mxmlElementGetAttr(node, "height"); 
    sscanf(name, "%lf", &gm->h);
    name = mxmlElementGetAttr(node, "width"); 
    sscanf(name, "%lf", &gm->w);

    b2Body* ground = NULL;
    {
        b2BodyDef bd;
        ground = gm->m_world->CreateBody(&bd);
        b2EdgeShape shape;
        b2FixtureDef fd;
        fd.shape = &shape;
        fd.density = 0.0f;
        fd.friction = 0.5f;
        fd.restitution = 0.2f;
        shape.Set(b2Vec2(0.0f, 0.0f), b2Vec2(0.0f, gm->h));
        ground->CreateFixture(&fd);
        shape.Set(b2Vec2(0.0f, gm->h), b2Vec2(gm->w, gm->h));
        ground->CreateFixture(&fd);
        shape.Set(b2Vec2(gm->w, gm->h), b2Vec2(gm->w, 0.0f));
        ground->CreateFixture(&fd);
        shape.Set(b2Vec2(gm->w, 0.0f), b2Vec2(0.0f, 0.0f));
        ground->CreateFixture(&fd);
    }


    gm->b_walls = NULL;
    b2BodyDef bd;
    gm->b_walls = gm->m_world->CreateBody(&bd);

    gm->save_count = 2;

	node = mxmlFindElement(tree, tree, "g", NULL, NULL, MXML_DESCEND);
    node = mxmlWalkNext(node, tree, MXML_DESCEND);

    for(node = node; node != NULL; node = mxmlGetNextSibling(node)){
        name = mxmlGetElement(node);
        while(name == NULL && node != NULL){
            node = mxmlGetNextSibling(node);
            name = mxmlGetElement(node);
        }
        if(name == NULL)
        {break;}

        if(strcmp(name, "rect") == 0){
            const char *color=mxmlElementGetAttr(node, "fill");
            float x, y, h, w;
            name = mxmlElementGetAttr(node, "x"); 
            sscanf(name, "%f", &x);
            name = mxmlElementGetAttr(node, "y"); 
            sscanf(name, "%f", &y);
            name = mxmlElementGetAttr(node, "height"); 
            sscanf(name, "%f", &h);
            name = mxmlElementGetAttr(node, "width"); 
            sscanf(name, "%f", &w);

            h *= 0.5;
            w *= 0.5;
            y = gm->h - y; 
            x += w;
            y -= h;
            if(strcmp(color, "#00bf5f") == 0){
                b2Vec2 pos(x,y);
                b2Vec2 vel(0,0);

                gm_load_character(gm, t_zombie, vel, pos, node, NULL);
            }
            //Gravify area
            else if(strcmp(color, "#0000ff") == 0){
                b2Vec2 grav(0,-100);
                gm_load_gavity_area(gm, grav, node);
            }

        }
        else if(strcmp(name, "circle") == 0){
            float cx;
            float cy;
            float r;

            name = mxmlElementGetAttr(node, "cx"); 
            sscanf(name, "%f", &cx);
            name = mxmlElementGetAttr(node, "cy"); 
            sscanf(name, "%f", &cy);
            name = mxmlElementGetAttr(node, "r"); 
            sscanf(name, "%f", &r);
            cy = gm->h - cy;

            const char *color=mxmlElementGetAttr(node, "fill");
            if(strcmp(color, "#e5e5e5") == 0){
                gm->safe_zone.o.p.x = cx;
                gm->safe_zone.o.p.y = cy;
                gm->safe_zone.o.r = r;
                gm->safe_zone.tr = r*cos(M_PI/6.0f);
                gm->safe_zone.whoami = t_safezone;
                gm->safe_zone.sensor_touch = 0;

                b2Vec2 pos(cx,cy);
                b2BodyDef bd;
                bd.position = pos;
                gm->safe_zone.bod = gm->m_world->CreateBody(&bd);
                b2CircleShape circle;
                circle.m_radius = r;
                b2FixtureDef fd;
                fd.shape = &circle;
                fd.restitution = 0.5f;
                fd.friction = 0.2f;
                fd.filter.categoryBits = k_safe_zone_sensor_cat;
                fd.filter.maskBits = k_safe_zone_sensor_mask;
                fd.isSensor = true;
                fd.userData = &gm->safe_zone;
                gm->safe_zone.bod->CreateFixture(&fd);


                gm->safe_zone.cage = gm->m_world->CreateBody(&bd);
                b2EdgeShape shape;
                fd.userData = NULL;
                fd.shape = &shape;
                fd.isSensor = false;
                fd.density = 0.0f;
                fd.friction = 0.5f;
                fd.restitution = 0.2f;
                fd.filter.categoryBits = k_safe_zone_cage_cat;
                fd.filter.maskBits = k_safe_zone_cage_mask;
                int i;
                for(i = 0; i<6; i++){
                    shape.Set(b2Vec2(r*cos(i/6.0f*M_PI*2), r*sin(i/6.0f*M_PI*2)), b2Vec2( r*cos((i+1)/6.0f*M_PI*2) ,  r*sin((i+1)/6.0f*M_PI*2) ));
                    gm->safe_zone.cage->CreateFixture(&fd);
                    printf("cage: %d %.1f %.1f\n", i,cx + r*cos(i/6.0f*M_PI*2),cy+r*sin(i/6.0f*M_PI*2) );
                }
            }
            else if(strcmp(color, "#ff0000") == 0 || strcmp(color, "#FF0000") == 0){
                b2Vec2 pos(cx,cy);
                b2Vec2 vel(0,0);

                gm_load_character(gm, t_hero, vel, pos, node, NULL);
            }
            else if(strcmp(color, "#00bf5f") == 0){
                b2Vec2 pos(cx,cy);
                b2Vec2 vel(0,0);

                gm_load_character(gm, t_zombie, vel, pos, node, NULL);
            }
            else if(strcmp(color, "#ffff00") == 0){
				int num = gm->person_num;
                b2Vec2 pos(cx,cy);
                b2Vec2 vel(0,0);

                gm_load_character(gm, t_person, vel, pos, node, NULL);
            }
            else if(strcmp(color, "#999999") == 0){
                b2Vec2 pos(cx,cy);
                b2Vec2 vel(0,0);
                gm_load_character(gm, t_spike, vel, pos, node, NULL);
            }
        }
        else if(strcmp(name, "line") == 0){
            b2EdgeShape shape;
            b2FixtureDef fd;
            fd.shape = &shape;
            fd.density = 0.0f;
            fd.friction = 1.0f;
            fd.filter.categoryBits = k_walls_cat;
            fd.filter.maskBits = k_walls_mask;
            float x1, x2, y1, y2;
            name = mxmlElementGetAttr(node, "x1"); 
            sscanf(name, "%f", &x1);
            name = mxmlElementGetAttr(node, "y1"); 
            sscanf(name, "%f", &y1);
            y1 = gm->h - y1;
            name = mxmlElementGetAttr(node, "x2"); 
            sscanf(name, "%f", &x2);
            name = mxmlElementGetAttr(node, "y2"); 
            sscanf(name, "%f", &y2);
            y2 = gm->h - y2;
            
            int n = gm->wall_num;
            gm->walls[n].p1.x = x1;
            gm->walls[n].p1.y = y1;
            gm->walls[n].p2.x = x2;
            gm->walls[n].p2.y = y2;
            gm->wall_num++;

            shape.Set(b2Vec2(x1,y1), b2Vec2(x2, y2));
            gm->b_walls->CreateFixture(&fd);
        }
		if(strcmp(name, "text") == 0){
            name = mxmlGetText(node, NULL); 
			sscanf(name, "%d", &gm->save_count);
        }


        //This is for portals, portals are stored in anchor elements
		if(strcmp(name, "a") == 0){
            mxml_node_t * child;
            int pn = gm->portal_num;
            name = mxmlElementGetAttr(node, "xlink:href"); 
            //printf("xlink:href portal link: %s\n", name);
            gm->portal[pn].lvl_path = (char*)malloc(strlen(name)*sizeof(char)+sizeof(char)*2);
            gm->portal[pn].name = (char*)malloc(strlen(name)*sizeof(char)+sizeof(char)*2);
            strcpy(gm->portal[pn].lvl_path, name);

            char *ans;
            ans = strchr(name,'.');
            *ans = '\0';
            strcpy(gm->portal[pn].name, name);

            float cx;
            float cy;
            float r;

            for(child = mxmlWalkNext(node, tree, MXML_DESCEND); child != NULL; child = mxmlGetNextSibling(child)){
                name = mxmlGetElement(child);
                while(name == NULL && child != NULL){
                    child = mxmlGetNextSibling(child);
                    name = mxmlGetElement(child);
                }
                if(name == NULL)
                {break;}

                if(strcmp(name, "circle") == 0){
                    name = mxmlElementGetAttr(child, "cx"); 
                    sscanf(name, "%f", &cx);
                    name = mxmlElementGetAttr(child, "cy"); 
                    sscanf(name, "%f", &cy);
                    name = mxmlElementGetAttr(child, "r"); 
                    sscanf(name, "%f", &r);
                    cy = gm->h - cy; 
                }
                if(strcmp(name, "text") == 0){
                    name = mxmlGetText(child, NULL); 
                    sscanf(name, "%d", &gm->portal[pn].save_count);
                    printf("Portal Save Count name: %d\n", gm->portal[pn].save_count);
                    //sscanf(name, "%d", &gm->save_count);
                    //printf("Text portal name: %s\n", name);
                    //strcpy(gm->portal[pn].name, name);
                }
            }

            b2Vec2 pos(cx,cy);
            b2BodyDef bd;
            bd.position = pos;
            gm->portal[pn].bod = gm->m_world->CreateBody(&bd);
            b2CircleShape circle;
            circle.m_radius = r;
            b2FixtureDef fd;
            fd.shape = &circle;
            fd.restitution = 0.5f;
            fd.friction = 0.2f;
            fd.filter.categoryBits = k_open_portal_cat;
            fd.filter.maskBits = 0;
            gm->portal[pn].bod->CreateFixture(&fd);

            circle.m_radius = r*0.8f;
            fd.shape = &circle;
            fd.filter.categoryBits = k_open_portal_sensor_cat;
            fd.filter.maskBits = k_open_portal_sensor_mask;
            fd.isSensor = true;
            fd.userData = &gm->portal[pn];
            gm->portal[pn].bod->CreateFixture(&fd);

            gm->portal[pn].max_saved = 0;
            gm->portal[pn].o.p.x = cx;
            gm->portal[pn].o.p.y = cy;
            gm->portal[pn].o.r = r;
            gm->portal[pn].open = 1;
            gm->portal[pn].o.m = 1000;
            gm->portal[pn].o.v.x = 0;
            gm->portal[pn].o.v.y = 0;
            gm->portal[pn].whoami = t_portal;
            gm->portal_num++;
        }


        //This is for groups, groups store velocity information
        else if(strcmp(name, "g") == 0){
            mxml_node_t * child;
            mxml_node_t * char_node;
            AiPath * path = NULL;

            float cx;
            float cy;
            float r;
            float vx, vy;

            float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

            const char *color;

            int shape_type;
            for(child = mxmlWalkNext(node, tree, MXML_DESCEND); child != NULL; child = mxmlGetNextSibling(child)){
                name = mxmlGetElement(child);
                while(name == NULL && child != NULL){
                    child = mxmlGetNextSibling(child);
                    name = mxmlGetElement(child);
                }
                if(name == NULL)
                {break;}

                if(strcmp(name, "circle") == 0){
                    shape_type = t_circle;
                    char_node = child;
                    name = mxmlElementGetAttr(child, "cx"); 
                    sscanf(name, "%f", &cx);
                    name = mxmlElementGetAttr(child, "cy"); 
                    sscanf(name, "%f", &cy);
                    name = mxmlElementGetAttr(child, "r"); 
                    sscanf(name, "%f", &r);
                    cy = gm->h - cy; 
                    color = mxmlElementGetAttr(child, "fill");
                }
                else if(strcmp(name, "rect") == 0){
                    shape_type = t_rect;
                    char_node = child;
                    name = mxmlElementGetAttr(child, "x"); 
                    sscanf(name, "%f", &cx);
                    name = mxmlElementGetAttr(child, "y"); 
                    sscanf(name, "%f", &cy);
                    name = mxmlElementGetAttr(child, "width"); 
                    sscanf(name, "%f", &r);
                    cy = gm->h - cy; 
                    color = mxmlElementGetAttr(child, "fill");
                }
                else if(strcmp(name, "path") == 0){
                    const char * d;
                    path = (AiPath*)malloc(sizeof(AiPath));
                    d = mxmlElementGetAttr(child, "d"); 
                    path->parsePath(d, gm->h);
                }
                else if(strcmp(name, "line") == 0){
                    name = mxmlElementGetAttr(child, "x1"); 
                    sscanf(name, "%f", &x1);
                    name = mxmlElementGetAttr(child, "y1"); 
                    sscanf(name, "%f", &y1);
                    y1 = gm->h - y1;
                    name = mxmlElementGetAttr(child, "x2"); 
                    sscanf(name, "%f", &x2);
                    name = mxmlElementGetAttr(child, "y2"); 
                    sscanf(name, "%f", &y2);
                    y2 = gm->h - y2;
                }
            }

            vx = x2 - x1;
            vy = y2 - y1;
                
            if(sqrt((x2-cx)*(x2-cx)+(y2-cy)*(y2-cy)) < sqrt((x1-cx)*(x1-cx)+(y1-cy)*(y1-cy))){
                vx *= -1;
                vy *= -1;
            }

            b2Vec2 pos;
            pos.x = cx;
            pos.y = cy;
            b2Vec2 vel(vx,vy);
			
            
            if(strcmp(color, "#ff0000") == 0 || strcmp(color, "#FF0000") == 0){
                gm_load_character(gm, t_hero, vel, pos, char_node, path);
            }
            else if(strcmp(color, "#00bf5f") == 0){
                gm_load_character(gm, t_zombie, vel, pos, char_node, path);
            }

            else if(strcmp(color, "#ffff00") == 0){
                gm_load_character(gm, t_person, vel, pos, char_node, path);
            }
            else if(strcmp(color, "#0000ff") == 0 && shape_type == t_rect){
                b2Vec2 grav = vel;
                gm_load_gavity_area(gm, grav, char_node);
            }
        }        
      
    }
    gm->gm_state = 0;
    mxmlDelete(tree);
	printf("Wall Num: %d\n", gm->wall_num);
	return 1;
}

void cl_hero_person(game gm, actor_type * hero, actor_type * person){
    _hero * t_hero = (_hero*)hero;
    ppl   * t_ppl  = (ppl*)person;
    if(t_hero->state == PERSON && t_ppl->state == ZOMBIE){
        gm->hero.nrg -= 90;
        s_add_snd(gm->saved_src, gm->buf[al_hdeath_buf], &gm->hero.o,0.1, 1);
    }
    if(t_hero->state == ZOMBIE && t_ppl->state == PERSON){
        t_ppl->timer = MAX_TIME;
        t_ppl->state = P_Z;
        chain_cut(gm, t_ppl - gm->person);
        s_add_snd(gm->saved_src, gm->buf[al_pdeath_buf], &t_ppl->o,1, 4);
    }
    if(t_hero->state == PERSON && t_ppl->state == PERSON){
        int k = 0;
        int i = t_ppl - gm->person;
        int leave = 0;
        for(k=0; k<gm->chain.num; k++){
            if(i == gm->chain.ppl[k]){
                leave = 1;
            }
        }
        
        if(leave == 0){
            gm->hero.spring_state = ATTACHED;
            gm->chain.ppl[gm->chain.num] = i;
            gm->chain.num++;
            gm->chain.make_q[gm->chain.qnum] = i;
            gm->chain.qnum++;
            
            int rand_num = rand()%3;
            switch (rand_num) {
                case 0:
                    s_add_snd(gm->saved_src, gm->buf[al_attached1_buf], &gm->person[i].o ,1, 1);
                    break;
                case 1:
                    s_add_snd(gm->saved_src, gm->buf[al_attached2_buf], &gm->person[i].o ,1, 1);
                    break;
                case 2:
                    s_add_snd(gm->saved_src, gm->buf[al_attached3_buf], &gm->person[i].o ,1, 1);
                    break;
            }
            gm->hero.person_id = i;
            gm->person[i].ready = 1;

            b2Filter flt;
            flt.categoryBits = k_hero_cat;
            flt.maskBits = k_chain_person_mask;
            gm->person[i].bod->GetFixtureList()->SetFilterData(flt);

            gm->person[i].mx_f = 0;
        }
    }

}

void cl_person_person(game gm, actor_type * person1, actor_type * person2){
    ppl * t_ppl1  = (ppl*)person1;
    ppl * t_ppl2  = (ppl*)person2;
    if(t_ppl1->state == PERSON && t_ppl2->state == ZOMBIE){
        t_ppl1->timer = MAX_TIME;
        t_ppl1->state = P_Z;
        t_ppl1->parent_id = t_ppl2 - gm->person;
        b2Filter flt;
        flt.categoryBits = k_person_cat;
        flt.maskBits = k_person_mask;
        t_ppl1->bod->GetFixtureList()->SetFilterData(flt);
        chain_cut(gm, t_ppl1 - gm->person);
        s_add_snd(gm->saved_src, gm->buf[al_pdeath_buf], &t_ppl1->o,1, 4);
    }
}

void cl_safezone_person_begin(game gm, actor_type * person1, actor_type * person2){
    _safe_zone * safe_zone  = (_safe_zone*)person1;
    safe_zone->sensor_touch++;
    //printf("SZ-T: %d\n", safe_zone->sensor_touch);
}

void cl_safezone_person_end(game gm, actor_type * person1, actor_type * person2){
    _safe_zone * safe_zone  = (_safe_zone*)person1;
    safe_zone->sensor_touch--;
    //printf("SZ-T: %d\n", safe_zone->sensor_touch);
}

void cl_gravity_character_begin(game gm, actor_type * grav, actor_type * character){
    character->grav.x += grav->grav.x;
    character->grav.y += grav->grav.y;
}

void cl_gravity_character_end(game gm, actor_type * grav, actor_type * character){
    character->grav.x -= grav->grav.x ;
    character->grav.y -= grav->grav.y;
}

void MyContactListener::BeginContact(b2Contact* contact)
{ 
    actor_type * typeA = (actor_type*)contact->GetFixtureA()->GetUserData(); 
    actor_type * typeB = (actor_type*)contact->GetFixtureB()->GetUserData(); 

    if(typeA != NULL && typeB != NULL){
        if(typeA->whoami == t_portal && typeB->whoami == t_hero){
           _portal * tportal = (_portal*)typeA;
           if(tportal->open){
               gm->onpt = tportal - gm->portal;
           }
        }
        if(typeA->whoami == t_hero && typeB->whoami == t_portal){
           gm->onpt = -1;
           _portal * tportal = (_portal*)typeB;
           if(tportal->open){
               gm->onpt = tportal - gm->portal;
           }
        }
        if(typeA->whoami == t_person && typeB->whoami == t_person){
            cl_person_person(gm, typeB, typeA);
            cl_person_person(gm, typeA, typeB);
        }

        if(typeA->whoami == t_hero && typeB->whoami == t_person){
            cl_hero_person(gm, typeA, typeB);
        }
        if(typeA->whoami == t_person && typeB->whoami == t_hero){
            cl_hero_person(gm, typeB, typeA);
        }
        
        if(typeA->whoami == t_person && typeB->whoami == t_safezone){
            cl_safezone_person_begin(gm, typeB, typeA);
        }
        if(typeB->whoami == t_person && typeA->whoami == t_safezone){
            cl_safezone_person_begin(gm, typeA, typeB);
        }
        if(typeB->whoami == t_gravity_area && 
                (typeA->whoami == t_hero || typeA->whoami == t_person ) ){
            cl_gravity_character_begin(gm, typeB, typeA);
        }
        if(typeA->whoami == t_gravity_area && 
                (typeB->whoami == t_hero || typeB->whoami == t_person ) ){
            cl_gravity_character_begin(gm, typeA, typeB);
        }
    }
}

void MyContactListener::EndContact(b2Contact* contact){
    actor_type * typeA = (actor_type*)contact->GetFixtureA()->GetUserData(); 
    actor_type * typeB = (actor_type*)contact->GetFixtureB()->GetUserData(); 

    if(typeA != NULL && typeB != NULL){
        if(typeA->whoami == t_portal && typeB->whoami == t_hero){
           gm->onpt = -1;
        }
        else if(typeA->whoami == t_hero && typeB->whoami == t_portal){
           gm->onpt = -1;
        }
        if(typeA->whoami == t_person && typeB->whoami == t_safezone){
            cl_safezone_person_end(gm, typeB, typeA);
        }
        else if(typeB->whoami == t_person && typeA->whoami == t_safezone){
            cl_safezone_person_end(gm, typeA, typeB);
        }
        if(typeB->whoami == t_gravity_area && 
                (typeA->whoami == t_hero || typeA->whoami == t_person ) ){
            cl_gravity_character_end(gm, typeB, typeA);
        }
        if(typeA->whoami == t_gravity_area && 
                (typeB->whoami == t_hero || typeB->whoami == t_person ) ){
            cl_gravity_character_end(gm, typeA, typeB);
        }
    }
}
void MyContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold){
}
void MyContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse){}

void MyContactListener::SetGM(game _gm){
    gm = _gm;
}


void draw_line(b2Vec2 p1, b2Vec2 p2, GLuint tex){
		glBindTexture(GL_TEXTURE_2D, tex);
		glPushMatrix();
		glBegin(GL_QUADS);
		
        b2Vec2 tt, ttt;


		glTexCoord2f(1.0, 0.0);
        tt = p1 - p2;
        ttt = tt;
        tt.x = -ttt.y;
        tt.y = ttt.x;
        tt.Normalize();
        tt *= 0.5f;
        ttt = tt;// Store this for later
        tt += p1;
		glVertex3f(tt.x, tt.y, 0.0);

		glTexCoord2f(0.0, 0.0);
        tt.x = -ttt.x;
        tt.y = -ttt.y;
        tt += p1;
		glVertex3f(tt.x, tt.y, 0.0);

		glTexCoord2f(0.0, 1.0);
        tt.x = -ttt.x;
        tt.y = -ttt.y;
        tt += p2;
		glVertex3f(tt.x, tt.y, 0.0);

		glTexCoord2f(1.0, 1.0);
        tt = ttt;
        tt += p2;
		glVertex3f(tt.x, tt.y, 0.0);

		glEnd();
		glPopMatrix();

}
