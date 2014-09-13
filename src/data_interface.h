typedef struct data_record_type * data_record;


data_record init_data_record(char * file_name, char * resource_path);

void stats_list_prep(data_record db);

void render_level_scores(data_record db, int width, int height);

void stats_reload_fonts(data_record db);

int db_get_lives(data_record db);

void game_record_lvl_stats(data_record db, char * lvl, double time_lvl, int extra_ppl, int complete);

void game_leave(data_record db, int lives, int deaths);

int user_nkey_down(data_record db, unsigned char key);

int user_nkey_up(data_record db, unsigned char key);

int db_get_save_count(data_record db);

int db_get_continues(data_record db);

int db_get_user_id(data_record db);

void user_skey_down(data_record db, int key);

void game_over(data_record db, int deaths);

void stats_render_lvl(data_record db, char * lvl, int width, int height);

int stats_render(data_record db, int width, int height);

void game_start_session(data_record db);

void game_end_session(data_record db);
