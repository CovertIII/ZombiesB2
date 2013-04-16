typedef struct gametype *game;

game gm_init(char * res_path);

int gm_init_textures(game gm);

void gm_init_sounds(game gm);

int gm_load_bk(game gm, char * file);

int gm_load_level_svg(game gm, char * file_path);

void gm_free_level(game gm);

void gm_update(game gm, int height, int width, double dt);

int gm_progress(game gm);

void gm_free(game gm);

void gm_mouse(game gm, int x, int y);

void gm_skey_down(game gm, int key);

void gm_skey_up(game gm, int key);	

void gm_nkey_up(game gm, unsigned char key);

void gm_nkey_down(game gm, unsigned char key);

void gm_render(game gm);

b2Vec2 gm_get_hero(game gm);

void gm_set_db_string(game gm, char * db_path);

void gm_set_hero(game gm, b2Vec2 hero);

char * gm_portal(game gm);

char * gm_portal_check(game gm);

void gm_check_portals(game gm, int save_count);

void gm_portal_ct(game gm, int user_id);

void gm_reshape(game gm, int width, int height);

vector2 gm_dim(game gm);

void gm_set_view(int width, int height, game gm);


void gm_message_render(game gm, int width, int height);

void gm_stats(game gm, double * time, int * people);

class MyContactListener : public b2ContactListener
{
    public:
        void BeginContact(b2Contact* contact);
        void EndContact(b2Contact* contact);
        void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
        void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse);
        void SetGM(game _gm);

    private:
        game gm;
};
