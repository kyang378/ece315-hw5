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

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_i2c_t *I2C_Monarch_Obj;
cyhal_spi_t *SPI_Monarch_Obj;
EventGroupHandle_t ECE353_RTOS_Events = NULL; //May or may not be used

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
    // Create the Cap Touch request queue
    Queue_Request_Cap_Touch = xQueueCreate(1, sizeof(device_request_msg_t));
    if (Queue_Request_Cap_Touch == NULL)
    {
        printf("Failed to create Cap Touch Request Queue\n\r");
        CY_ASSERT(0);
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
void task_hw05_system_control(void *pvParameters){
    //Initialization requirements
    //1. initialize board and establish communicatoin
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

    //Note cap_touch is not initialized, if needed, add it
    
    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif