 /**
 * @file hw02.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "hw02.h"
#include "master_mind_lib.h"

#if defined(HW02)

char APP_DESCRIPTION[] = "ECE353 S26 HW02";

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
QueueHandle_t Queue_Requests_Joystick = NULL;
QueueHandle_t Queue_LCD = NULL;
EventGroupHandle_t ECE353_RTOS_Events = NULL;

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 * @brief helper function to move current selection tile
 * Assumes the switch is always legal and that inputs are
 * available tiles
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
    msg.command = LCD_CMD_DRAW_TILE;
    msg.payload.tile.row = old_row;
    msg.payload.tile.col = old_col;
    msg.payload.tile.number = currTile;
    msg.payload.tile.color_fg = LCD_COLOR_GREEN;
    msg.payload.tile.color_bg = LCD_COLOR_BLACK;
    master_mind_handle_msg(&msg);

    //next tile (selecting)
    msg.command = LCD_CMD_DRAW_TILE_INVERTED;
    msg.payload.tile.row = new_row;
    msg.payload.tile.col = new_col;
    msg.payload.tile.number = nextTile;
    msg.payload.tile.color_fg = LCD_COLOR_GREEN;
    msg.payload.tile.color_bg = LCD_COLOR_BLACK;
    master_mind_handle_msg(&msg);
}

void task_hw02_system_control(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

   
    /* Draw the initial master mind game board */
    /* Allocate a lcd_msg_t variable */
    lcd_msg_t msg;

    /* Write a message to the user in the Text Area of the screen*/
    msg.command = LCD_CMD_PRINT_MESSAGE;
    snprintf(msg.payload.message, 32, "Select Your Cypher!");
    master_mind_handle_msg(&msg);


    /* Draw 4 blank tiles for the secret code */
    for(int col = 0; col < 4; col++)
    {
        if (col == 0) { //first tile will be inverted to indicate it is selected
            msg.command = LCD_CMD_DRAW_TILE_INVERTED;
        } else {
            msg.command = LCD_CMD_DRAW_TILE;
        }
        msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
        msg.payload.tile.col = col;
        msg.payload.tile.number = 0; // number is ignored for code tiles
        msg.payload.tile.color_fg = LCD_COLOR_ORANGE;
        msg.payload.tile.color_bg = LCD_COLOR_BLACK;
        master_mind_handle_msg(&msg);
    }


    /* Draw numbers 0-3 for the user input*/
    for(int col = 0; col < 4; col++)
    {
        if (col == 0) { //first tile will be inverted to indicate it is selected
            msg.command = LCD_CMD_DRAW_TILE_INVERTED;
        } else {
            msg.command = LCD_CMD_DRAW_TILE;
        }
        msg.payload.tile.row = LCD_TILE_ROW_NUM_0_3;
        msg.payload.tile.col = col;
        msg.payload.tile.number = col;
        msg.payload.tile.color_fg = LCD_COLOR_GREEN;
        msg.payload.tile.color_bg = LCD_COLOR_BLACK;

        master_mind_handle_msg(&msg);
    }

    /* Draw the numbers for 4-7 for the user input*/
    for(int col = 0; col < 4; col++)
    {
        msg.command = LCD_CMD_DRAW_TILE;
        msg.payload.tile.row = LCD_TILE_ROW_NUM_4_7;
        msg.payload.tile.col = col;
        msg.payload.tile.number = col + 4;
        msg.payload.tile.color_fg = LCD_COLOR_GREEN;
        msg.payload.tile.color_bg = LCD_COLOR_BLACK;
        master_mind_handle_msg(&msg);
    }

    /* Allow the user to select 4 numbers for their cypher */
    int currCyhperTile = 0; //0-3
    int nextCypherTile = 0;
    int currSelectTile = 0; //0-7
    int nextSelectTile = 0;
    while(1)
    {
        //wait for some event bit to trigger
        EventBits_t events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_EVENT_SW1_PRESSED | ECE353_EVENT_JOYSTICK_DOWN | ECE353_EVENT_JOYSTICK_LEFT 
            | ECE353_EVENT_JOYSTICK_UP | ECE353_EVENT_JOYSTICK_RIGHT,
            pdTRUE,     // clear bits on exit
            pdFALSE,    // wait for ANY
            portMAX_DELAY
        );


        //Respond to joystick
        if(events & ECE353_EVENT_JOYSTICK_LEFT) {
            if ((currSelectTile != 0) && (currSelectTile != 4)) {
                //move left if able
                nextSelectTile = currSelectTile-1;
                //switch the tiles
                switchSelectTiles(currSelectTile, nextSelectTile);
                currSelectTile = nextSelectTile;
            }
        } else if (events & ECE353_EVENT_JOYSTICK_UP) {
            if ((currSelectTile > 3)) {
                //move up if able
                nextSelectTile = currSelectTile-4;
                //switch the tiles
                switchSelectTiles(currSelectTile, nextSelectTile);
                currSelectTile = nextSelectTile;
            }
        } else if (events & ECE353_EVENT_JOYSTICK_RIGHT) {
            if((currSelectTile != 3) && (currSelectTile != 7)) {
                //move right if able
                nextSelectTile = currSelectTile+1;
                //switch the tiles
                switchSelectTiles(currSelectTile, nextSelectTile);
                currSelectTile = nextSelectTile;
            }
        } else if (events & ECE353_EVENT_JOYSTICK_DOWN) {
            if(currSelectTile < 4) {
                //move down if able
                nextSelectTile = currSelectTile+4;
                //switch the tiles
                switchSelectTiles(currSelectTile, nextSelectTile);
                currSelectTile = nextSelectTile;
            }
        }

        //TODO IMPLEMENT SELECTION
        //TODO once cypher is made, lock input
        //TODO TODO TODO TODO TODO TODO
        //TODO TODO TODO TODO TODO TODO
        //TODO TODO TODO TODO TODO TODO
        //TODO TODO TODO TODO TODO TODO
        //TODO TODO TODO TODO TODO TODO
        //TODO TODO TODO TODO TODO TODO

    }
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
    printf("\x1b[30m");
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Initialize the joystick */
    joystick_init();

    /* Initialize the buttons*/
    buttons_init_gpio();

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
    /* Create the event group for RTOS events */
    ECE353_RTOS_Events = xEventGroupCreate();
    if (ECE353_RTOS_Events == NULL) {
        printf("Failed to create event group\n\r");
        while (1); // Loop forever if event group creation fails
    }

    /* Create the FreeRTOS queues */
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


    /* Create the FreeRTOS tasks */
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

    /* Buttons task */
    status = xTaskCreate(task_buttons,
        "BUTTONS",
        configMINIMAL_STACK_SIZE,
        NULL,
        2,
        NULL);
    if (status != pdPASS)
    {
        printf("Failed to initialize button task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* LCD task */ 
    if (!task_lcd_resources_init(Queue_LCD))
    {
        printf("Failed to initialize LCD task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* System control task */
    status = xTaskCreate(task_hw02_system_control,
        "HW02_CTRL",
        configMINIMAL_STACK_SIZE + 300,
        NULL,
        3,
        NULL);
    if (status != pdPASS)
    {
        printf("Failed to initialize system control task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif