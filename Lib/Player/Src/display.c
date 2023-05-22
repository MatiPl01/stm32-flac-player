#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "cmsis_os.h"

#include "display.h"
#include "player.h"

#define LCD_LAYER_FG 1
#define LCD_LAYER_BG 0
#define DISPLAY_WIDTH RK043FN48H_WIDTH
#define DISPLAY_HEIGHT RK043FN48H_HEIGHT

#define VW_TO_PX(vw) ((vw * DISPLAY_WIDTH) / 100)
#define VH_TO_PX(vh) ((vh * DISPLAY_HEIGHT) / 100)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define COUNT(x) (sizeof(x)/sizeof(x[0]))

static int current_layer = LCD_LAYER_FG;

static volatile uint32_t lcd_image_fg[DISPLAY_HEIGHT][DISPLAY_WIDTH] __attribute__((section(".sdram")));
static volatile uint32_t lcd_image_bg[DISPLAY_HEIGHT][DISPLAY_WIDTH] __attribute__((section(".sdram")));

// SHAPES
// Rectangle
// create rect using CREATE_SHAPE macro
static const Point rect_points[] = {
        {0, 0},
        {0, 30},
        {7, 30},
        {7, 0}
};

// Triangle
static const Point triangle_points[] = {
        {0,  0},
        {20, 15},
        {0,  30}
};

// Chevron
// Right
static const Point chevron_right_points[] = {
        {5,  0},
        {20, 15},
        {5,  30},
        {0,  25},
        {15, 15},
        {0,  5}

};

// Left
static const Point chevron_left_points[] = {
        {15, 0},
        {0,  15},
        {15, 30},
        {20, 25},
        {5,  15},
        {20, 5}
};

// BUTTONS
// Back button
static const TranslatedShape chevron_left_icon_shapes[] = {
        {
                .shape = {
                        .points = chevron_left_points,
                        .points_count = COUNT(chevron_left_points),
                },
                .translation = {-2, 0}
        }
};
static Button back_button = {
        .center_position = {VW_TO_PX(20), VH_TO_PX(80)},
        .radius = 25,
        .icon = {
                .shapes = chevron_left_icon_shapes,
                .shapes_count = 1,
        },
        .last_changed_state = 0,
        .is_touched = false,
        .active = false
};

// Next button
static const TranslatedShape chevron_right_icon_shapes[] = {
        {
                .shape = {
                        .points = chevron_right_points,
                        .points_count = COUNT(chevron_right_points),
                },
                .translation = {2, 0}
        }
};
static Button next_button = {
        .center_position = {VW_TO_PX(80), VH_TO_PX(80)},
        .radius = 25,
        .icon = {
                .shapes = chevron_right_icon_shapes,
                .shapes_count = 1,
        },
        .last_changed_state = 0,
        .is_touched = false,
        .active = false
};

// Play button
static const TranslatedShape play_icon_shapes[] = {
        {
                .shape = {
                        .points = triangle_points,
                        .points_count = COUNT(triangle_points),
                },
                .translation = {2, 0}
        }
};
static Button play_button = {
        .center_position = {VW_TO_PX(50), VH_TO_PX(80)},
        .radius = 35,
        .icon = {
                .shapes = play_icon_shapes,
                .shapes_count = 1,
        },
        .last_changed_state = 0,
        .is_touched = false,
        .active = false
};

// Pause button
static const TranslatedShape pause_icon_shapes[] = {
        {
                .shape = {
                        .points = rect_points,
                        .points_count = COUNT(rect_points),
                },
                .translation = {-8, 0}
        },
        {
                .shape = {
                        .points = rect_points,
                        .points_count = COUNT(rect_points),
                },
                .translation = {8, 0}
        }
};
static Button pause_button = {
        .center_position = {VW_TO_PX(50), VH_TO_PX(80)},
        .radius = 35,
        .icon = {
                .shapes = pause_icon_shapes,
                .shapes_count = 2,
        },
        .last_changed_state = 0,
        .is_touched = false,
        .active = false
};

// Buttons
static Button *buttons[] = {
        &back_button,
        &next_button,
        &play_button,
        &pause_button
};

// Progress bar
static const Point progress_bar_boundaries[] = {
        {10, 150},
        {DISPLAY_WIDTH - 10, 160}
};

