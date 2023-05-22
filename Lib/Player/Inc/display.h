#include "stdbool.h"
#include "stm32746g_discovery_lcd.h"

#ifndef STM32_FLAC_PLAYER_DISPLAY_H
#define STM32_FLAC_PLAYER_DISPLAY_H
typedef struct {
    int x1;
    int y1;
    int x2;
    int y2;
} BoundingRect;

typedef struct {
    const Point *points;
    const uint16_t points_count;
} Shape;

typedef struct {
    const Shape shape;
    const Point translation;
} TranslatedShape;

typedef struct {
    const TranslatedShape *shapes;
    const uint16_t shapes_count;
} Icon;

typedef struct {
    const Point center_position;
    const uint16_t radius;
    const Icon icon;
    unsigned last_changed_state;
    bool is_touched;
    bool active;
    bool disabled;
} Button;

void initialize_screen();

void render_text(const char *text, int x, int y, sFONT *font, uint32_t color, uint32_t background_color,
                 Text_AlignModeTypdef alignment);

void handle_touch();

void draw_circle(Point center_position, int radius, uint32_t color);

void draw_polygon(Point center_position, const Point *icon_points, uint16_t icon_points_count, uint32_t color);

void
transform_points(Point center_position, const Point *icon_points, uint16_t points_count, Point *transformed_points);

void render_info_screen(const char *info, const char *sub_info);

void render_track_screen(const char *track_name, const char *artist_name, int total_files_count, int current_file_index,
                         double progress, double duration, bool is_playing);

bool is_back_button_active(void);
bool is_next_button_active(void);
bool is_play_button_active(void);
bool is_pause_button_active(void);

#endif //STM32_FLAC_PLAYER_DISPLAY_H
