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

#if defined(HW02)

char APP_DESCRIPTION[] = "ECE353 S26 HW02";

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
EventGroupHandle_t ECE353_RTOS_Events = NULL;

QueueHandle_t xQueue_Request_LCD = NULL;
QueueHandle_t xQueue_Response_LCD = NULL;

QueueHandle_t xQueue_Request_Joystick = NULL;
QueueHandle_t xQueue_Response_SystemControl = NULL; // for response from joystick task


/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
typedef struct
{
    /* Keeps track of which input tile is currently highlighted, which cypher
       tile we are currently filling in, and a few flags for HW02 behavior. */
    uint8_t active_input;
    uint8_t active_cypher;
    uint8_t cypher_values[4];
    bool accepting_input; // that the system control is still accepting input from the joystick to fill in the cypher tiles
    bool joystick_centered;
} hw02_state_t;

#define HW02_INPUT_TILE_COUNT      (8)
#define HW02_CYPHER_TILE_COUNT     (4)

/*
    Helper function that wraps the LCD request/response queue exchange.

    We use this so task_hw02_system_control() does not need to repeatedly build
    a lcd_msg_request_t by hand every single time it wants to redraw a tile.

    It also gives us one place to check if the queues were actually created and
    whether the LCD gatekeeper said the draw succeeded.
*/
static bool hw02_send_lcd_msg(const lcd_msg_t *msg)
{
    lcd_msg_request_t request;
    lcd_msg_response_t response;

    if ((msg == NULL) || (xQueue_Request_LCD == NULL) || (xQueue_Response_LCD == NULL))
    {
        return false;
    }

    request.msg = *msg;
    request.return_queue = xQueue_Response_LCD;

    if (xQueueSend(xQueue_Request_LCD, &request, portMAX_DELAY) != pdPASS)
    {
        return false;
    }

    if (xQueueReceive(xQueue_Response_LCD, &response, portMAX_DELAY) != pdPASS)
    {
        return false;
    }

    return response == LCD_MSG_RESPONSE_SUCCESS;
}

/*
    Helper function for drawing one of the 4 cypher tiles.

    The "active" cypher tile is the one with black text on a red background.
    All inactive cypher tiles are drawn as red text on a black background.
*/
static bool hw02_draw_cypher_tile(uint8_t col, uint8_t number, bool active)
{
    lcd_msg_t msg;

    if (col >= HW02_CYPHER_TILE_COUNT) // i.e. we've drawn the 4 cypher tiles already
    {
        return false;
    }

    msg.command = LCD_CMD_DRAW_TILE;
    msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
    msg.payload.tile.col = col;
    msg.payload.tile.number = number;
    msg.payload.tile.color_fg = active ? LCD_COLOR_BLACK : LCD_COLOR_RED;
    msg.payload.tile.color_bg = active ? LCD_COLOR_RED : LCD_COLOR_BLACK;

    return hw02_send_lcd_msg(&msg);
}

/*
    Helper function for drawing one of the 8 selectable input tiles.

    The "active" input tile is the one with black text on a green background.
    All inactive input tiles are drawn as green text on a black background.
*/
static bool hw02_draw_input_tile(uint8_t input_index, bool active)
{
    lcd_msg_t msg;

    if (input_index >= HW02_INPUT_TILE_COUNT)
    {
        return false;
    }

    msg.command = LCD_CMD_DRAW_TILE;
    msg.payload.tile.row = (input_index < 4) ? LCD_TILE_ROW_NUM_0_3 : LCD_TILE_ROW_NUM_4_7;
    msg.payload.tile.col = input_index % 4;
    msg.payload.tile.number = input_index;
    msg.payload.tile.color_fg = active ? LCD_COLOR_BLACK : LCD_COLOR_GREEN;
    msg.payload.tile.color_bg = active ? LCD_COLOR_GREEN : LCD_COLOR_BLACK;

    return hw02_send_lcd_msg(&msg);
}

