#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

// Константы игры
#define PLAYER_SIZE 5
#define PLAYER_X 15
#define GROUND_LEVEL 55
#define GRAVITY 0.6f
#define JUMP_FORCE -8.0f
#define OBSTACLE_WIDTH 8
#define OBSTACLE_SPACING 10
#define OBSTACLE_SPEED 2
#define SCROLL_SPEED 2
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Состояние игры
typedef struct {
    float player_y;
    float player_speed_y;
    int obstacle_group_x;
    int current_level_index;
    int score;
    bool game_over;
    bool jumping;
    bool show_flying_text;
    int flying_text_x;
    bool flying_text_has_appeared;
    float game_speed;
} GameState;

// Данные уровней
typedef struct {
    int x;
    int count;
    int heights[3];
} LevelData;

static const LevelData level_data[] = {
    {200, 1, {20, 0, 0}},
    {300, 2, {20, 40, 0}}, 
    {400, 3, {20, 40, 20}}, 
    {500, 1, {60, 0, 0}}, 
    {600, 2, {60, 20, 0}},
    {700, 3, {20, 20, 40}}, 
    {800, 1, {20, 0, 0}},
    {900, 2, {40, 60, 0}},
    {1000, 3, {60, 40, 20}},
    {1100, 1, {50, 0, 0}},
    {1200, 2, {20, 20, 0}},
    {1300, 3, {30, 30, 30}},
    {1400, 1, {60, 0, 0}},
    {1500, 2, {20, 60, 0}},
    {1600, 3, {60, 40, 20}},
    {1700, 1, {20, 0, 0}},
    {1800, 2, {40, 60, 0}},
    {1900, 3, {60, 40, 20}},
    {2000, 1, {30, 0, 0}},
    {2100, 2, {30, 30, 0}},
    {2200, 3, {60, 40, 30}},
    {2300, 1, {20, 0, 0}}, 
    {2400, 2, {20, 40, 0}},
    {2500, 3, {20, 40, 20}}, 
    {2600, 1, {60, 0, 0}},
    {2700, 2, {60, 20, 0}},
    {2800, 3, {20, 20, 40}},
    {2900, 1, {20, 0, 0}},
    {3000, 2, {40, 60, 0}},
    {3100, 3, {60, 40, 20}},
    {3200, 1, {50, 0, 0}},
    {3300, 2, {20, 20, 0}},
    {3400, 3, {30, 30, 30}},
    {3500, 1, {60, 0, 0}},
    {3600, 2, {20, 60, 0}},
    {3700, 3, {60, 40, 20}},
    {3800, 1, {20, 0, 0}},
    {3900, 2, {40, 60, 0}},
    {4000, 3, {60, 40, 20}},
    {4100, 1, {30, 0, 0}},
    {4200, 2, {30, 30, 0}},
    {4300, 3, {60, 40, 30}}
};

#define LEVEL_COUNT (sizeof(level_data) / sizeof(LevelData))

// Сброс игры
void reset_game(GameState* state) {
    state->player_y = GROUND_LEVEL - PLAYER_SIZE;
    state->player_speed_y = 0;
    state->current_level_index = 0;
    state->obstacle_group_x = level_data[0].x;
    state->score = 0;
    state->game_over = false;
    state->game_speed = 1.0f;
    state->flying_text_x = SCREEN_WIDTH;
    state->show_flying_text = false;
    state->flying_text_has_appeared = false;
}

