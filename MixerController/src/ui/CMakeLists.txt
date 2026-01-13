SET(SOURCES screens/ui_Screen3.c
    screens/ui_Screen1.c
    screens/ui_Screen2.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_237439637.c
    images/ui_img_1771564824.c
    images/ui_img_481835706.c
    images/ui_img_1424899605.c
    images/ui_img_1916212264.c
    images/ui_img_461411260.c
    fonts/ui_font_Hebrew50.c
    fonts/ui_font_Hebrew30.c)

add_library(ui ${SOURCES})
