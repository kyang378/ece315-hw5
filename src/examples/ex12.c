/**
 * @file ex03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(EX12)
#include "buzzer.h"
#include "drivers.h"
#include "task_console.h"
#include "task_imu.h"

char APP_DESCRIPTION[] = "ECE353: Example 12 - FreeRTOS IMU";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
SemaphoreHandle_t Spi_Semaphore = NULL;
cyhal_spi_t *SPI_Obj;
QueueHandle_t Queue_IMU_Responses = NULL;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/
bool task_system_control_resources_init(void);
void task_system_control(void *arg);

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
bool task_system_control_resources_init(void)
{
    Queue_IMU_Responses = xQueueCreate(1, sizeof(device_response_msg_t));
    if(Queue_IMU_Responses == NULL)
    {
        return false;
    }

    if (xTaskCreate(
            task_system_control,
            "System Control",
            5*configMINIMAL_STACK_SIZE,
            NULL,
            tskIDLE_PRIORITY + 1,
            NULL) != pdPASS)
    {
        return false;
    }

    return true;
}

void task_system_control(void *arg)
{
    (void)arg;
    bool return_status = false;
    int16_t accel_data[3] = {0};

    task_console_printf("Starting System Control Task\r\n");

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));

        return_status = system_sensors_get_imu(Queue_IMU_Responses, accel_data);
        if(!return_status)
        {
            task_console_printf("IMU read request failed!\r\n");
        }
    }
}

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    // Initialize the SPI interface
    SPI_Obj = spi_init(PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_CLK);
    if(SPI_Obj == NULL)
    {
        printf("SPI initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Initialize the IMU CS pin
    rslt = cyhal_gpio_init(PIN_SPI_IMU_CS, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("IMU CS pin initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
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
    if(!task_system_control_resources_init())
    {
        printf("System Control Task initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_console_init())
    {
        printf("Console initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Create the binary semaphore for SPI access
    Spi_Semaphore = xSemaphoreCreateBinary();
    if(Spi_Semaphore == NULL)
    {
        printf("Failed to create SPI semaphore!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Make the semaphore available for the first time
    xSemaphoreGive(Spi_Semaphore);

    // Initialize IMU task resources
    if(!task_imu_resources_init(&Spi_Semaphore, SPI_Obj, PIN_SPI_IMU_CS))
    {
        printf("Failed to initialize IMU task resources!\n\r");
        for(int i = 0; i < 10000; i++);
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
