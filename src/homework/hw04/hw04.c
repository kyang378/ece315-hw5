 /**
 * @file hw04.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "hw04.h"

#if defined(HW04)

char APP_DESCRIPTION[] = "ECE353 S26 HW04";

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_i2c_t *I2C_Monarch_Obj;
cyhal_spi_t *SPI_Monarch_Obj;

SemaphoreHandle_t SPI_Semaphore;
SemaphoreHandle_t I2C_Semaphore;


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
static void hw04_semaphores_init(void) {
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
static void hw04_queues_init(void)
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
    printf("\x1b[37m"); //makes text white for ease of reading
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

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
    /* Initialize the semaphores for I2C and SPI */
    hw04_semaphores_init();

    /* Initialize the queues for communication */
    hw04_queues_init();

    /* Initialize the resources needed for the console task */
    rslt = task_console_init();
    if (!rslt)
    {
        printf("Console Task resource initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Start any other tasks required to complete this homework */
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




    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif