 /**
 * @file hw05.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "hw05.h"
#include "master_mind_lib.h"


#if defined(HW05)

char APP_DESCRIPTION[] = "ECE353 S26 HW05";

#define HW05_READY_STATUS_ACK_TIMEOUT_MS (500U)
#define HW05_EEPROM_HIGH_SCORE_ADDR      (0x0000U)

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_i2c_t *I2C_Monarch_Obj;
cyhal_spi_t *SPI_Monarch_Obj;
EventGroupHandle_t ECE353_RTOS_Events = NULL; //May or may not be used

SemaphoreHandle_t SPI_Semaphore;
SemaphoreHandle_t I2C_Semaphore;

QueueHandle_t Queue_LCD;
static QueueHandle_t Queue_Cap_Touch_Response;
static QueueHandle_t Queue_EEPROM_Response;

bool darkMode;
/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
/**
 * @brief 
 * This function is used to initialize any semaphores used in the application.
 * 
 * The I2C and SPI busses are both shared resources that will require a 
 * semaphore to protect access to them.  You should create a binary semaphore 
 * for each bus and give the semaphore once after creating it to ensure that 
 * it is available for use.
 */
static void hw05_semaphores_init(void) {
    // Create binary semaphores
    SPI_Semaphore = xSemaphoreCreateBinary();
    I2C_Semaphore = xSemaphoreCreateBinary();

    // Ensure they start available
    xSemaphoreGive(SPI_Semaphore);
    xSemaphoreGive(I2C_Semaphore);
}

/* If you are going to create any queues outside of the tasks 
*  create them in this function.  You will also need to allocate the queues
* as global variables above.
*
* If you have created the queues in other task files, then you can leave this 
* function empty.
*/
static void hw05_queues_init(void)
{
    /* ADD CODE */
    Queue_Request_Cap_Touch = xQueueCreate(1, sizeof(device_request_msg_t));
    if (Queue_Request_Cap_Touch == NULL)
    {
        printf("Failed to create Cap Touch Request Queue\n\r");
        CY_ASSERT(0);
    }

    Queue_Cap_Touch_Response = xQueueCreate(1, sizeof(device_response_msg_t));
    if (Queue_Cap_Touch_Response == NULL)
    {
        printf("Failed to create Cap Touch Response Queue\n\r");
        CY_ASSERT(0);
    }
    
    //Joystick queue
    Queue_Requests_Joystick = xQueueCreate(4, sizeof(device_request_msg_t));
    if (Queue_Requests_Joystick == NULL) {
        printf("Failed to create Queue_Requests_Joystick\n\r");
        while (1);
    }

    /* Queue for sending LCD draw/update requests */
    Queue_LCD = xQueueCreate(8, sizeof(lcd_msg_t));
    if (Queue_LCD == NULL) {
        printf("Failed to create Queue_LCD\n\r");
        while (1);
    }
  

    Queue_EEPROM_Response = xQueueCreate(1, sizeof(device_response_msg_t));
    if (Queue_EEPROM_Response == NULL)
    {
        printf("Failed to create EEPROM Response Queue\n\r");
        CY_ASSERT(0);
    }
}  

/**
 * @brief Clears the LCD text band used for status text.
 */
static void hw05_clear_status_area(void)
{
    if(darkMode) {
        lcd_draw_rectangle(
            LCD_W / 2,
            (LCD_TEXT_H + (2 * LCD_MARGIN)) / 2,
            LCD_W,
            LCD_TEXT_H + (2 * LCD_MARGIN),
            LCD_COLOR_BLACK,
            true
        );
    }else {
        lcd_draw_rectangle(
            LCD_W / 2,
            (LCD_TEXT_H + (2 * LCD_MARGIN)) / 2,
            LCD_W,
            LCD_TEXT_H + (2 * LCD_MARGIN),
            LCD_COLOR_WHITE,
            true
        );
    }
    
}

/**
 * @brief Draw the status header at the top of the LCD.
 */
