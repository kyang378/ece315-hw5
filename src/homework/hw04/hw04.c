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
static SemaphoreHandle_t HW04_I2C_Semaphore = NULL;
static SemaphoreHandle_t HW04_SPI_Semaphore = NULL;

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
static void hw04_semaphores_init(void)
{
    HW04_I2C_Semaphore = xSemaphoreCreateBinary();
    HW04_SPI_Semaphore = xSemaphoreCreateBinary();

    if(HW04_I2C_Semaphore == NULL || HW04_SPI_Semaphore == NULL)
    {
        task_console_printf("Failed to create hw04 semaphores!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    xSemaphoreGive(HW04_I2C_Semaphore);
    xSemaphoreGive(HW04_SPI_Semaphore);
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
    Queue_Request_Cap_Touch = xQueueCreate(1, sizeof(device_request_msg_t));

    if (Queue_Request_Cap_Touch == NULL)
    {
        task_console_printf("Failed to create Cap Touch request queue!\n\r");
        for(int i = 0; i < 100000; i++) {}
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
    console_init();
    // Set text color to black
    task_console_printf("\x1b[30m");
    task_console_printf("\x1b[2J\x1b[;H");
    task_console_printf("**************************************************\n\r");
    task_console_printf("* %s\n\r", APP_DESCRIPTION);
    task_console_printf("* Date: %s\n\r", __DATE__);
    task_console_printf("* Time: %s\n\r", __TIME__);
    task_console_printf("* Name:%s\n\r", NAME);
    task_console_printf("**************************************************\n\r");

    /* Initialize the I2C interface */
    I2C_Monarch_Obj = i2c_init(PIN_I2C_SDA, PIN_I2C_SCL);
    if( I2C_Monarch_Obj == NULL)
    {
        task_console_printf("I2C Initialization Failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Initialize SPI Interface */
    SPI_Monarch_Obj = spi_init(PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_CLK);
    if( SPI_Monarch_Obj == NULL)
    {
        task_console_printf("SPI Initialization Failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Initialize the CS pin for the EEPROM */
    cyhal_gpio_init(PIN_SPI_EEPROM_CS, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

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
        task_console_printf("Console Task resource initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Start any other tasks required to complete this homework */
    rslt = task_eeprom_resources_init(&HW04_SPI_Semaphore, SPI_Monarch_Obj, PIN_SPI_EEPROM_CS);
    if (!rslt)
    {
        task_console_printf("EEPROM Task resource initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    rslt = task_cap_touch_resources_init(
        Queue_Request_Cap_Touch,
        HW04_I2C_Semaphore,
        I2C_Monarch_Obj,
        NC
    );
    if (!rslt)
    {
        task_console_printf("Cap Touch Task resource initialization failed!\n\r");
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
