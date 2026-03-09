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
#include "lcd-fonts.h"


// helper function to print messages onto the LCD with a variable starting point
// ex. want a vertical offset of 50 => let initial_y be 50
// ex. for LCD_CMD_PRINT_MESSAGE, initial_x = LCD_MARGIN, initial_y = lcd_text_area_center_y()
bool master_mind_print_string (lcd_msg_t *msg, int initial_x, int initial_y) {
    int offset_x = 0; // how much to offset the x coordinate for each subsequent char, initialized to 0 for the first char
            
    // loop through each char (so at most 32 times) in the message string, each time drawing one char and updating offset_x
    for (int i = 0; i < 32; i++)
    {
        if (msg->payload.message[i] == '\0') // stop when we reach the end of the string
        {
            break;
        }

        // 1. get the index for the current char

        // check if the char is valid first
        if (msg->payload.message[i] < Consolas_20ptFontInfo.start_char || msg->payload.message[i] > Consolas_20ptFontInfo.end_char)
        {
            printf("Error: invalid char in message\n\r");
            return false;
        }
        uint16_t index = msg->payload.message[i] - Consolas_20ptFontInfo.start_char; 


        // 2. get the font character info for the current char using the index
        FONT_CHAR_INFO info = Consolas_20ptDescriptors[index];


        // 3. get the bitmap for the current character to be drawn
        const uint8_t *bitmap = Consolas_20ptBitmaps + info.offset;


        // 4. draw the image - here no way for user of this function to specify color
        // so we just use white for foreground and black for background
        lcd_draw_image(initial_x + offset_x, initial_y, info.width, info.height, bitmap, LCD_COLOR_WHITE, LCD_COLOR_BLACK, true);

        
        // 5. update the offset_x for the next char, so that the next char is drawn to the right of the current char

        // offset given by half the width of the current char + half the width of the next char
        offset_x += info.width/2 + Consolas_20ptDescriptors[index + 1].width/2;
    }
    return true;
}


/**
 * @brief 
 * This function will parse an LCD message and perform the appropriate action
 * @param msg 
 * @return true 
 * @return false 
 */
bool master_mind_handle_msg(lcd_msg_t* msg)
{
    // first make sure the message is not null
    if(msg == NULL) return false;

    switch (msg->command)
    {
        case LCD_CMD_DRAW_TILE: // then we should interpret the payload as a lcd_tile_t
            // get the rectangle coordinates and dimensions for the given tile
            lcd_rect_t r;
            if (lcd_tile_rect(&r, msg->payload.tile.row, msg->payload.tile.col) == false)
            {
                printf("Error: invalid tile row or col\n\r");
                return false;
            }

            /* DRAW THE TILE BACKGROUND RECTANGLE */
            // note: foreground field of rectangle = color_bg, or the tile part wouldn't blend in
            lcd_draw_rectangle(r.cx, r.cy, r.w, r.h, msg->payload.tile.color_bg, true);



            /* GET THE IMAGE (BITMAP) FOR THE NUMBER TO BE DRAWN ON THE TILE, AND DRAW IT */
            // 1. calculate the index to get the font character info for the number

            // check if the number is valid first
            if (msg->payload.tile.number < LARGE_NUMBERS_FontInfo.start_char || msg->payload.tile.number > LARGE_NUMBERS_FontInfo.end_char)
            {
                printf("Error: invalid tile number\n\r");
                return false;
            }
            uint16_t index = msg->payload.tile.number - LARGE_NUMBERS_FontInfo.start_char;


            // 2. get the font character info for the number using the index
            FONT_CHAR_INFO info = FONT_CHAR_INFO_LARGE_NUMBERS[index];


            // 3. get the bitmap for the number using the offset in the font character info
            // and the starting address of the LARGE_NUMBERS font bitmap data
            const uint8_t *bitmap = FONT_NUM_LARGE_BITMAPS + info.offset;


            // 4. draw the image - note the width and height of the image to be drawn is determined by the font character info, not the tile dimensions
            lcd_draw_image(r.cx, r.cy, info.width, info.height, bitmap, msg->payload.tile.color_fg, msg->payload.tile.color_bg, true);
            return true;

        case LCD_CMD_DRAW_TILE_INVERTED: // again interpret payload as lcd_tile_t
            // for inverted tile, we can just swap the foreground and background colors and call 
            // the same code as LCD_CMD_DRAW_TILE
            uint16_t temp = msg->payload.tile.color_bg;
            msg->payload.tile.color_bg = msg->payload.tile.color_fg;
            msg->payload.tile.color_fg = temp;

            // don't forget to also change the command to LCD_CMD_DRAW_TILE before the recursive call, otherwise we'll end up in an infinite loop
            msg->command = LCD_CMD_DRAW_TILE;

            return master_mind_handle_msg(msg); // recursive call to draw the tile

        case LCD_CMD_PRINT_MESSAGE: // payload as char messsage[32] and print it to the text area
            // for LCD_CMD_PRINT_MESSAGE, initial_x = LCD_MARGIN, initial_y = lcd_text_area_center_y()
            return master_mind_print_string(msg, LCD_MARGIN, lcd_text_area_center_y());

        case LCD_CMD_PRINT_SW1_COUNT:
            // print out the received string to the LCD, starting at (10, 50)
            return master_mind_print_string(msg, 10, 50);

        case LCD_CMD_PRINT_SW2_COUNT:
            // same thing, just print the string specified by sw2 task starting at (10, 100)
            return master_mind_print_string(msg, 10, 100);

        case LCD_CMD_CLEAR_SCREEN:
            lcd_clear_screen(LCD_COLOR_BLACK);
            return true;

        default:
            printf("Error: unknown LCD command\n\r");
            return false;
    }
}