static void hw05_draw_status_header(
    uint8_t high_score,
    uint8_t exact_matches,
    uint8_t misplaced_matches)
{
    lcd_msg_t msg;

    hw05_clear_status_area();

    if (high_score == 0xFF)
    {
        high_score = 0;
    }

    msg.command = LCD_CMD_PRINT_MESSAGE;
    snprintf(msg.payload.message, sizeof(msg.payload.message), "Best: %02u", high_score);
    lcd_print_message(&msg, 0, 0);

    msg.command = LCD_CMD_PRINT_MESSAGE;
    //TODO maybe try to print exact_matches in green and misplaced in yellos? 
    snprintf(msg.payload.message, sizeof(msg.payload.message), "Last: %1u %1u", exact_matches, misplaced_matches);
    lcd_print_message(&msg, 130, 0);
}

/**
 * @brief Draw a four-tile row with optional highlight.
 */
static void hw05_draw_tile_row(
    lcd_row_t row,
    const uint8_t digits[4],
    uint16_t color_fg,
    uint16_t color_bg,
    int highlighted_col)
{
    lcd_msg_t msg;

    for (int col = 0; col < 4; col++)
    {
        msg.command = (col == highlighted_col) ? LCD_CMD_DRAW_TILE_INVERTED
                                               : LCD_CMD_DRAW_TILE;
        msg.payload.tile.row = row;
        msg.payload.tile.col = col;
        msg.payload.tile.number = digits[col];
        msg.payload.tile.color_fg = color_fg;
        msg.payload.tile.color_bg = color_bg;
        master_mind_handle_msg(&msg);
    }
}

/**
 * @brief Read the high score from EEPROM.
 */
static uint8_t hw05_read_high_score(void)
{
    uint8_t high_score = 0;

    if (!system_sensors_eeprom_read(
            Queue_EEPROM_Response,
            HW05_EEPROM_HIGH_SCORE_ADDR,
            &high_score))
    {
        task_console_printf("EEPROM high score read failed, defaulting to 00\n\r");
        return 0;
    }

    if (high_score == 0xFF)
    {
        return 0;
    }

    return high_score;
}

/**
 * @brief Reset the high score stored in EEPROM to 0.
 */
static void hw05_reset_high_score(void)
{
    uint8_t high_score = 0;

    if (!system_sensors_eeprom_write(
            Queue_EEPROM_Response,
            HW05_EEPROM_HIGH_SCORE_ADDR,
            high_score))
    {
        task_console_printf("EEPROM high score reset failed\n\r");
    }
}

/**
 * @brief Handle SW3 high-score reset while still in the pre-game setup phase.
 */
static void hw05_handle_pre_game_reset(
    uint8_t *high_score,
    uint8_t exact_matches,
    uint8_t misplaced_matches)
{
    hw05_reset_high_score();
    *high_score = hw05_read_high_score();
    hw05_draw_status_header(*high_score, exact_matches, misplaced_matches);
    task_console_printf("SW3 pressed: high score reset to %02u\n\r", *high_score);
}

/**
 * @brief Draw the cypher-entry screen using the shared LCD layout.
 */
static void hw05_draw_cypher_entry_screen(uint8_t high_score)
{
    static const uint8_t cypher_defaults[4] = {0, 0, 0, 0};
    static const uint8_t keypad_top[4] = {0, 1, 2, 3};
    static const uint8_t keypad_bottom[4] = {4, 5, 6, 7};
    if(darkMode) {
        lcd_clear_screen(LCD_COLOR_BLACK);
        hw05_draw_status_header(high_score, 0, 0);
        hw05_draw_tile_row(LCD_TILE_ROW_CYPHER, cypher_defaults, LCD_COLOR_RED, LCD_COLOR_BLACK, 0);
        hw05_draw_tile_row(LCD_TILE_ROW_NUM_0_3, keypad_top, LCD_COLOR_GREEN, LCD_COLOR_BLACK, -1);
        hw05_draw_tile_row(LCD_TILE_ROW_NUM_4_7, keypad_bottom, LCD_COLOR_GREEN, LCD_COLOR_BLACK, -1);
    } else {
        lcd_clear_screen(LCD_COLOR_WHITE);
        hw05_draw_status_header(high_score, 0, 0);
        hw05_draw_tile_row(LCD_TILE_ROW_CYPHER, cypher_defaults, LCD_COLOR_RED, LCD_COLOR_WHITE, 0);
        hw05_draw_tile_row(LCD_TILE_ROW_NUM_0_3, keypad_top, LCD_COLOR_GREEN, LCD_COLOR_WHITE, -1);
        hw05_draw_tile_row(LCD_TILE_ROW_NUM_4_7, keypad_bottom, LCD_COLOR_GREEN, LCD_COLOR_WHITE, -1);
    }
    
}

