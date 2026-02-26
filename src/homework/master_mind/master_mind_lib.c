/**
 * @file master_mind.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-01-06
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "main.h"
#include "drivers.h"
#include "master_mind_lib.h"
#include "lcd-io.h"

//SAVE FOR FUTURE USE
/** prints a text message to the LCD
 * @param msg a pointer to an lcd_msg_t struct that contains the message to be printed
 */
bool lcd_print_message(lcd_msg_t* msg, uint16_t currX, uint16_t currY) {

    //iterate until null terminator
    for (int i = 0; msg->payload.message[i] != '\0'; i++) {

        //get next character
        char c = msg->payload.message[i];

        //skip any unsupported character
        if (c < Consolas_20ptFontInfo.start_char || c > Consolas_20ptFontInfo.end_char) {
            printf("Unsupported character in string, terminating print: %c\n", c);
            return false;
        }

        //get relevant info
        uint16_t index  = c - Consolas_20ptFontInfo.start_char;
        uint16_t width  = Consolas_20ptFontInfo.char_info[index].width;
        uint16_t height = Consolas_20ptFontInfo.char_info[index].height;
        uint16_t offset = Consolas_20ptFontInfo.char_info[index].offset;

        const uint8_t* bitmap = Consolas_20ptFontInfo.bitmap + offset;

        //draw the character
        lcd_draw_image(
            currX,
            currY,
            width,
            height,
            bitmap,
            LCD_COLOR_WHITE,
            LCD_COLOR_BLACK,
            false
        );

        //move to next print location
        currX += width;

        //TODO if longer messages, may need to support looping to next row
        //For the purposes of HW01, the method is fine as is
    }
    //success
    return true;
}


//SAVE FOR FUTURE USE
/** Draws a non-inverted tile to the LCD
 * To draw inverted tile - the user is expected to switch the foreground and background 
 * colors before calling this method
 * @param msg a pointer to an lcd_msg_t struct that contains the relevant tile information
 */
bool lcd_draw_tile(lcd_msg_t* msg) {
    lcd_rect_t r;

    //attempt to create rectangle
    if (!lcd_tile_rect(&r, msg->payload.tile.row, msg->payload.tile.col)) {
    printf("Error: Invalid tile row/col\n");
    return false;
    }

    //draw background rectangle
    lcd_draw_rectangle(
        r.cx,
        r.cy,
        r.w,
        r.h,
        msg->payload.tile.color_bg,
        true 
    );

    //draw number inside tile
    uint8_t num = msg->payload.tile.number;

    lcd_draw_image(
        r.cx,
        r.cy,
        FONT_CHAR_INFO_LARGE_NUMBERS[num].width,
        FONT_CHAR_INFO_LARGE_NUMBERS[num].height,
        FONT_NUM_LARGE_BITMAPS + FONT_CHAR_INFO_LARGE_NUMBERS[num].offset,
        msg->payload.tile.color_fg,
        msg->payload.tile.color_bg,
        true
    );
    return true;
}



/**
 * @brief 
 * This function will parse an LCD message and perform the appropriate action
 * @param msg 
 * @return true 
 * @return false 
 */
bool master_mind_handle_msg(lcd_msg_t* msg) {
        switch(msg->command) {
            case LCD_CMD_PRINT_MESSAGE:
                //see above method for implementation
                return lcd_print_message(msg, 0, 0);
            case LCD_CMD_DRAW_TILE:
                //see above method for implementation
                return lcd_draw_tile(msg);;
            case LCD_CMD_DRAW_TILE_INVERTED:
                //switch foreground and background colors
                lcd_tile_t* tile = &msg->payload.tile;
                uint16_t temp_color = tile->color_fg;
                tile->color_fg = tile->color_bg;
                tile->color_bg = temp_color;
                
                //draw as standard tile
                return lcd_draw_tile(msg);
            case LCD_CMD_CLEAR_SCREEN:
                lcd_clear_screen(LCD_COLOR_BLACK);
                return true;
            default:
                printf("!!!Error: Unrecognized LCD Command: %d\n", msg->command);
                return false;
        }
    return false;
}