/*
    Helper function for figuring out which input tile should become active next.

    If the joystick movement would push us off the 2x4 input board, we return
    false so the caller knows to leave the active tile where it already is.
*/
static bool hw02_get_next_input(uint8_t current_input, joystick_position_t position, uint8_t *next_input)
{
    uint8_t row;
    uint8_t col;

    if ((next_input == NULL) || (current_input >= HW02_INPUT_TILE_COUNT))
    {
        return false;
    }

    row = current_input / 4;
    col = current_input % 4;

    switch (position)
    {
        case JOYSTICK_POS_LEFT:
            if (col == 0)
            {
                return false;
            }
            col--; // otherwise, we're good to make the move
            break;

        case JOYSTICK_POS_RIGHT:
            if (col >= 3)
            {
                return false;
            }
            col++;
            break;

        case JOYSTICK_POS_UP:
            if (row == 0)
            {
                return false;
            }
            row--;
            break;

        case JOYSTICK_POS_DOWN:
            if (row >= 1)
            {
                return false;
            }
            row++;
            break;

        default:
            return false; // we don't recognize the in-between positions, so we treat them as no movement
    }

    *next_input = (row * 4) + col; // give back the index of the new active tile
    return true;
}

void task_hw02_system_control(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    EventBits_t events;

    // the startup config
    hw02_state_t state = {
        .active_input = 0,
        .active_cypher = 0,
        .cypher_values = {0, 0, 0, 0},
        .accepting_input = true,
        .joystick_centered = true
    };

    // for sending requests and receiving responses from the joystick task;
    // we don't have these for the lcd because they are in the helper function
    device_request_msg_t joystick_request;
    device_response_msg_t joystick_response;
    joystick_position_t joystick_position;

    // index for the next input tile to become active when the user moves the joystick
    uint8_t next_input;
    
    /* 1. DRAW THE INITIAL MASTER MIND GAME BOARD 
        This is basically from hw01.c's app_main(). It's just that
        now instead of directly calling master_mind_handle_msg() to 
        manipulate the LCD, we send request messages to the gatekeeper LCD task to manipulate the LCD

        For this reason I created a wrapper function that builds our request message for
        the LCD task and also does some checkings so it's safer.
    */

    /* Allocate a lcd_msg_t variable */
    lcd_msg_t msg;

    msg.command = LCD_CMD_CLEAR_SCREEN;
    hw02_send_lcd_msg(&msg);

    /* Write the message to the user in the Text Area of the screen*/
    msg.command = LCD_CMD_PRINT_MESSAGE;
    snprintf(msg.payload.message, 32, "Select Your Cypher!");
    hw02_send_lcd_msg(&msg);


    /* Draw 4 blank tiles for the secret code */
    for (int col = 0; col < 4; col++)
    {
        hw02_draw_cypher_tile(col, 0, col == 0);
    }


    /* Draw numbers 0-3 for the user input
        Where entry tile 0 has a black "0" and a green background to indicate it's selected, and the rest have green "numbers" and black background to indicate they're not selected

        (changed msg.command from draw tile inverted in hw01.c to just draw tile now)
    */
    for (int col = 0; col < 4; col++)
    {
        hw02_draw_input_tile(col, col == 0);
    }

    /* Draw the numbers for 4-7 for the user input*/
    for (int col = 4; col < 8; col++)
    {
        hw02_draw_input_tile(col, false);
    }

    /* 2. Allow the user to select 4 numbers for their cypher */

    while (1)
    {
        // the active behavior of the system control task is entirely driven by joystick movements and SW1 presses, so we just wait for those 2 events and react accordingly when we get them
        events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_EVENT_BUTTON_SW1_PRESSED | ECE353_EVENT_JOYSTICK_CHANGED,
            pdTRUE,
            pdFALSE, // wait for any of them
            portMAX_DELAY
        );

        // JOYSTICK TRIGGER - note for either trigger, the premise is that we're not done accepting input for the cypher
        if (((events & ECE353_EVENT_JOYSTICK_CHANGED) != 0) && state.accepting_input)
        {
            /* Ask task_hw02_system_joystick to tell us the current joystick position.
               The diagram shows this as xQueue_Request_Joystick and xQueue_Response_SystemControl. 
            */
            joystick_request.device = DEVICE_JOYSTICK;
            joystick_request.operation = DEVICE_OP_READ; // read the current position
            joystick_request.address = 0; // not used for joystick
            joystick_request.value = 0; // not used for joystick
            joystick_request.response_queue = xQueue_Response_SystemControl;

            // if we have sent the request, received the response, and the read operation was successful, 
            // then we can update our active input tile based on the joystick position we read
            if ((xQueueSend(xQueue_Request_Joystick, &joystick_request, portMAX_DELAY) == pdPASS) &&
                (xQueueReceive(xQueue_Response_SystemControl, &joystick_response, portMAX_DELAY) == pdPASS) &&
                (joystick_response.device == DEVICE_JOYSTICK) &&
                (joystick_response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS))
            {
                /* The joystick task already filtered out diagonals for us (wrote a helper function), 
                    so here we only need to reason about center/left/right/up/down
                */
                joystick_position = joystick_response.payload.joystick;

                if (joystick_position == JOYSTICK_POS_CENTER)
                {
                    // The spec says the joystick must return to center before we accept
                    // another movement so this flag is how we enforce that rule
                    state.joystick_centered = true;
                }

                // note the state's joystick_centered is not the local variable (joystick_position) 
                // we just got from the joystick task. 
                // 
                // So this is asking if the joystick was centered right before this change of position, 
                // which is what allows us to enforce the rule that the joystick must return to center 
                // before we accept another movement (if also not out of bound)
                else if (state.joystick_centered) 
                {
                    // check if out of bound
                    if (hw02_get_next_input(state.active_input, joystick_position, &next_input))
                    {
                        /* only redraw the tiles that actually changed:
                           1. the old active tile becomes inactive
                           2. the new active tile becomes active 
                        */
                        hw02_draw_input_tile(state.active_input, false);
                        state.active_input = next_input;
                        hw02_draw_input_tile(state.active_input, true);
                    }

                    state.joystick_centered = false;
                }

                // otherwise nothing happens; new movements are not accepted until the joystick returns to center
            }
        }


        // SW1 TRIGGER
        if (((events & ECE353_EVENT_BUTTON_SW1_PRESSED) != 0) && state.accepting_input)
        {
            /* 
                copy the selected input value into the current cypher tile, then
                advance the active cypher tile one position to the right
             */
            state.cypher_values[state.active_cypher] = state.active_input;
            hw02_draw_cypher_tile(state.active_cypher, state.active_input, false);

            // increment active_cypher count and advance the selected cypher if we
            // haven't drawn all of them
            state.active_cypher++;
            if (state.active_cypher < HW02_CYPHER_TILE_COUNT)
            {
                hw02_draw_cypher_tile(state.active_cypher, 0, true);
            }
            else
            {
                state.accepting_input = false;
            }
        }
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
    BaseType_t rslt;

    /* 1. Create the event group for RTOS events */
    ECE353_RTOS_Events = xEventGroupCreate();
    if (ECE353_RTOS_Events == NULL) {
        printf("Failed to create event group\n\r");
    }

    /* 2. Create the FreeRTOS queues */
    xQueue_Request_LCD = xQueueCreate(10, sizeof(lcd_msg_request_t));
    if (xQueue_Request_LCD == NULL) {
        printf("Failed to create LCD request queue\n\r");
    }

    xQueue_Response_LCD = xQueueCreate(10, sizeof(lcd_msg_response_t));
    if (xQueue_Response_LCD == NULL) {
        printf("Failed to create LCD response queue\n\r");
    }

    /* 
    HW2 pdf tells us we need to start using the data structures in devices.h which provide more flexibility because from now on, whatever request message we send to devices would be of type device_request_msg_t. 
    
    And we can specify which device we want to talk to in the message, instead of having separate queues for each device. The payload is a union so that checks out.
    */
    xQueue_Request_Joystick = xQueueCreate(10, sizeof(device_request_msg_t)); // so it's sizeof(device_request_msg_t)
    if (xQueue_Request_Joystick == NULL) {
        printf("Failed to create Joystick request queue\n\r");
    }

    xQueue_Response_SystemControl = xQueueCreate(10, sizeof(device_response_msg_t));
    if (xQueue_Response_SystemControl == NULL) {
        printf("Failed to create System Control response queue\n\r");
    }

    /* 3. Create the FreeRTOS tasks */
    // buttons, joystick, and system control all stay at IDLE + 1
    // the LCD gatekeeper is one level higher so a tile redraw can finish cleanly once it starts

    // don't use task_button_init because it also initializes the button pins, which we already did
    rslt = xTaskCreate(
        task_buttons,
        "Button Task",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    if (rslt != pdPASS)
    {
        printf("Task Button initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
    
    if (!task_hw02_system_joystick_resources_init(xQueue_Request_Joystick))
    {
        printf("Task Joystick resource initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    if (!task_hw02_system_lcd_resources_init(xQueue_Request_LCD))
    {
        printf("Task LCD resource initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    rslt = xTaskCreate(
        task_hw02_system_control,
        "task_hw02_system_control", 
        TASK_SYSTEM_CONTROL_STACK_SIZE, 
        NULL, 
        TASK_SYSTEM_CONTROL_PRIORITY, 
        NULL
    );
    if (rslt != pdPASS)
    {
        printf("Task HW02 System Control creation failed!\n\r");
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