/**
 * @brief Draw the lower two gameplay rows while waiting for the opponent.
 *
 * Row 1 is intentionally left untouched so the active guess row can remain
 * visible once ENTER_GUESS is implemented.
 */
static void hw05_draw_wait_for_guess_screen(void)
{
    lcd_msg_t msg;
    uint16_t bg_color = darkMode ? LCD_COLOR_BLACK : LCD_COLOR_WHITE;

    lcd_draw_rectangle(
        LCD_W / 2,
        lcd_tile_center_y(LCD_TILE_ROW_NUM_0_3),
        LCD_W,
        TILE_H,
        bg_color,
        true);

    lcd_draw_rectangle(
        LCD_W / 2,
        lcd_tile_center_y(LCD_TILE_ROW_NUM_4_7),
        LCD_W,
        TILE_H,
        bg_color,
        true);

    msg.command = LCD_CMD_PRINT_MESSAGE;
    snprintf(msg.payload.message, sizeof(msg.payload.message), "Waiting for");
    lcd_print_message(&msg, 54, lcd_tile_top_y(LCD_TILE_ROW_NUM_0_3) + 12);

    snprintf(msg.payload.message, sizeof(msg.payload.message), "opponent guess...");
    lcd_print_message(&msg, 18, lcd_tile_top_y(LCD_TILE_ROW_NUM_4_7) + 12);
}

/**
 * @brief Sends discovery requests until either a request or an ACK
 * is received,. when complete, the player who sucessfully sent a 
 * discovery (received an ACK) will be designated as player 1, and 
 * the player who received the discovery (sent an ACK) will be 
 * player 2
 * 
 * @param sequence_num used to track the number of attempts at connecting
 * @param player1 will be set to true if this board is player 1, false if player 2
 * @return true if discovery was sucessful
 * @return false if discovery fails
 */
static bool discover_board(uint16_t *sequence_num, bool *player1)
{
    bool discovery_complete = false;

    while (!discovery_complete)
    {
        // Send a discovery packet
        ipc_send_discovery(*sequence_num);

        // Wait for ACK or DISCOVERY from the other board
        EventBits_t events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_EVENT_IPC_ACK_RECEIVED | ECE353_EVENT_IPC_DISCOVERY_RECEIVED,
            pdTRUE,     // clear bits
            pdFALSE,    // wait for ANY
            pdMS_TO_TICKS(500)
        );

        if (events & ECE353_EVENT_IPC_ACK_RECEIVED)
        {
            // this board initiated discovery and got ACKed - this board is Player 1
            task_console_printf("Discovery successfully sent\n\r");
            *player1 = true;
            discovery_complete = true;
        }
        else if (events & ECE353_EVENT_IPC_DISCOVERY_RECEIVED)
        {
            // Other board initiated discovery - this board is Player 2
            *player1 = false;
            task_console_printf("Discovery Received\n\r");
            // Send ACK back
            ipc_send_ack(*sequence_num);

            discovery_complete = true;
        }
        else
        {
            // No response - retry with next sequence number
            if (*sequence_num%4 == 0) { //print every ~2s if board not found
                task_console_printf("Opponent board not found, trying again.\n\r");
            }
            (*sequence_num)++;
        }
    }

    return true;
}

/**
 * @brief This helper function will take an x and y coordinate 
 * and convert it to the relevant tile selected. Note that 
 * this method will return -1 (no tile selected) when selecting
 * the cypher row (which is not directly interactable with 
 * touchscreen) or when there is no board date. This method
 * does not read the cap_touch sensor, but does expect data from it.
 * 
 * @param x the x coordinate on the screen
 * @param y the y coordinate on the screen
 * @return int the number associated with the selected tile.
 */
