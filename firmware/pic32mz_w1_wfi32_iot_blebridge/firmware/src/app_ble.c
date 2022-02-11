/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_ble.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "app_ble.h"
#include "definitions.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_BLE_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_BLE_DATA app_bleData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

void Input_RXHandler(uintptr_t context)
{
    if (UART1_ErrorGet() == UART_ERROR_NONE)
    {
        UART1_Read((void*)&app_bleData.rxData, 1) ;
        UART2_Write((void*)&app_bleData.rxData, 1) ;
    }
}

void BLE_RxHandler(uintptr_t context)
{
    if (UART2_ErrorGet() == UART_ERROR_NONE)
    {
        // read one byte
        UART2_Read((void*)&app_bleData.rxData, 1) ;
        UART1_Write((void*)&app_bleData.rxData, 1) ;
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

void BLE_Delay(void)
{
    uint32_t delay = 200000 ;
    while (delay > 0)
    {
        delay-- ;
        Nop() ;
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_BLE_Initialize ( void )

  Remarks:
    See prototype in app_ble.h.
 */

void APP_BLE_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    app_bleData.state = APP_BLE_STATE_INIT;

    UART1_ReadCallbackRegister(Input_RXHandler, (uintptr_t)NULL) ;
    UART1_Read((void*)&app_bleData.rxData, 1) ;
    
    UART2_ReadCallbackRegister(BLE_RxHandler, (uintptr_t)NULL) ;
    UART2_Read((void*)&app_bleData.rxData, 1) ;
}

/******************************************************************************
  Function:
    void APP_BLE_Tasks ( void )

  Remarks:
    See prototype in app_ble.h.
 */
//bool taskDelayEnabled = false;
void APP_BLE_Tasks ( void )
{
    /* Check the application's current state. */
    switch ( app_bleData.state )
    {
        /* Application's initial state. */
        case APP_BLE_STATE_INIT:
        {
            app_bleData.state = APP_BLE_STATE_RESET ;
            break;
        }
        case APP_BLE_STATE_RESET:
        {
            BLE_RST_Set() ;
            BLE_Delay() ;
            BLE_RST_Clear() ;
            BLE_Delay() ;
            BLE_RST_Set() ;
            app_bleData.state = APP_BLE_STATE_SERIAL_BRIDGE ;            
            break ;
        }
        case APP_BLE_STATE_SERIAL_BRIDGE:
        {   // do nothing as communication is handled thru callback
            break ;
        }
        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


/*******************************************************************************
 End of File
 */