// Обновление игры
void update_game(GameState* state) {
    if(state->game_over) return;
    
    // Прыжок
    if(state->jumping && state->player_y >= GROUND_LEVEL - PLAYER_SIZE) {
        state->player_speed_y = JUMP_FORCE;
        state->jumping = false;
    }
    
    // Гравитация
    state->player_speed_y += GRAVITY * state->game_speed;
    state->player_y += state->player_speed_y * state->game_speed;
    
    // Ограничение по земле
    if(state->player_y + PLAYER_SIZE > GROUND_LEVEL) {
        state->player_y = GROUND_LEVEL - PLAYER_SIZE;
        state->player_speed_y = 0;
    }
    
    // Движение препятствий
    state->obstacle_group_x -= OBSTACLE_SPEED * state->game_speed;
    
    // Проверка столкновений
    const LevelData* current_level = &level_data[state->current_level_index];
    for(int j = 0; j < current_level->count; j++) {
        int obs_x = state->obstacle_group_x + j * OBSTACLE_SPACING;
        int obs_y = GROUND_LEVEL - current_level->heights[j];
        int obs_height = current_level->heights[j];
        
        // Проверка видимости препятствия
        if(obs_x + OBSTACLE_WIDTH > 0 && obs_x < SCREEN_WIDTH) {
            // Проверка столкновения
            if(PLAYER_X + PLAYER_SIZE > obs_x && 
               PLAYER_X < obs_x + OBSTACLE_WIDTH &&
               state->player_y + PLAYER_SIZE > obs_y) {
                state->game_over = true;
            }
        }
    }
    
    // Проверка перехода к следующему уровню
    int clearance = 3;
    if(state->obstacle_group_x + OBSTACLE_WIDTH * clearance + 
       OBSTACLE_SPACING * (current_level->count - 1) < 0) {
        state->current_level_index++;
        if(state->current_level_index >= LEVEL_COUNT) {
            state->current_level_index = 0;
        }
        state->obstacle_group_x = level_data[state->current_level_index].x;
        state->score++;
        
        // Увеличение скорости каждые 10 очков
        if(state->score % 10 == 0) {
            state->game_speed += 0.2f;
        }
    }
    
    // Показ летающего текста
    if(state->score >= 10 && !state->flying_text_has_appeared) {
        state->show_flying_text = true;
        state->flying_text_has_appeared = true;
        state->flying_text_x = SCREEN_WIDTH;
    }
    
    // Движение летающего текста
    if(state->show_flying_text) {
        state->flying_text_x -= SCROLL_SPEED;
        if(state->flying_text_x < -100) {
            state->show_flying_text = false;
        }
    }
}

// Рендеринг игры
void render_game(Canvas* canvas, GameState* state) {
    canvas_clear(canvas);
    
    // Рисуем землю
    canvas_draw_line(canvas, 0, GROUND_LEVEL, SCREEN_WIDTH, GROUND_LEVEL);
    
    // Рисуем игрока
    canvas_draw_box(canvas, PLAYER_X, (int)state->player_y, PLAYER_SIZE, PLAYER_SIZE);
    
    // Рисуем препятствия
    const LevelData* current_level = &level_data[state->current_level_index];
    for(int j = 0; j < current_level->count; j++) {
        int obs_x = state->obstacle_group_x + j * OBSTACLE_SPACING;
        int obs_y = GROUND_LEVEL - current_level->heights[j];
        int obs_height = current_level->heights[j];
        
        if(obs_x + OBSTACLE_WIDTH > 0 && obs_x < SCREEN_WIDTH) {
            canvas_draw_triangle(canvas, obs_x, obs_y + obs_height, 
                                obs_x + OBSTACLE_WIDTH/2, obs_y,
                                obs_x + OBSTACLE_WIDTH, obs_y + obs_height);
        }
    }
    
    // Рисуем счет
    char score_str[20];
    snprintf(score_str, sizeof(score_str), "Score: %d", state->score);
    canvas_draw_str(canvas, 5, 10, score_str);
    
    // Рисуем летающий текст
    if(state->show_flying_text) {
        canvas_draw_str(canvas, state->flying_text_x, 20, "by @Tiramisu2_0");
    }
    
    // Рисуем экран окончания игры
    if(state->game_over) {
        canvas_draw_str(canvas, 40, 30, "Game Over!");
        char final_score[20];
        snprintf(final_score, sizeof(final_score), "Score: %d", state->score);
        canvas_draw_str(canvas, 40, 40, final_score);
    }
}

// Обработчик ввода
void input_callback(InputEvent* input_event, void* ctx) {
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

// Основная точка входа
int32_t jump_game_app(void* p) {
    UNUSED(p);
    
    // Создаем состояние игры
    GameState* state = malloc(sizeof(GameState));
    reset_game(state);
    
    // Инициализация GUI
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, (ViewPortDrawCallback)render_game, state);
    view_port_input_callback_set(view_port, input_callback, event_queue);
    
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    
    InputEvent event;
    bool running = true;
    
    while(running) {
        // Обработка ввода
        if(furi_message_queue_get(event_queue, &event, 50) == FuriStatusOk) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                    case InputKeyOk:
                        state->jumping = true;
                        break;
                    case InputKeyBack:
                        running = false;
                        break;
                    default:
                        break;
                }
            }
        }
        
        // Обновление игры
        update_game(state);
        
        // Перерисовка
        view_port_update(view_port);
    }
    
    // Очистка
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_record_close(RECORD_GUI);
    free(state);
    
    return 0;
}
