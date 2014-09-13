
void ai_chase(object * target, object * chaser, double vec, double force);

void ai_seek(vector2 target, object * chaser, double force, double length);

void ai_avoid(vector2 target, object * chaser, double force);

class AiPath {
    public:
        AiPath();

        void parsePath(const char * d, float height);

        void updatePath(b2Vec2 pt);

        b2Vec2 getTarget();

        int p_num;
        b2Vec2 p[255];

	
        int marker;
        bool closed;
        int dir;
        float rad;


        void moveToNextPoint();
};       