void initialize_screen() {
    // Initialize screen
    BSP_LCD_Init();

    BSP_LCD_LayerDefaultInit(LCD_LAYER_FG, (uint32_t) lcd_image_fg);
    BSP_LCD_LayerDefaultInit(LCD_LAYER_BG, (uint32_t) lcd_image_bg);

    BSP_LCD_SelectLayer(LCD_LAYER_BG);
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    BSP_LCD_SelectLayer(LCD_LAYER_FG);
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    BSP_LCD_SetLayerVisible(current_layer, ENABLE);
    BSP_LCD_DisplayOn();

    BSP_TS_Init(DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

void render_text(const char *text, int x, int y, sFONT *font, uint32_t color, uint32_t background_color,
                 Text_AlignModeTypdef alignment) {
    BSP_LCD_SetBackColor(background_color);
    BSP_LCD_SetTextColor(color);

    BSP_LCD_SetFont(font);

    BSP_LCD_DisplayStringAt(x, y, (uint8_t *) text, alignment);
}

void render_heading(const char *text, int x, int y, sFONT *font) {
    render_text(text, x, y, font, LCD_COLOR_WHITE, LCD_COLOR_TRANSPARENT, LEFT_MODE);
}

void render_h1(const char *text, int x, int y) {
    render_heading(text, x, y, &Font24);
}

void render_h2(const char *text, int x, int y) {
    render_heading(text, x, y, &Font20);
}

void render_h3(const char *text, int x, int y) {
    render_heading(text, x, y, &Font16);
}

void render_h4(const char *text, int x, int y) {
    render_heading(text, x, y, &Font12);
}

void render_paragraph(const char *text, int x, int y) {
    render_heading(text, x, y, &Font8);
}

void transform_points(const Point center_position, const Point *icon_points, uint16_t points_count,
                      Point *transformed_points) {
    BoundingRect bounding_rect = {
            .x1 = icon_points[0].X,
            .y1 = icon_points[0].Y,
            .x2 = icon_points[0].X,
            .y2 = icon_points[0].Y
    };

    for (uint16_t i = 0; i < points_count; i++) {
        bounding_rect.x1 = MIN(bounding_rect.x1, icon_points[i].X);
        bounding_rect.y1 = MIN(bounding_rect.y1, icon_points[i].Y);
        bounding_rect.x2 = MAX(bounding_rect.x2, icon_points[i].X);
        bounding_rect.y2 = MAX(bounding_rect.y2, icon_points[i].Y);
    }

    int icon_width = bounding_rect.x2 - bounding_rect.x1;
    int icon_height = bounding_rect.y2 - bounding_rect.y1;

    for (uint16_t i = 0; i < points_count; i++) {
        transformed_points[i].X = center_position.X + icon_points[i].X - icon_width / 2;
        transformed_points[i].Y = center_position.Y + icon_points[i].Y - icon_height / 2;
    }
}

bool is_button_touched(Button button, TS_StateTypeDef touch_state) {
    return touch_state.touchX[0] > button.center_position.X - button.radius &&
           touch_state.touchX[0] < button.center_position.X + button.radius &&
           touch_state.touchY[0] > button.center_position.Y - button.radius &&
           touch_state.touchY[0] < button.center_position.Y + button.radius;
}

void handle_touch() {
    unsigned current_tick = osKernelSysTick();
    TS_StateTypeDef touch_state;
    BSP_TS_GetState(&touch_state);

    for (int i = 0; i < COUNT(buttons); i++) {
        Button *button = buttons[i];
        if (button->disabled) continue;
        bool is_touched = is_button_touched(*button, touch_state);
        if (is_touched != button->is_touched) {
            if (current_tick - button->last_changed_state >= 100) {
                if (!button->is_touched && is_touched) {
                    button->active = true;
                }
                button->is_touched = is_touched;
            }
            button->last_changed_state = current_tick;
        }
    }
}

bool is_back_button_active() {
    bool active = back_button.active;
    back_button.active = false;
    return active;
}

bool is_next_button_active() {
    bool active = next_button.active;
    next_button.active = false;
    return active;
}

bool is_play_button_active() {
    bool active = play_button.active;
    play_button.active = false;
    return active;
}

bool is_pause_button_active() {
    bool active = pause_button.active;
    pause_button.active = false;
    return active;
}

void swap_screen_layers() {
    // Wait for VSYNC
    while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));

    current_layer = !current_layer;
    BSP_LCD_SetLayerVisible(current_layer, ENABLE);
    BSP_LCD_SetLayerVisible(!current_layer, DISABLE);
    BSP_LCD_SelectLayer(!current_layer);
}

void render_info_screen(const char *info, const char *sub_info) {
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    render_h1(info, VW_TO_PX(10), VH_TO_PX(40));
    render_h2(sub_info, VW_TO_PX(10), VH_TO_PX(60));

    swap_screen_layers();
}

void draw_progress_bar(double progress) {
    // Create the outer rectangle
    BSP_LCD_SetTextColor(LCD_COLOR_GRAY);
    BSP_LCD_FillRect(progress_bar_boundaries[0].X, progress_bar_boundaries[0].Y,
                     progress_bar_boundaries[1].X - progress_bar_boundaries[0].X,
                     progress_bar_boundaries[1].Y - progress_bar_boundaries[0].Y);
    // Create the inner rectangle
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(progress_bar_boundaries[0].X + 2, progress_bar_boundaries[0].Y + 2,
                     (int) (progress * (progress_bar_boundaries[1].X - progress_bar_boundaries[0].X - 4)),
                     progress_bar_boundaries[1].Y - progress_bar_boundaries[0].Y - 4);
}

void draw_shape(Shape shape, Point position, uint32_t color) {
    BSP_LCD_SetTextColor(color);
    // Translate shape points to the target position
    Point transformed_points[shape.points_count];
    transform_points(position, shape.points, shape.points_count, transformed_points);
    // Draw the shape
    BSP_LCD_FillPolygon(transformed_points, shape.points_count);
}

void draw_icon(const Icon icon, Point position, uint32_t color) {
    for (uint16_t i = 0; i < icon.shapes_count; i++) {
        // Translate shape points to the target position
        TranslatedShape translated_shape = icon.shapes[i];
        // Calculate teh resulting absolute shape position based on icon position and translation
        Point absolute_shape_position = {
                position.X + translated_shape.translation.X,
                position.Y + translated_shape.translation.Y
        };
        // Draw the shape
        draw_shape(translated_shape.shape, absolute_shape_position, color);
    }
}

void draw_button(const Button button, uint32_t background_color, uint32_t icon_color, uint32_t active_background_color,
                 uint32_t active_icon_color) {
    // Draw circular button background with button radius on the target center position
    // Display the active background color if the time after last active state change is less than 300ms
    uint32_t bg_color;
    uint32_t i_color;
    if (button.active && osKernelSysTick() - button.last_changed_state < 300) {
        bg_color = active_background_color;
        i_color = active_icon_color;
    } else {
        bg_color = background_color;
        i_color = icon_color;
    }
    BSP_LCD_SetTextColor(bg_color);
    BSP_LCD_FillEllipse(button.center_position.X, button.center_position.Y, button.radius, button.radius);
    // Draw button icon
    draw_icon(button.icon, button.center_position, i_color);
}

void render_track_screen(const char *track_name, const char *artist_name, int total_files_count, int current_file_index,
                         double progress, double duration, bool is_playing) {
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    render_h3(track_name, VW_TO_PX(5), VH_TO_PX(20));
    render_h4(artist_name, VW_TO_PX(5), VH_TO_PX(30));

    draw_progress_bar(progress);

    // Back button
    draw_button(back_button, LCD_COLOR_GRAY, LCD_COLOR_WHITE, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
    // Next button
    draw_button(next_button, LCD_COLOR_GRAY, LCD_COLOR_WHITE, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
    // Play button
    if (is_playing) {
        draw_button(pause_button, LCD_COLOR_WHITE, LCD_COLOR_BLACK, LCD_COLOR_GRAY, LCD_COLOR_WHITE);
        play_button.disabled = true;
        pause_button.disabled = false;
    } else {
        draw_button(play_button, LCD_COLOR_WHITE, LCD_COLOR_BLACK, LCD_COLOR_GRAY, LCD_COLOR_WHITE);
        play_button.disabled = false;
        pause_button.disabled = true;
    }

    swap_screen_layers();
}