int coords_to_tile(uint16_t x, uint16_t y)
{
    lcd_rect_t r;

    // Check row NUM_0_3 (tiles 0–3)
    for (int col = 0; col < 4; col++)
    {
        lcd_tile_rect(&r, LCD_TILE_ROW_NUM_0_3, col);

        int left   = r.cx - r.w/2;
        int right  = r.cx + r.w/2;
        int top    = r.cy - r.h/2;
        int bottom = r.cy + r.h/2;

        if (x >= left && x <= right && y >= top && y <= bottom)
        {
            return col;   // tile 0–3
        }
    }

    // Check row NUM_4_7 (tiles 4–7)
    for (int col = 0; col < 4; col++)
    {
        lcd_tile_rect(&r, LCD_TILE_ROW_NUM_4_7, col);

        int left   = r.cx - r.w/2;
        int right  = r.cx + r.w/2;
        int top    = r.cy - r.h/2;
        int bottom = r.cy + r.h/2;

        if (x >= left && x <= right && y >= top && y <= bottom)
        {
            return col + 4;   // tile 4–7
        }
    }

    return -1; // No tile touched
}


/**
 * @brief helper function to move current selection tile
 * Assumes the switch is always legal (and correct) 
 * and that inputs are available tiles.
 * 
 * @param currTile The current selected tile that will be deselected
 * @param nextTile The next tile to be selected
 */
void switchSelectTiles(int currTile, int nextTile) {
    //message used to draw the tiles
    lcd_msg_t msg;
    //find row and col for each tile
    //current tile (being deselected)
    lcd_row_t old_row = LCD_TILE_ROW_NUM_0_3; 
    if (currTile > 3) {
        old_row = LCD_TILE_ROW_NUM_4_7;
    }
    int old_col = currTile % 4;

    //next tile (being selected)
    lcd_row_t new_row = LCD_TILE_ROW_NUM_0_3; 
    if (nextTile > 3) {
        new_row = LCD_TILE_ROW_NUM_4_7;
    }
    int new_col = nextTile % 4;

    //draw changed tiles
    //current tile (deselecting)
    if(currTile >= 0) { //do nothing if no tile currently selected
        msg.command = LCD_CMD_DRAW_TILE;
        msg.payload.tile.row = old_row;
        msg.payload.tile.col = old_col;
        msg.payload.tile.number = currTile;
        msg.payload.tile.color_fg = LCD_COLOR_GREEN;
        if (darkMode) {
            msg.payload.tile.color_bg = LCD_COLOR_BLACK;
        } else {
            msg.payload.tile.color_bg = LCD_COLOR_WHITE;
        }
        
        master_mind_handle_msg(&msg);
    }

    //next tile (selecting)
    if(nextTile >= 0) { //if next tile is off screen, do nothing
        msg.command = LCD_CMD_DRAW_TILE_INVERTED;
        msg.payload.tile.row = new_row;
        msg.payload.tile.col = new_col;
        msg.payload.tile.number = nextTile;
        msg.payload.tile.color_fg = LCD_COLOR_GREEN;
        if (darkMode) {
            msg.payload.tile.color_bg = LCD_COLOR_BLACK;
        } else {
            msg.payload.tile.color_bg = LCD_COLOR_WHITE;
        }
        master_mind_handle_msg(&msg);
    }
    
}

/**
 * @brief Helper function to add the selected tile
 * to the cypher display.
 * 
 * @param currSelectTile The tile with the number to be added
 * @param currCypherTile The current position in the cypher
 */
