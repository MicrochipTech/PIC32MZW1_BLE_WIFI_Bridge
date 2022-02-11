/*******************************************************************************
  MPLAB Harmony Application Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_ble.h

  Summary:
    This header file provides prototypes and definitions for the application.

  Description:
    This header file provides function prototypes and data type definitions for
    the application.  Some of these are required by the system (such as the
    "APP_BLE_Initialize" and "APP_BLE_Tasks" prototypes) and some of them are only used
    internally by the application (such as the "APP_BLE_STATES" definition).  Both
    are defined here for convenience.
*******************************************************************************/

#ifndef _APP_BLE_H
#define _APP_BLE_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "configuration.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************
#define RN487X_BUFFER_SIZE          100
#define RN487X_TIMEOUT              0x0FFFFF        // response timeout
#define RN487X_STARTUP_DELAY        500             // value in ms
#define PROMPT_START				"CMD> "
#define PROMPT_END					"END\r\n"
#define REBOOT_MSG					"REBOOT"
#define CONNECT_MSG					"CONNECT"
#define STREAM_OPEN_MSG				"STREAM_OPEN"
#define DISCONNECT_MSG				"DISCONNECT"
#define REBOOTING_RESP				"Rebooting\r\n"
#define AOK_RESP					"AOK\r\n"
#define ERR_RESP					"Err\r\n"
#define STATUS_MESSAGE_DELIMITER	'%'

#define DEFAULT_TASK_DELAY          10              // value in ms
#define INIT_TASK_DELAY             2000

#define FRAME_TIMEOUT               10              // frameTimeout x task delay
    
#define EXPECTED_MSG_SIZE           RN487X_BUFFER_SIZE
#define RX_BUFFER_SIZE              RN487X_BUFFER_SIZE  
#define STATUS_MSG_BUFFER_SIZE      RN487X_BUFFER_SIZE
    
/* Keyword of provisioning command */
#define PROVISIONING_STX            '&' // start of provisioning string
#define PROVISIONING_ETX            '&' // end of provisioning string
#define PROVISIONING_CMD_KEYWORD    "wifiprov"
#define PROVISIONING_CMD_KEYWORD_LEN strlen(PROVISIONING_CMD_KEYWORD)
/* Minimum length of provisioning command */
#define MIN_PROVISIONING_CMD_LEN    15

/* Demo instructions */
#define DEMO_INSTRUCTIONS_1         "Open Microchip Bluetooth Data App\r\n"
#define DEMO_INSTRUCTIONS_2         "- Select BLE UART and BM70\r\n"
#define DEMO_INSTRUCTIONS_3         "- Connect to your device WFI32_xxxx\r\n"
#define DEMO_INSTRUCTIONS_4         "- Select Transparent\r\n"
#   
/* Format */
#define PROVISIONING_FORMAT_1       "- Frame format for Wi-Fi provisioning over BLE:\r\n"
#define PROVISIONING_FORMAT_2        "wifiprov|<ssid>|<authtype>|<password>"
#define PROVISIONING_AUTHTYPE_DETAIL "1: Open, 3: WPAWPA2, 4: WPA2, 5: WPA2WPA3, 6: WPA3\r\n"
#define PROVISIONING_EXAMPLE_1      "e.g. "
#define PROVISIONING_EXAMPLE_2      "wifiprov|DEMO_AP|3|password"
/*
wifiprov|<ssid>|<authtype>|<paswword>
authtype(Security type)
1 - OPEN Mode.
3 - WPAWPA2 (Mixed) mode.
4 - WPA2 mode.
5 - WPA2WPA3 (Mixed) mode.
6 - WPA3 mode.
*/
/*
e.g. wifiprov|DEMO_AP|3|password
 *   wifiprov|DEMO_AP|1
*/
typedef enum
{
    PROV_KEYWORD = 0,
    PROV_SSID,
    PROV_AUTHTYPE,
    PROV_PASSWORD
} PROV_INDEX ;

// *****************************************************************************
/* Application states

  Summary:
    Application states enumeration

  Description:
    This enumeration defines the valid application states.  These states
    determine the behavior of the application at various times.
*/

typedef enum
{
    /* Application's state machine's initial state. */
    APP_BLE_STATE_INIT=0,
    APP_BLE_STATE_RESET,
    APP_BLE_STATE_CONFIGURE,
    APP_BLE_STATE_WAIT_RSP,
    APP_BLE_STATE_WAIT_TRANSPARENT_DATA,
    APP_BLE_STATE_VALIDATE_FRAME,
    APP_BLE_STATE_EXTRACT_PROVISIONING,
    APP_BLE_STATE_SUCCESS,
    APP_BLE_STATE_ERROR
} APP_BLE_STATES;


// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    Application strings and buffers are be defined outside this structure.
 */

typedef struct
{
    /* The application's current state */
    APP_BLE_STATES state;
    APP_BLE_STATES nextState ;
    /* TODO: Define any additional data used by the application. */
    uint8_t cmdIndex ;
    uint16_t taskDelay ;
    uint32_t rspTimeout ;
    volatile uint8_t frameTimeout ;
    volatile char rxData ;
    /* Expected Message Buffer */
    char expectedMessage[RX_BUFFER_SIZE] ;
    uint8_t expectedMessageLen ;
    /* Reception Buffer */
    uint8_t rxBuffer[RX_BUFFER_SIZE] ;
    uint8_t rxBufferIndex ;
    uint8_t rxBufferLen ;
    /* Status Message Buffer */
    uint8_t statusMsgBuffer[RN487X_BUFFER_SIZE] ;
    uint8_t statusMsgBufferIndex ;
    uint8_t statusMsgBufferLen ;
    /* Flags */
    volatile bool statusMsgReceived ;
    volatile bool provisioningReceived ;
    volatile bool allCommandsSent ;
    volatile bool configurationDone ;
    volatile bool transparentInProgress ;
} APP_BLE_DATA;

typedef struct
{
    char *cmd ;
    char *expectedRsp ;
    //uint32_t timeout ;
} BLE_CMD ;

static const BLE_CMD ble_config[] = { \
/* Enter command mode  */       {"$$$", PROMPT_START}, \
/* Set serialized name */       {"S-,WFI32\r\n", AOK_RESP}, \
/* Enable transparent service*/ {"SS,C0\r\n", AOK_RESP}, \
/* Set adv. power to highest */ {"SGA,0\r\n", AOK_RESP}, \
/* Set con. power to highest */ {"SGC,0\r\n", AOK_RESP}, \
/* Reboot device */             {"R,1\r\n", REBOOTING_RESP}, \
/* End of command */            {NULL, NULL} \
} ;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Routines
// *****************************************************************************
// *****************************************************************************
/* These routines are called by drivers when certain events occur.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_BLE_Initialize ( void )

  Summary:
     MPLAB Harmony application initialization routine.

  Description:
    This function initializes the Harmony application.  It places the
    application in its initial state and prepares it to run so that its
    APP_BLE_Tasks function can be called.

  Precondition:
    All other system initialization routines should be called before calling
    this routine (in "SYS_Initialize").

  Parameters:
    None.

  Returns:
    None.

  Example:
    <code>
    APP_BLE_Initialize();
    </code>

  Remarks:
    This routine must be called from the SYS_Initialize function.
*/

void APP_BLE_Initialize ( void );


/*******************************************************************************
  Function:
    void APP_BLE_Tasks ( void )

  Summary:
    MPLAB Harmony Demo application tasks function

  Description:
    This routine is the Harmony Demo application's tasks function.  It
    defines the application's state machine and core logic.

  Precondition:
    The system and application initialization ("SYS_Initialize") should be
    called before calling this.

  Parameters:
    None.

  Returns:
    None.

  Example:
    <code>
    APP_BLE_Tasks();
    </code>

  Remarks:
    This routine must be called from SYS_Tasks() routine.
 */

void APP_BLE_Tasks( void );

void BLE_Delay(void) ;
void BLE_PrintInstructions(void) ;
void BLE_Init(void) ;
void BLE_SendCmd(char *cmd, uint8_t cmdLen) ;
bool BLE_NextCmd(void) ;
void BLE_GetRsp(char *rsp) ;
bool BLE_WaitExpectedRsp(const uint8_t *expectedMsg, uint8_t expectedMsgLen, uint32_t timeout) ;
void BLE_FlushRxBuffer(void) ;
void BLE_DumpRxBuffer(void) ;
void BLE_FillRxBuffer(char c) ;
void BLE_FlushStatusBuffer(void) ;
void BLE_DumpStatusBuffer(void) ;
void BLE_FillStatusBuffer(char c) ;
void BLE_HandleStatusMsg(void) ;
bool BLE_ValidateFrame(char *data) ;

//bool BLE_ExtractData(uint8_t *data) ;

//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif /* _APP_BLE_H */

/*******************************************************************************
 End of File
 */

