#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "cmsis_os.h"

#include "memory.h"
#include "stdlib.h"
#include "stdbool.h"

#define LCD_LAYER_FG 1
#define LCD_LAYER_BG 0
#define LCD_X_SIZE RK043FN48H_WIDTH
#define LCD_Y_SIZE RK043FN48H_HEIGHT
static volatile uint32_t lcd_image_fg[LCD_Y_SIZE][LCD_X_SIZE] __attribute__((section(".sdram")));
static volatile uint32_t lcd_image_bg[LCD_Y_SIZE][LCD_X_SIZE] __attribute__((section(".sdram")));

void load_screen() {
    BSP_LCD_Init();

    BSP_LCD_LayerDefaultInit(LCD_LAYER_FG, (uint32_t) lcd_image_fg);
    BSP_LCD_LayerDefaultInit(LCD_LAYER_BG, (uint32_t) lcd_image_bg);

    BSP_LCD_DisplayOn();

    BSP_LCD_SelectLayer(LCD_LAYER_BG);
    BSP_LCD_Clear(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);

    BSP_LCD_SelectLayer(LCD_LAYER_FG);
    BSP_LCD_Clear(LCD_COLOR_TRANSPARENT);

    BSP_LCD_SetTransparency(LCD_LAYER_BG, 255);

    BSP_TS_Init(LCD_X_SIZE, LCD_Y_SIZE);
}

void render_text() {
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(100, 100, (uint8_t *) "Hello World!", CENTER_MODE);
}