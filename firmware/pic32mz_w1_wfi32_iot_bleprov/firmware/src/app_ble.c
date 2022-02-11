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
#include "string.h"
#include "stdlib.h"

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

// Used to filter incoming status message (%MSG%)
static bool asyncFiltering = false ;
static bool dataReady = false ;
// Used to detect provisioning frame in transparent data mode
static bool provStrFiltering = false ;
static uint8_t provStrLen = 0 ;

#define APP_BLE_PRINT_ALL_MSG       0   // print all message received
#define APP_BLE_PRINT_STATUS_MSG    0   // print status message
#define APP_BLE_PRINT_RX_MSG        0   // print received message

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/
void BLE_RxHandler(uintptr_t context)
{
    if (UART2_ErrorGet() == UART_ERROR_NONE)
    {
        // read one byte
        UART2_Read((void*)&app_bleData.rxData, 1) ;
#if (APP_BLE_PRINT_ALL_MSG == 1)
        //for debug - printing any data received
        UART1_Write((void*)&app_bleData.rxData, 1) ;
#endif
        // handle if status message received (%MSG%)
        if (asyncFiltering == false)
        {
            if (app_bleData.rxData == STATUS_MESSAGE_DELIMITER)
            {   // first delimiter found, do not capture data
                // prepare to filter further incoming data until next delimiter
                asyncFiltering = true ;
            }
            else
            {   // data not considered as status message, no filtering
                dataReady = true ;
            }
        }
        else
        {   // capture data in a dedicated buffer for status message
            if (app_bleData.rxData == STATUS_MESSAGE_DELIMITER)
            {   // second delimiter found, do not capture data
                asyncFiltering = false ;
                app_bleData.statusMsgBuffer[app_bleData.statusMsgBufferIndex] = '\0' ;
                app_bleData.statusMsgBufferLen = app_bleData.statusMsgBufferIndex + 1 ;
                // entire status message received
                app_bleData.statusMsgReceived = true ;
            }
            else
            {   // fill status message buffer
                BLE_FillStatusBuffer(app_bleData.rxData) ;
#if (APP_BLE_PRINT_STATUS_MSG == 1)
                //for debug - printing only status message
                UART1_Write((void*)&app_bleData.rxData, 1) ;
#endif
            }
        }
        
        if (dataReady)
        {   // data is ready for RX Buffer
            dataReady = false ;
#if (APP_BLE_PRINT_RX_MSG == 1)
            //for debug - printing RX message
            UART1_Write((void*)&app_bleData.rxData, 1) ;
#endif
            if (app_bleData.configurationDone == false)
            {   // in configuration mode
                // capture data in reception buffer
                BLE_FillRxBuffer(app_bleData.rxData) ;
            }
            else
            {   // in transparent data mode
                app_bleData.transparentInProgress = true ;
                // re-arm frame timeout
                app_bleData.frameTimeout = FRAME_TIMEOUT ;
                // search for provisioning frame
                if (provStrFiltering == false)
                {
                    if (app_bleData.rxData == PROVISIONING_STX)
                    {   // start of provisioning string
                        provStrFiltering = true ;
                        provStrLen = 0 ;
                    }
                }
                else
                {
                    if (app_bleData.rxData == PROVISIONING_ETX)
                    {   // end of provisioning string
                        provStrFiltering = false ;
                        //app_bleData.rxBuffer[app_bleData.rxBufferIndex] = '\0' ;
                        //app_bleData.rxBufferLen = app_bleData.rxBufferIndex + 1 ;
                        // entire provisioning message received
                        app_bleData.provisioningReceived = true ;
                    }
                    else
                    {
                        provStrLen++ ;
                        BLE_FillRxBuffer(app_bleData.rxData) ;
                    }
                }           
            }
        }
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

void BLE_PrintInstructions(void)
{
    SYS_CONSOLE_MESSAGE(DEMO_INSTRUCTIONS_1) ;
    SYS_CONSOLE_MESSAGE(DEMO_INSTRUCTIONS_2) ;
    SYS_CONSOLE_MESSAGE(DEMO_INSTRUCTIONS_3) ;
    SYS_CONSOLE_MESSAGE(DEMO_INSTRUCTIONS_4) ;
    SYS_CONSOLE_MESSAGE(PROVISIONING_FORMAT_1) ;
    SYS_CONSOLE_PRINT("%c%s%c\r\n", PROVISIONING_STX, PROVISIONING_FORMAT_2, PROVISIONING_ETX) ;
    SYS_CONSOLE_MESSAGE(PROVISIONING_AUTHTYPE_DETAIL) ;
    SYS_CONSOLE_PRINT("%s%c%s%c\r\n", PROVISIONING_EXAMPLE_1, PROVISIONING_STX, PROVISIONING_EXAMPLE_2, PROVISIONING_ETX) ;
}

void BLE_Init(void)
{
    // start by the first configuration command
    app_bleData.cmdIndex = 0 ;
    // reset flags
    app_bleData.configurationDone = false ;
    app_bleData.provisioningReceived = false ;
    app_bleData.statusMsgReceived = false ;
    app_bleData.allCommandsSent = false ;
    app_bleData.transparentInProgress = false ;
    // make sure to let other tasks to get initialized
    app_bleData.taskDelay = INIT_TASK_DELAY ;
    // clean all buffers
    BLE_FlushRxBuffer() ;
    BLE_FlushStatusBuffer() ;
}

// Send command
void BLE_SendCmd(char *cmd, uint8_t cmdLen)
{
    UART2_Write(&cmd[0], cmdLen) ;
    while (UART2_WriteIsBusy()) ;
}

// Move to next command
// Return false if no more command to execute
bool BLE_NextCmd(void)
{
    // prepare for next command
    app_bleData.cmdIndex++ ;
    if (ble_config[app_bleData.cmdIndex].cmd == NULL)
    {
        return false ;
    }
    return true ;
}

// Prepare to receive expected response
void BLE_PrepareRsp(char *rsp)
{
    app_bleData.rspTimeout = RN487X_TIMEOUT ;
    memset(&app_bleData.expectedMessage, 0, EXPECTED_MSG_SIZE) ;
    memcpy(&app_bleData.expectedMessage, rsp, strlen(rsp)) ;
    app_bleData.expectedMessageLen = strlen(rsp) ;
}

// wait until timeout to receive expected response
bool BLE_WaitExpectedRsp(const uint8_t *expectedMsg, uint8_t expectedMsgLen, uint32_t timeout)
{
    uint8_t index = 0 ;
    uint8_t resp ;
    while ((timeout > 0) && timeout --)
    {
        if (index < expectedMsgLen)
        {
            resp = app_bleData.rxBuffer[index] ;
            if (resp == expectedMsg[index])
            {
                index++ ;
            }
            else
            {
                index = 0 ;
                if (resp == expectedMsg[index])
                {
                    index++ ;
                }
            }
        }
        else
        {
            return true ;
        }
    }
    return false ;
}

// Flush RX buffer
void BLE_FlushRxBuffer(void)
{
    app_bleData.rxBufferIndex = 0 ;
    memset(&app_bleData.rxBuffer, 0, RX_BUFFER_SIZE) ;
}

void BLE_DumpRxBuffer(void)
{
    uint8_t i ;
    for (i = 0; i < RX_BUFFER_SIZE; i++)
    {
        UART1_Write((void*)&app_bleData.rxBuffer[i], 1) ;
    }
}

void BLE_FillRxBuffer(char c)
{
    app_bleData.rxBuffer[app_bleData.rxBufferIndex++] = c ;
    if (app_bleData.rxBufferIndex >= RX_BUFFER_SIZE)
        app_bleData.rxBufferIndex = 0 ;
}

// Flush Status message buffer
void BLE_FlushStatusBuffer(void)
{
    app_bleData.statusMsgBufferIndex = 0 ;
    app_bleData.statusMsgBufferLen = 0 ;
    memset(&app_bleData.statusMsgBuffer, 0, STATUS_MSG_BUFFER_SIZE) ;
}

void BLE_DumpStatusBuffer(void)
{
    uint8_t i ;
    for (i = 0; i < STATUS_MSG_BUFFER_SIZE; i++)
    {
        UART1_Write((void*)&app_bleData.statusMsgBuffer[i], 1) ;
    }
}

void BLE_FillStatusBuffer(char c)
{
    app_bleData.statusMsgBuffer[app_bleData.statusMsgBufferIndex++] = c ;
    if (app_bleData.statusMsgBufferIndex >= STATUS_MSG_BUFFER_SIZE)
        app_bleData.statusMsgBufferIndex = 0 ;
}

void BLE_HandleStatusMsg(void)
{
    if (strstr((const char*)app_bleData.statusMsgBuffer, DISCONNECT_MSG))
    {
        SYS_CMD_MESSAGE("\r\n[APP_BLE] Disconnected\r\n") ;
        BLE_FlushStatusBuffer() ;
        LED_GREEN_Off() ;
    }
    else if (strstr((const char*)app_bleData.statusMsgBuffer, STREAM_OPEN_MSG))
    {
        SYS_CMD_MESSAGE("\r\n[APP_BLE] Transparent stream opened\r\n") ;
        BLE_FlushStatusBuffer() ;
    }
    else if (strstr((const char*)app_bleData.statusMsgBuffer, CONNECT_MSG))
    {
        SYS_CMD_MESSAGE("\r\n[APP_BLE] Connected\r\n") ;
        BLE_FlushStatusBuffer() ;
        LED_GREEN_On() ;
    }
}

// Validate provisioning frame
bool BLE_ValidateFrame(char *data)
{
    PROV_INDEX index = 0 ;
    char str[100 + PROVISIONING_CMD_KEYWORD_LEN] ;
    char ssid[32] ;
    char pass[64] ;
    char auth[4] ;
    
    char *strToken ;
    //const char *separators = " ,.-!&" ;
    const char *separators = "|&" ;
    
    memset(&ssid, 0, sizeof(ssid)) ;  
    memset(&pass, 0, sizeof(pass)) ;
    memset(&auth, 0, sizeof(auth)) ;
    
    memset(&str, 0, sizeof(str)) ;
    memcpy(&str, data, sizeof(str)) ;
    
    if (strlen(str) < MIN_PROVISIONING_CMD_LEN)
        return false ;

    // index:               0       1       2           3
    // expected format: <keyword>|<ssid>|<authtype>|<password>

    // start by the first token
    strToken = strtok((char*)str, separators) ;
    while (strToken != NULL)
    {
        //SYS_CONSOLE_PRINT("%s\n", strToken) ;
        switch (index)
        {
            case PROV_KEYWORD:
            {
                // check the keyword
                if (strcmp(strToken, PROVISIONING_CMD_KEYWORD) != 0)
                    return false ;
                index++ ;
                break ;
            }
            case PROV_SSID:
            {
                // extract the SSID
                if (strlen(strToken) > sizeof(ssid))
                    return false ;
                memcpy(&ssid, strToken, strlen(strToken)) ;
                //SYS_CONSOLE_PRINT("\r\nExtracted SSID: %s\r\n", ssid) ;
                index++ ;
                break ;
            }
            case PROV_AUTHTYPE:
            {
                // extract the authentication type
                if (strlen(strToken) != 1)
                    return false ;
                // check limit in sys_wifi.h / SYS_WIFI_AUTH
                if (strToken[0] < '1' || strToken[0] > '6' || strToken[0] == '2')
                    return false ;
                memcpy(&auth, strToken, strlen(strToken)) ;
                //SYS_CONSOLE_PRINT("Extracted Authtype: %c\r\n", auth[0]) ;
                index++ ;
                break ;
            }
            case PROV_PASSWORD:
            {
                // extract the password
                if (strlen(strToken) > sizeof(pass))
                    return false ;
                memcpy(&pass, strToken, strlen(strToken)) ;
                //SYS_CONSOLE_PRINT("Extracted Password: %s\r\n", pass) ;
                break ;
            }
        }
        // move to the next token
        strToken = strtok(NULL, separators) ;
    }

    SYS_CONSOLE_PRINT("SSID: %s - AUTH: %c - PASS: %s\r\n", ssid, auth[0], pass) ;    
    
    // populate WiFi Config with extracted data
    memset(&app_wifiData.wifiConfig.staConfig.ssid, 0, sizeof(app_wifiData.wifiConfig.staConfig.ssid)) ;
    memset(&app_wifiData.wifiConfig.staConfig.psk, 0, sizeof(app_wifiData.wifiConfig.staConfig.psk)) ;
    memcpy(app_wifiData.wifiConfig.staConfig.ssid, ssid, strlen((char*)ssid)) ;
    memcpy(app_wifiData.wifiConfig.staConfig.psk, pass, strlen((char*)pass)) ;
    app_wifiData.wifiConfig.staConfig.authType = (SYS_WIFI_AUTH)(auth[0] - '0') ;
    return true ;
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

    UART2_ReadCallbackRegister(BLE_RxHandler, (uintptr_t)NULL) ;
    UART2_Read((void*)&app_bleData.rxData, 1) ;
}

/******************************************************************************
  Function:
    void APP_BLE_Tasks ( void )

  Remarks:
    See prototype in app_ble.h.
 */
void APP_BLE_Tasks ( void )
{
    /* Check the application's current state. */
    switch ( app_bleData.state )
    {
        /* Application's initial state. */
        case APP_BLE_STATE_INIT:
        {
            BLE_Init() ;
            app_bleData.state = APP_BLE_STATE_RESET ;
            break;
        }
        case APP_BLE_STATE_RESET:
        {
            SYS_CONSOLE_MESSAGE("[APP_BLE] Init.\r\n") ;            
            BLE_RST_Set() ;
            BLE_Delay() ;
            BLE_RST_Clear() ;
            BLE_Delay() ;
            BLE_RST_Set() ;
            // ignore all incoming data for a certain time after performed a reset
            app_bleData.taskDelay = RN487X_STARTUP_DELAY ;
            // move to the next state after that delay
            app_bleData.state = APP_BLE_STATE_CONFIGURE ;            
            break ;
        }
        case APP_BLE_STATE_CONFIGURE:
        {
            // restore default task delay
            app_bleData.taskDelay = DEFAULT_TASK_DELAY ;
            // prepare to get expected response
            BLE_PrepareRsp(ble_config[app_bleData.cmdIndex].expectedRsp) ;
            // clean receive buffer
            BLE_FlushRxBuffer() ;
            // execute one-by-one the configuration commands
            BLE_SendCmd(ble_config[app_bleData.cmdIndex].cmd, strlen(ble_config[app_bleData.cmdIndex].cmd)) ;
            // other command to execute ?
            if (BLE_NextCmd())
            {   // continue configuration
                app_bleData.nextState = APP_BLE_STATE_CONFIGURE ;
                app_bleData.state = APP_BLE_STATE_WAIT_RSP ;
            }
            else
            {   // executed all configuration commands
                app_bleData.allCommandsSent = true ;
                app_bleData.nextState = APP_BLE_STATE_WAIT_TRANSPARENT_DATA ;
            }
            // wait response of the latest command sent
            app_bleData.state = APP_BLE_STATE_WAIT_RSP ;
            break;
        }
        case APP_BLE_STATE_WAIT_RSP:
        {   // wait a certain time for expected response in configuration mode
            if (BLE_WaitExpectedRsp((uint8_t*)app_bleData.expectedMessage, app_bleData.expectedMessageLen, app_bleData.rspTimeout))
            {   // received expected response
                if (app_bleData.allCommandsSent)
                {
                    SYS_CONSOLE_MESSAGE("[APP_BLE] Configuration done.\r\n") ;
                    BLE_PrintInstructions() ;
                    app_bleData.allCommandsSent = false ;
                    app_bleData.configurationDone = true ;
                }
                // flush the reception buffer
                BLE_FlushRxBuffer() ;
                // move to the next state
                app_bleData.state = app_bleData.nextState ;
            }
            else
            {   // issue
                BLE_DumpRxBuffer() ;
                app_bleData.state = APP_BLE_STATE_ERROR ;
            }
            break ;
        }
        case APP_BLE_STATE_WAIT_TRANSPARENT_DATA:
        {   // wait until getting the provisioning string
            if (app_bleData.provisioningReceived)
            {
                SYS_CMD_MESSAGE("\r\n[APP_BLE] Frame received\r\n") ;                
                app_bleData.provisioningReceived = false ;
                app_bleData.state = APP_BLE_STATE_VALIDATE_FRAME ;
            }
            // monitor connection from status message
            if (app_bleData.statusMsgReceived)
            {   // handle status message received
                app_bleData.statusMsgReceived = false ;
                BLE_HandleStatusMsg() ;
            }
            // handle frame timeout when receiving transparent data
            if (app_bleData.transparentInProgress)
            {
                if (app_bleData.frameTimeout == 0)
                {
                    app_bleData.transparentInProgress = false ;
                    provStrFiltering = false ;
                    BLE_FlushRxBuffer() ;
                }
                else app_bleData.frameTimeout-- ;
            }
            break ;
        }
        case APP_BLE_STATE_VALIDATE_FRAME:
        {   // validate the frame received
            if (strstr((char*)app_bleData.rxBuffer, PROVISIONING_CMD_KEYWORD) != NULL)
            {
                //SYS_CMD_PRINT("%s", strstr((char*)app_bleData.rxBuffer, PROVISIONING_CMD_KEYWORD)) ;
                if (BLE_ValidateFrame(strstr((char*)app_bleData.rxBuffer, PROVISIONING_CMD_KEYWORD)))
                {
                    app_bleData.state = APP_BLE_STATE_SUCCESS ;
                }
                else
                {
                    SYS_CMD_MESSAGE("\r\n[APP_BLE] Wrong frame format\r\n") ;
                    app_bleData.state = APP_BLE_STATE_WAIT_TRANSPARENT_DATA ;                    
                }
            }
            else
            {
                SYS_CMD_MESSAGE("\r\n[APP_BLE] Wrong frame format\r\n") ;
                app_bleData.state = APP_BLE_STATE_WAIT_TRANSPARENT_DATA ;
            }
            BLE_FlushRxBuffer() ;
            break ;            
        }
        case APP_BLE_STATE_SUCCESS:
        {
            LED_RED_Off() ;
            // ask APP_WIFI to set new config
            app_wifiData.newWiFiConfig = true ;
            app_bleData.state = APP_BLE_STATE_WAIT_TRANSPARENT_DATA ;
            break ;
        }
        case APP_BLE_STATE_ERROR:
        {
            LED_RED_On() ;
            break ;
        }
        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
    vTaskDelay(app_bleData.taskDelay / portTICK_PERIOD_MS) ;
}


/*******************************************************************************
 End of File
 */
