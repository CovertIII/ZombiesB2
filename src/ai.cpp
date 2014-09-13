#include "vector2.h"
#include "physics.h"
#include <Box2D/Box2D.h>
#include "ai.h"

enum{
    pt_m,
    pt_l,
    pt_L,
    pt_z,
    pt_none
};   

typedef struct {
    const char * buf;
    char * cx;
    char * cy;
    int buf_len;
    int type;

    b2Vec2 pt;
} AiNode; 

int isCommand(char d);

void ai_chase(object * target, object * chaser, double vec, double force){
    vector2 line = v2Sub(chaser->p, target->p);
    float time = v2Len(line)/vec;

    vector2 p2 = v2Add(target->p, v2sMul(time, target->v));
    vector2 p1 = chaser->p;
    vector2 pp = v2Sub(p2,p1);
    vector2 fc = v2sMul(force, v2Unit(pp));
    chaser->f = v2Add(chaser->f, fc);
}

void ai_seek(vector2 target, object * chaser, double force, double length){
    vector2 line = v2Sub(chaser->p, target);
    float len = v2Len(line);
    float var = len > 20 ? 20 : len;
    vector2 fc = v2sMul(-force/len * var , line);
    chaser->f = v2Add(chaser->f, fc);
}


void ai_avoid(vector2 target, object * chaser, double force){
    vector2 line = v2Sub(chaser->p, target);
    vector2 fc = v2sMul(force/v2Len(line), v2Unit(line));
    chaser->f = v2Add(chaser->f, fc);
}

AiPath::AiPath(void){
    marker = 0;
}

void AiPath::parsePath(const char * d, float height){
    //Initize crap
    marker = 0;
    closed = false;
    dir = 1;
    rad = 4;

    p_num = 0;
    //
    // This gets the string stuff 
    //
    AiNode node[256];
    int node_num = 0;

    int i = 0;
    int buf_len = 0;
    int type;
    for(i = 0; (d[i] != NULL && node_num < 256) ; i++){
        type = isCommand(d[i]);
        if(type == pt_none){
            buf_len++;
        }
        else {
            node[node_num].type = type;
            if(node_num > 0){
                node[node_num - 1].buf_len = buf_len;
                node[node_num - 1].buf = d + i - buf_len;
            }
            if(type == pt_z){
                node[node_num].buf_len = 0;
                node[node_num].buf = NULL;
                closed = true;
            }else{
                node_num++;
            }


            buf_len = 0;
        }
    }

    //This parses the string
    //
    printf("%s\n", d);
    for(i = 0; i < node_num; i++){
        int k;
        bool use_x = true;

        AiNode * thisnode = &node[i];
        thisnode->cx = (char*)malloc(sizeof(char)*thisnode->buf_len);
        thisnode->cy = (char*)malloc(sizeof(char)*thisnode->buf_len);
        int cx_len = 0;
        int cy_len = 0;

        printf("%d ", thisnode->type);
        for(k = 0; k < thisnode->buf_len; k++){
            if(thisnode->buf[k] == ','){
                use_x = false;
            }else{
                if(use_x){
                    thisnode->cx[cx_len] = thisnode->buf[k];
                    cx_len++;
                }else{
                    thisnode->cy[cy_len] = thisnode->buf[k];
                    cy_len++;
                }
            }
        }

        thisnode->cx[cx_len] = NULL;
        thisnode->cy[cy_len] = NULL;

        sscanf(thisnode->cx, "%f", &thisnode->pt.x);
        sscanf(thisnode->cy, "%f", &thisnode->pt.y);

        free(thisnode->cx);
        free(thisnode->cy);

        //Pass it on to the path class
        if(node[i].type == pt_m || node[i].type == pt_L){
            p[i].x = node[i].pt.x;
            p[i].y = node[i].pt.y;
        }else{
            p[i].x = node[i].pt.x + p[i-1].x;
            p[i].y = node[i].pt.y + p[i-1].y;
        }

        printf("x: %f y: %f\n", p[i].x, p[i].y);
    }

    for(i = 0; i < node_num; i++){
        p[i].y = height - p[i].y;
    }

    p_num = node_num;
}

void AiPath::moveToNextPoint(){
    if(closed){
        marker++;
        if(marker >= p_num){
            marker = 0;
        }
    }
    else{
        marker += dir;
        if(dir == 1){
            if(marker == p_num){
                dir = -1;
                marker -= 2;
            }
        }else{
            if(marker == -1){
                dir = 1;
                marker += 2;
            }
        }
    }
}

void AiPath::updatePath(b2Vec2 pt){
    pt = pt - p[marker];
    if(pt.LengthSquared() < rad*rad){
        moveToNextPoint();
    }
}

b2Vec2 AiPath::getTarget(){
    return p[marker];
}

int isCommand(const char d){
    switch (d){
        case 'm':
            return pt_m;
        case 'M':
            return pt_m;
        case 'l':
            return pt_l;
        case 'L':
            return pt_L;
        case 'Z':
        case 'z':
            return pt_z;
        default:
            return pt_none;
    }
}