void addToCypher(int currSelectTile, int currCypherTile) {
    //redraw the current cypher tile with the new number
    //and without the inverted color
    lcd_msg_t msg;
    msg.command = LCD_CMD_DRAW_TILE;
    msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
    msg.payload.tile.col = currCypherTile;          //by our convention, our cypher tile number is its column
    msg.payload.tile.number = currSelectTile;       //the selected number
    msg.payload.tile.color_fg = LCD_COLOR_RED;
    if(darkMode) {
        msg.payload.tile.color_bg = LCD_COLOR_BLACK;
    } else {
        msg.payload.tile.color_bg = LCD_COLOR_WHITE;
    }
    
    master_mind_handle_msg(&msg);

    //select the next cypher tile only if we are not at the end
    if(currCypherTile != 4) {
        msg.command = LCD_CMD_DRAW_TILE_INVERTED;
        msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
        msg.payload.tile.col = currCypherTile + 1; //next cypher tile
        msg.payload.tile.number = 0;               //default
        msg.payload.tile.color_fg = LCD_COLOR_RED;
        if(darkMode) {
            msg.payload.tile.color_bg = LCD_COLOR_BLACK;
        } else {
            msg.payload.tile.color_bg = LCD_COLOR_WHITE;
        }
    
        master_mind_handle_msg(&msg);
    }
    
}


/**
 * @brief Lets the user select their cypher using the touchscreen
 * Stores their cypher for future use
 * 
 * @param cypher_out an array used to store the selected cypher
 */
void select_cypher(uint8_t cypher_out[4])
{
    lcd_msg_t msg;
    uint8_t high_score = hw05_read_high_score();

    /************************************************************
     * 1. Draw UI: message + cypher tiles + number tiles
     ************************************************************/
    hw05_draw_cypher_entry_screen(high_score);
    task_console_printf("Select your cypher with the touchscreen, \n\r");
    task_console_printf("press SW1 to confirm each digit,\n\r");
    task_console_printf("use SW3 to reset the high score before gameplay starts.\n\r");

    /************************************************************
     * 2. cypher entry loop
     ************************************************************/
    int currcypherTile = 0;   // 0–3
    int currSelectTile = -1;  // No tile selected yet
    int touched_tile = -1;

    while (currcypherTile < 4)
    {
        //check whether we need to change dark/light mode
        //if so, complete redraw is required
        if (update_dark_mode())
        {
            // Mode changed → redraw the entire cypher entry UI
            hw05_draw_cypher_entry_screen(high_score);

            // Redraw cypher progress so far
            for (int i = 0; i < currcypherTile; i++)
            {
                addToCypher(cypher_out[i], i);
            }

            // Redraw current highlight if applicable
            if (currSelectTile >= 0)
            {
                switchSelectTiles(-1, currSelectTile);
            }
        }
        /********************************************************
         * A. Check for cap‑touch input
         ********************************************************/
        uint16_t x, y;
        bool touched = system_sensors_get_cap_touch(Queue_Cap_Touch_Response, &x, &y);

        if (touched)
        {
            touched_tile = coords_to_tile(x, y);

            if (touched_tile >= 0 && touched_tile <= 7)
            {
                // If this is the first selection or nothing is currently selected,
                // no need to deselect anything
                if (currSelectTile == -1)
                {
                    // Highlight the touched tile
                    switchSelectTiles(-1, touched_tile);
                    currSelectTile = touched_tile;
                }
                else if (touched_tile != currSelectTile)
                {
                    // Switch highlight from old tile to new tile
                    switchSelectTiles(currSelectTile, touched_tile);
                    currSelectTile = touched_tile;
                }
            }
        }

        /********************************************************
         * B. Check for SW1 press to confirm selection and move to next tile, or SW3 press to reset high score
         ********************************************************/
        EventBits_t events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_EVENT_SW1_PRESSED | ECE353_EVENT_SW3_PRESSED,
            pdTRUE,     // clear bit
            pdFALSE,    // wait for ANY
            pdMS_TO_TICKS(20)
        );

        if (events & ECE353_EVENT_SW3_PRESSED)
        {
            hw05_handle_pre_game_reset(&high_score, 0, 0);
        }

        //if SW1 is pressed, save this digit and move to next, or exit if done
        if (events & ECE353_EVENT_SW1_PRESSED)
        {
            if (currSelectTile >= 0)
            {
                // Store selected number
                cypher_out[currcypherTile] = currSelectTile;

                // Update LCD
                addToCypher(currSelectTile, currcypherTile);

                currcypherTile++;

                // Highlight next cypher tile (if any)
                if (currcypherTile < 4)
                {
                    msg.command = LCD_CMD_DRAW_TILE_INVERTED;
                    msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                    msg.payload.tile.col = currcypherTile;
                    msg.payload.tile.number = 0;
                    msg.payload.tile.color_fg = LCD_COLOR_RED;
                    if (darkMode) {
                        msg.payload.tile.color_bg = LCD_COLOR_BLACK;
                    } else {
                        msg.payload.tile.color_bg = LCD_COLOR_WHITE;
                    }
                    

                    master_mind_handle_msg(&msg);
                }
            }
        }
    }

    /************************************************************
     * 3. cypher entry complete
     ************************************************************/
    task_console_printf("cypher entry complete: %d %d %d %d\n\r",
                        cypher_out[0], cypher_out[1],
                        cypher_out[2], cypher_out[3]);

    /* Redraw the header so the screen is not left in setup mode text. */
    hw05_draw_status_header(high_score, 0, 0);
}

