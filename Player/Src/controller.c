#include "controller.h"
#include "display.h"

void controller_task(void) {
    load_screen();
    render_text();
}
