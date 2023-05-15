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

// Back button
static const Point back_icon_center = {VW_TO_PX(20), VH_TO_PX(80)};
static const Point back_icon_points[] = {
        {0,  15},
        {20, 0},
        {20, 30}
};
static Button back_button = {
        .center_position = {VW_TO_PX(20), VH_TO_PX(80)},
        .boundaries = {30, 30},
        .is_touched = false
};

// Next button
static const Point next_icon_center = {VW_TO_PX(80), VH_TO_PX(80)};
static const Point next_icon_points[] = {
        {0,  0},
        {20, 15},
        {0,  30}
};
static Button next_button = {
        .center_position = {VW_TO_PX(80), VH_TO_PX(80)},
        .boundaries = {30, 30},
        .is_touched = false
};

// Play button
static const Point play_icon_center = {VW_TO_PX(50), VH_TO_PX(80)};
static const int play_icon_radius = 40;
static const Point play_icon_points[] = {
        {0,  0},
        {20, 15},
        {0,  30}
};
static Button play_button = {
        .center_position = {VW_TO_PX(50), VH_TO_PX(80)},
        .boundaries = {30, 30},
        .is_touched = false
};

// Pause button
static const Point pause_icon_positions[] = {
        {VW_TO_PX(48), VH_TO_PX(80)},
        {VW_TO_PX(52), VH_TO_PX(80)}
};
static const Point pause_icon_points[] = {
        {0, 0},
        {5, 0},
        {5, 30},
        {0, 30}
};

// Progress bar
static const Point progress_bar_boundaries[] = {
        {10, 150},
        {DISPLAY_WIDTH - 10, 160}
};

void initialize_screen() {
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

void draw_circle(const Point center_position, int radius, uint32_t color) {
    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillCircle(center_position.X, center_position.Y, radius);
}

void draw_polygon(const Point center_position, const Point *icon_points, uint16_t icon_points_count, uint32_t color) {
    BSP_LCD_SetTextColor(color);

    Point transformed_points[icon_points_count];
    transform_points(center_position, icon_points, icon_points_count, transformed_points);

    BSP_LCD_FillPolygon(transformed_points, icon_points_count);
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
    return touch_state.touchX[0] > button.center_position.X - button.boundaries.X / 2 &&
           touch_state.touchX[0] < button.center_position.X + button.boundaries.X / 2 &&
           touch_state.touchY[0] > button.center_position.Y - button.boundaries.Y / 2 &&
           touch_state.touchY[0] < button.center_position.Y + button.boundaries.Y / 2;
}

void handle_touch() {
    unsigned last_touch_tick = osKernelSysTick();
    TS_StateTypeDef touch_state;
    BSP_TS_GetState(&touch_state);


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
                     (int)(progress * (progress_bar_boundaries[1].X - progress_bar_boundaries[0].X - 4)),
                     progress_bar_boundaries[1].Y - progress_bar_boundaries[0].Y - 4);
}

void render_track_screen(const char *track_name, const char *artist_name, int total_files_count, int current_file_index,
                         double progress, double duration, bool is_playing) {
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    render_h3(track_name, VW_TO_PX(5), VH_TO_PX(20));
    render_h4(artist_name, VW_TO_PX(5), VH_TO_PX(30));

    draw_progress_bar(progress);

    draw_polygon(back_icon_center, back_icon_points, COUNT(back_icon_points), LCD_COLOR_WHITE);
    draw_polygon(next_icon_center, next_icon_points, COUNT(next_icon_points), LCD_COLOR_WHITE);

    if (is_playing) {
        draw_circle(play_icon_center, play_icon_radius, LCD_COLOR_WHITE);
        draw_polygon(play_icon_center, play_icon_points, COUNT(play_icon_points), LCD_COLOR_BLACK);
    } else {
        draw_polygon(pause_icon_positions[0], pause_icon_points, COUNT(pause_icon_points), LCD_COLOR_BLACK);
        draw_polygon(pause_icon_positions[1], pause_icon_points, COUNT(pause_icon_points), LCD_COLOR_BLACK);
    }

    swap_screen_layers();
}