/**
 * @brief Sends the one-time initialization status indicating this board is ready.
 * 
 * This is part of the startup handshake after both boards finish selecting
 * their cyphers. It is not meant to be sent after every gameplay action.
 * 
 * @param sequence_num The next IPC sequence number to use.
 */
static void send_ready_status(uint16_t *sequence_num)
{
    bool acked = false;

    while (!acked)
    {
        xEventGroupClearBits(ECE353_RTOS_Events, ECE353_EVENT_IPC_ACK_RECEIVED);

        if (!ipc_send_status(*sequence_num, IPC_STATUS_OK))
        {
            task_console_printf("Failed to queue ready status (seq=%u)\n\r", *sequence_num);
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        acked = ipc_wait_for_ack(HW05_READY_STATUS_ACK_TIMEOUT_MS);

        if (!acked)
        {
            task_console_printf("Ready status ACK timeout (seq=%u), retrying\n\r", *sequence_num);
            (*sequence_num)++;
        }
    }

    task_console_printf("Ready status ACKed (seq=%u)\n\r", *sequence_num);
    (*sequence_num)++;
}

/**
 * @brief waits for the other player to send a ready status
 */
static void wait_for_other_player_ready(uint8_t *high_score)
{
    task_console_printf("Waiting for other player...\n\r");
    task_console_printf("SW3 can still reset the high score until gameplay begins.\n\r");

    while (1)
    {
        EventBits_t events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_EVENT_IPC_STATUS_RECEIVED | ECE353_EVENT_SW3_PRESSED,
            pdTRUE,     // clear on exit
            pdFALSE,    // wait for ANY
            pdMS_TO_TICKS(50)
        );

        if (events & ECE353_EVENT_SW3_PRESSED)
        {
            hw05_handle_pre_game_reset(high_score, 0, 0);
        }

        if (events & ECE353_EVENT_IPC_STATUS_RECEIVED)
        {
            break;
        }
    }

    task_console_printf("Other player is ready!\n\r");
}

static bool update_dark_mode(void)
{
    uint16_t ambient = 0;

    if (!system_sensors_get_light(Queue_Cap_Touch_Response, &ambient)) {
        return false;
    }

    bool oldMode = darkMode;

    //debug print statement
    //task_console_printf("Ambient light reading (CH0) = %u\n\r", ambient);


    if (darkMode)
    {
        // Currently dark - switch to light only if bright enough
        if (ambient > 350)
        {
            darkMode = false;
            task_console_printf("Switching to LIGHT mode (ambient=%u)\n\r", ambient);

            lcd_msg_t msg = { .command = LCD_CMD_CLEAR_SCREEN };
            master_mind_handle_msg(&msg);
        }
    }
    else
    {
        // Currently light - switch to dark only if dim enough
        if (ambient < 180)
        {
            darkMode = true;
            task_console_printf("Switching to DARK mode (ambient=%u)\n\r", ambient);

            lcd_msg_t msg = { .command = LCD_CMD_CLEAR_SCREEN };
            master_mind_handle_msg(&msg);
        }
    }
    return oldMode != darkMode;
}





/*************************************************
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 ************************************************/
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    // Set text color to black
    //printf("\x1b[30m"); //commented out since console has a black background
    printf("\x1b[37m"); //makes text white
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    //init LCD
    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Initialize the joystick */
    joystick_init();

    /* Initialize the buttons, buttons task init does not do this*/
    buttons_init_gpio();

    /* Initialize the I2C interface */
    I2C_Monarch_Obj = i2c_init(PIN_I2C_SDA, PIN_I2C_SCL);
    if( I2C_Monarch_Obj == NULL)
    {
        printf("I2C Initialization Failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Initialize SPI Interface */
    SPI_Monarch_Obj = spi_init(PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_CLK);
    if( SPI_Monarch_Obj == NULL)
    {
        printf("SPI Initialization Failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    //initialize CS pin, REMEMBER TO DO THIS IN THE FUTURE
    //initialize CS pin, REMEMBER TO DO THIS IN THE FUTURE
    //initialize CS pin, REMEMBER TO DO THIS IN THE FUTURE
    //initialize CS pin, REMEMBER TO DO THIS IN THE FUTURE
    cyhal_gpio_init(PIN_EEPROM_CS, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true);

}

/**
* primary control task for hw05
*/
void task_hw05_system_control(void *pvParameters)
{
    uint16_t sequence_num = 0;
    bool player1 = false;
    darkMode = true; //default to dark mode
    hw05_game_state_t state = HW05_STATE_INIT;
    // increments after every guess from either player
        //even turns - player 1 guesses
        //odd turns - player 2 guesses
    int turn_number = 0;   

    //tracks the accuracy of this board's last guess
    uint8_t last_exact = 0;
    uint8_t last_misplaced = 0;
    uint8_t opponent_guess[4] = {0};

    uint8_t high_score = 0;

    //Establish communication and assign roles
    discover_board(&sequence_num, &player1);

    if (player1)
    {
        task_console_printf("I am Player 1\n\r");
    }
    else
    {
        task_console_printf("I am Player 2\n\r");
    }

    // 2. Cypher selection
    //Allocate storage for this player's cypher
    
    uint8_t my_cypher[4] = {0};

    //Let the user select their cypher
    select_cypher(my_cypher);

    //Debug print
    //task_console_printf("My cypher: %d %d %d %d\n\r", my_cypher[0], my_cypher[1], my_cypher[2], my_cypher[3]);

    //3. Wait for both players to be ready and begin game
    //Notify the other board that we are ready
    send_ready_status(&sequence_num);

    //Wait for the other board's ready message
    high_score = hw05_read_high_score();
    wait_for_other_player_ready(&high_score);

    task_console_printf("Both players ready — starting game!\n\r");
    task_console_printf("Player 1 guesses first.\n\r");

    hw05_draw_status_header(high_score, 0, 0);

    //enter core loop
    while (true)
    {
        if (update_dark_mode())
        {
            // TODO: redraw current screen based on state
            //may not be necessary as most helper methods that 
            //take control for a period of time will have to implement
            //this
        }

        bool my_turn =
            (player1 && (turn_number % 2 == 0)) ||
            (!player1 && (turn_number % 2 == 1));

        switch (state)
        {
            case HW05_STATE_INIT:
                /*
                * Conditions:
                * - Player 1 always starts
                * - If it's my turn, go to ENTER_GUESS
                * - Otherwise, wait for opponent's guess
                */
                state = my_turn ? HW05_STATE_ENTER_GUESS
                                : HW05_STATE_WAIT_FOR_GUESS;
                break;


            case HW05_STATE_ENTER_GUESS:
                /*
                * TODO:
                * - Draw guess entry UI
                * - Let user enter 4-digit guess
                * - When confirmed:
                *      -> send guess via IPC
                *      -> state = HW05_STATE_WAIT_FOR_FEEDBACK
                */
                break;


            case HW05_STATE_WAIT_FOR_GUESS:
                hw05_draw_status_header(high_score, last_exact, last_misplaced);
                hw05_draw_wait_for_guess_screen();

                // Wait for opponent's guess via IPC
                while (!ipc_wait_for_guess(100, opponent_guess))
                {
                    if (update_dark_mode())
                    {
                        hw05_draw_status_header(high_score, last_exact, last_misplaced);
                        hw05_draw_wait_for_guess_screen();
                    }
                }

                task_console_printf(
                    "Opponent guess received: %u %u %u %u\n\r",
                    opponent_guess[0],
                    opponent_guess[1],
                    opponent_guess[2],
                    opponent_guess[3]);

                state = HW05_STATE_EVAL_GUESS;
                break;


            case HW05_STATE_EVAL_GUESS:
                /*
                * TODO:
                * - Compare opponent's guess to my cypher
                * - Compute exact & misplaced
                * - Save into last_exact, last_misplaced
                * - state = HW05_STATE_SEND_FEEDBACK
                */
                break;


            case HW05_STATE_SEND_FEEDBACK:
                /*
                * TODO:
                * - Send feedback to opponent via IPC
                * - state = HW05_STATE_CHECK_WIN
                */
                break;


            case HW05_STATE_WAIT_FOR_FEEDBACK:
                /*
                * TODO:
                * - Wait for IPC feedback
                * - When received:
                *      -> update last_exact, last_misplaced
                *      -> state = HW05_STATE_CHECK_WIN
                */
                break;


            case HW05_STATE_CHECK_WIN:
                /*
                * TODO:
                * - If last_exact == 4:
                *      -> state = HW05_STATE_GAME_OVER
                * - Else:
                *      -> turn_number++
                *      -> state = (my_turn ? WAIT_FOR_GUESS : ENTER_GUESS)
                */
                break;


            case HW05_STATE_GAME_OVER:
                /*
                * TODO:
                * - Display win/lose
                * - Update high score if needed
                * - Wait for SW1 to restart
                * - Reset turn_number = 0
                * - state = HW05_STATE_INIT
                */
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }


}


/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{
    bool rslt; 

    // Initialize the EventGroup
    ECE353_RTOS_Events = xEventGroupCreate();

    /* Initialize the semaphores for I2C and SPI */
    hw05_semaphores_init();

    /* Initialize the queues for communication */
    hw05_queues_init();

    //init eeprom
    rslt = task_eeprom_resources_init(
        &SPI_Semaphore,
        SPI_Monarch_Obj,
        PIN_EEPROM_CS
    );
    if (!rslt)
    {
        printf("EEPROM Task initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }


    //init buttons
    if(!task_button_init())
    {
        printf("Button initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    //init ipc
    if(!task_ipc_init())
    {
        printf("IPC initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }



    /* Initialize LCD task resources */
    if (!task_lcd_resources_init(Queue_LCD)) {
        printf("LCD Task initialization failed!\n\r");
        for (int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }


    /* Create other FreeRTOS tasks */
    BaseType_t status;

    /* Joystick task */
    status = xTaskCreate(task_joystick,
        "JOYSTICK",
        configMINIMAL_STACK_SIZE,
        NULL,
        2,
        NULL);
    if (status != pdPASS)
    {
        printf("Failed to initialize joystick task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    //init cap_touch
    rslt = task_cap_touch_resources_init(
        Queue_Request_Cap_Touch,
        I2C_Semaphore,
        I2C_Monarch_Obj,
        PIN_CAP_TOUCH_INT
    );

    if (!rslt)
    {
        printf("Cap Touch Task initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    //init light sensor
    if (!task_light_sensor_resources_init(&I2C_Semaphore, I2C_Monarch_Obj))
    {
        printf("Light Sensor Task initialization failed!\n\r");
        CY_ASSERT(0);
    }



    //Init console
    rslt = task_console_init();
    if (!rslt)
    {
        printf("Console Task resource initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
    
    //init control task
     status = xTaskCreate(task_hw05_system_control,
        "HW05_CTRL",
        TASK_SYSTEM_CONTROL_STACK_SIZE,
        NULL,
        TASK_SYSTEM_CONTROL_PRIORITY,
        NULL);
    if (status != pdPASS)
    {
        printf("Failed to initialize system control task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    //Note cap_touch and LEDs are not initialized, if needed, add them
    
    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif
