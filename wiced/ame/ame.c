#include "bleprofile.h"
#include "bleapp.h"
#include "gpiodriver.h"
#include "string.h"
#include "stdio.h"
#include "platform.h"
#include "spar_utils.h"
#include "sparcommon.h"
#include "pwm.h"
#include "bleappevent.h"
#include "bt_clock_based_timer.h"
#include "ame.h"
#include "log.h"

/******************************************************
 *                      Constants
 ******************************************************/
#define PIO0 (26)
#define PIO1 (27)
#define PIO2 (28)
#define PIO3 (25)
#define PIO4 (4)
#define PIO5 (24)
#define PIN_SWITCH (PIO4)
#define POUT_LEDR (PIO0)
#define POUT_LEDG (PIO1)
#define POUT_LEDB (PIO2)

#define BLE_COMMAND_R (0x41)
#define BLE_COMMAND_G (0x42)
#define BLE_COMMAND_B (0x43)

#define LED_PWM_R (PWM0)
#define LED_PWM_G (PWM1)
#define LED_PWM_B (PWM2)

/******************************************************
 *                     Structures
 ******************************************************/

#pragma pack(1)
//host information for NVRAM
typedef PACKED struct
{
    // BD address of the bonded host
    BD_ADDR  bdaddr;

    // Current value of the client configuration descriptor
    UINT16  characteristic_client_configuration;

    // sensor configuration. number of times to blink when button is pushed.
    UINT8   number_of_blinks;

}  HOSTINFO;
#pragma pack()

/******************************************************
 *               Function Prototypes
 ******************************************************/

static void ame_create(void);
static void ame_timeout( UINT32 count );
static void ame_fine_timeout( UINT32 finecount );
static void ame_connection_up( void );
static void ame_connection_down( void );
static void ame_advertisement_stopped( void );
static int  ame_write_handler( LEGATTDB_ENTRY_HDR *p );
static void ame_gpio_interrupt_handler(void *parameter, UINT8 arg);
static int ame_timer_expired_callback(void* context);
static void led_init(void);
static void led_open(void);
static void pwm_stop(void);

/******************************************************
 *               Variables Definitions
 ******************************************************/

/*
 * This is the GATT database for the Hello Sensor application.  It defines
 * services, characteristics and descriptors supported by the sensor.  Each
 * attribute in the database has a handle, (characteristic has two, one for
 * characteristic itself, another for the value).  The handles are used by
 * the peer to access attributes, and can be used locally by application for
 * example to retrieve data written by the peer.  Definition of characteristics
 * and descriptors has GATT Properties (read, write, notify...) but also has
 * permissions which identify if application is allowed to read or write
 * into it.  Handles do not need to be sequential, but need to be in order.
 */
const UINT8 ame_gatt_database[]=
{
    // Handle 0x01: GATT service
	// Service change characteristic is optional and is not present
    PRIMARY_SERVICE_UUID16 (0x0001, UUID_SERVICE_GATT),

    // Handle 0x14: GAP service
    // Device Name and Appearance are mandatory characteristics.  Peripheral
    // Privacy Flag only required if privacy feature is supported.  Reconnection
    // Address is optional and only when privacy feature is supported.
    // Peripheral Preferred Connection Parameters characteristic is optional
    // and not present.
    PRIMARY_SERVICE_UUID16 (0x0014, UUID_SERVICE_GAP),

    // Handle 0x15: characteristic Device Name, handle 0x16 characteristic value.
    // Any 16 byte string can be used to identify the sensor.  Just need to
    // replace the "Hello" string below.  Keep it short so that it fits in
    // advertisement data along with 16 byte UUID.
    CHARACTERISTIC_UUID16 (0x0015, 0x0016, UUID_CHARACTERISTIC_DEVICE_NAME,
    					   LEGATTDB_CHAR_PROP_READ, LEGATTDB_PERM_READABLE, 16),
       'A','M','E',0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    // Handle 0x17: characteristic Appearance, handle 0x18 characteristic value.
    // List of approved appearances is available at bluetooth.org.  Current
    // value is set to 0x200 - Generic Tag
    CHARACTERISTIC_UUID16 (0x0017, 0x0018, UUID_CHARACTERISTIC_APPEARANCE,
    					   LEGATTDB_CHAR_PROP_READ, LEGATTDB_PERM_READABLE, 2),
        BIT16_TO_8(APPEARANCE_GENERIC_TAG),

    // Handle 0x28: Hello Service.
    // This is the main proprietary service of Hello Sensor.  It has 2 characteristics.
    // One will be used to send notification(s) to the paired client when button is
    // pushed, another is a configuration of the device.  The only thing which
    // can be configured is number of times to send notification.  Note that
    // UUID of the vendor specific service is 16 bytes, unlike standard Bluetooth
    // UUIDs which are 2 bytes.  _UUID128 version of the macro should be used.
    PRIMARY_SERVICE_UUID128 (HANDLE_AME_SERVICE_UUID, UUID_AME_SERVICE),

    // Characteristic for Open Status.
    CHARACTERISTIC_UUID128 (
        HANDLE_AME_CHARACTERISTIC_BUTTON,
        HANDLE_AME_VALUE_BUTTON,
        UUID_AME_CHARACTERISTIC_BUTTON,
        LEGATTDB_CHAR_PROP_READ | LEGATTDB_CHAR_PROP_NOTIFY | LEGATTDB_CHAR_PROP_INDICATE,
        LEGATTDB_PERM_READABLE,
        1),
        0x00,

    // Characteristic for something write
    CHARACTERISTIC_UUID128_WRITABLE (
        HANDLE_AME_CHARACTERISTIC_WEATHER,
        HANDLE_AME_VALUE_WEATHER,
        UUID_AME_CHARACTERISTIC_WEATHER,
        LEGATTDB_CHAR_PROP_WRITE ,
        LEGATTDB_PERM_WRITE_CMD | LEGATTDB_PERM_WRITE_REQ,
        1),
        0x00,
};

const BLE_PROFILE_CFG ame_cfg =
{
    /*.fine_timer_interval            =*/ 1000, // ms
    /*.default_adv                    =*/ 4,    // HIGH_UNDIRECTED_DISCOVERABLE
    /*.button_adv_toggle              =*/ 0,    // pairing button make adv toggle (if 1) or always on (if 0)
    /*.high_undirect_adv_interval     =*/ 2056,   // slots
    /*.low_undirect_adv_interval      =*/ 2056, // slots
    /*.high_undirect_adv_duration     =*/ 10,   // seconds
    /*.low_undirect_adv_duration      =*/ 120,  // seconds
    /*.high_direct_adv_interval       =*/ 10,    // seconds
    /*.low_direct_adv_interval        =*/ 120,    // seconds
    /*.high_direct_adv_duration       =*/ 10,    // seconds
    /*.low_direct_adv_duration        =*/ 120,    // seconds
    /*.local_name                     =*/ "AME-001",        // [LOCAL_NAME_LEN_MAX];
    /*.cod                            =*/ BIT16_TO_8(APPEARANCE_GENERIC_TAG),0x00, // [COD_LEN];
    /*.ver                            =*/ "1.00",         // [VERSION_LEN];
    /*.encr_required                  =*/ (SECURITY_ENABLED | SECURITY_REQUEST),    // data encrypted and device sends security request on every connection
    /*.disc_required                  =*/ 0,    // if 1, disconnection after confirmation
    /*.test_enable                    =*/ 1,    // TEST MODE is enabled when 1
    /*.tx_power_level                 =*/ 0x00, // dbm
    /*.con_idle_timeout               =*/ 3,    // second  0-> no timeout
    /*.powersave_timeout              =*/ 0,    // second  0-> no timeout
    /*.hdl                            =*/ {0x00, 0x0063, 0x00, 0x00, 0x00}, // [HANDLE_NUM_MAX];
    /*.serv                           =*/ {0x00, UUID_SERVICE_BATTERY, 0x00, 0x00, 0x00},
    /*.cha                            =*/ {0x00, UUID_CHARACTERISTIC_BATTERY_LEVEL, 0x00, 0x00, 0x00},
    /*.findme_locator_enable          =*/ 0,    // if 1 Find me locator is enable
    /*.findme_alert_level             =*/ 0,    // alert level of find me
    /*.client_grouptype_enable        =*/ 0,    // if 1 grouptype read can be used
    /*.linkloss_button_enable         =*/ 0,    // if 1 linkloss button is enable
    /*.pathloss_check_interval        =*/ 0,    // second
    /*.alert_interval                 =*/ 0,    // interval of alert
    /*.high_alert_num                 =*/ 0,    // number of alert for each interval
    /*.mild_alert_num                 =*/ 0,    // number of alert for each interval
    /*.status_led_enable              =*/ 0,    // if 1 status LED is enable
    /*.status_led_interval            =*/ 0,    // second
    /*.status_led_con_blink           =*/ 0,    // blink num of connection
    /*.status_led_dir_adv_blink       =*/ 0,    // blink num of dir adv
    /*.status_led_un_adv_blink        =*/ 0,    // blink num of undir adv
    /*.led_on_ms                      =*/ 0,    // led blink on duration in ms
    /*.led_off_ms                     =*/ 0,    // led blink off duration in ms
    /*.buz_on_ms                      =*/ 100,  // buzzer on duration in ms
    /*.button_power_timeout           =*/ 0,    // seconds
    /*.button_client_timeout          =*/ 0,    // seconds
    /*.button_discover_timeout        =*/ 0,    // seconds
    /*.button_filter_timeout          =*/ 0,    // seconds
};

// Following structure defines UART configuration
const BLE_PROFILE_PUART_CFG ame_puart_cfg =
{
    /*.baudrate   =*/ 115200,
    /*.txpin      =*/ PUARTDISABLE | GPIO_PIN_UART_TX,
    /*.rxpin      =*/ PUARTDISABLE | GPIO_PIN_UART_RX,
};

// Following structure defines GPIO configuration used by the application
const BLE_PROFILE_GPIO_CFG ame_gpio_cfg =
{
    /*.gpio_pin =*/
    {
        GPIO_PIN_WP,      // This need to be used to enable/disable NVRAM write protect
        GPIO_PIN_BUTTON,  // Button GPIO is configured to trigger either direction of interrupt
        GPIO_PIN_LED,     // LED GPIO, optional to provide visual effects
        GPIO_PIN_BATTERY, // Battery monitoring GPIO. When it is lower than particular level, it will give notification to the application
        -1,-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 // other GPIOs are not used
    },
    /*.gpio_flag =*/
    {
        GPIO_SETTINGS_WP,
        GPIO_SETTINGS_BUTTON,
        GPIO_SETTINGS_LED,
        GPIO_SETTINGS_BATTERY,
        0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
};

UINT32 	ame_timer_count        = 0;
UINT32 	ame_fine_timer_count   = 0;
UINT16 	ame_connection_handle	= 0;	// HCI handle of connection, not zero when connected
BD_ADDR ame_remote_addr        = {0, 0, 0, 0, 0, 0};
UINT8 	ame_indication_sent    = 0;	// indication sent, waiting for ack
UINT8   ame_num_to_write       = 0;  	// Number of messages we need to send

// NVRAM save area
HOSTINFO ame_hostinfo;

UINT8 cap_sense_counter = 0;
UINT8 cap_sense_duration = 0;
UINT8 cap_sense_input = 0;

UINT16 led_value = 0;
UINT16 led_repeat_ctr = 0;
UINT8 led_pwm_color = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/

// Application initialization
APPLICATION_INIT()
{
    bleapp_set_cfg((UINT8 *)ame_gatt_database,
                   sizeof(ame_gatt_database),
                   (void *)&ame_cfg,
                   (void *)&ame_puart_cfg,
                   (void *)&ame_gpio_cfg,
                   ame_create);

//    BLE_APP_DISABLE_TRACING();     ////// Uncomment to disable all tracing
}

#define KOSHIAN02_NUMBER_OF_GPIO    (0x06)
const UINT8 koshian02_gpio_table[KOSHIAN02_NUMBER_OF_GPIO] = {
    26, 27, 28, 25, 4, 24
};
struct {
    UINT8 setting;
    UINT8 pullup;
    UINT8 output;
    UINT8 input;
} gpio;


// Create hello sensor
void ame_create(void)
{
    BLEPROFILE_DB_PDU db_pdu;

	extern UINT32 blecm_configFlag ;
	blecm_configFlag &= ~BLECM_DBGUART_LOG & ~BLECM_DBGUART_LOG_L2CAP & ~BLECM_DBGUART_LOG_SMP;

    ble_trace0("ame_create()");
    ble_trace0(bleprofile_p_cfg->ver);

    // dump the database to debug uart.
    legattdb_dumpDb();

    bleprofile_Init(bleprofile_p_cfg);
    bleprofile_GPIOInit(bleprofile_gpio_p_cfg);

    // register connection up and connection down handler.
    bleprofile_regAppEvtHandler(BLECM_APP_EVT_LINK_UP, ame_connection_up);
    bleprofile_regAppEvtHandler(BLECM_APP_EVT_LINK_DOWN, ame_connection_down);
    bleprofile_regAppEvtHandler(BLECM_APP_EVT_ADV_TIMEOUT, ame_advertisement_stopped);

    // register to process client writes
    legattdb_regWriteHandleCb((LEGATTDB_WRITE_CB)ame_write_handler);

    bleprofile_regTimerCb(ame_fine_timeout, ame_timeout);
    bleprofile_StartTimer();

    // Read value of the service from GATT DB.
    bleprofile_ReadHandle(HANDLE_AME_SERVICE_UUID, &db_pdu);
    ble_tracen((char *)db_pdu.pdu, db_pdu.len);

    if (db_pdu.len != 16)
    {
        ble_trace1("ame bad service UUID len: %d\n", db_pdu.len);
    }
    else
    {
    	// total length should be less than 31 bytes
    	BLE_ADV_FIELD adv[3];

        // flags
        adv[0].len     = 1 + 1;
        adv[0].val     = ADV_FLAGS;
        adv[0].data[0] = LE_LIMITED_DISCOVERABLE | BR_EDR_NOT_SUPPORTED;

        adv[1].len     = 16 + 1;
        adv[1].val     = ADV_SERVICE_UUID128_COMP;
        memcpy(adv[1].data, db_pdu.pdu, 16);

        // name
        adv[2].len      = strlen(bleprofile_p_cfg->local_name) + 1;
        adv[2].val      = ADV_LOCAL_NAME_COMP;
        memcpy(adv[2].data, bleprofile_p_cfg->local_name, adv[2].len - 1);

        bleprofile_GenerateADVData(adv, 3);
    }
    blecm_setTxPowerInADV(-5);

    // enable my timer
    bt_clock_based_periodic_timer_Init();

    // enable button
    UINT8 p = PIN_SWITCH;
    UINT16 interrupt_handler_mask[0x03] = {0x00, 0x00, 0x00};
    interrupt_handler_mask[p >> 0x04] |= 1 << (p & 0x0f);
    gpio_registerForInterrupt(interrupt_handler_mask, ame_gpio_interrupt_handler, 0);
    gpio_configurePin(PIO2 >> 0x04, PIO2 & 0x0f, GPIO_OUTPUT_ENABLE, GPIO_PIN_OUTPUT_LOW);
    gpio_configurePin(p >> 0x04, p & 0x0f, GPIO_INPUT_ENABLE | GPIO_PULL_DOWN | GPIO_EN_INT_BOTH_EDGE, GPIO_PIN_OUTPUT_LOW);
}

UINT8 is_connected = 0;

// This function will be called on every connection establishmen
void ame_connection_up(void)
{
    LOG_I("-s");
    UINT8 writtenbyte;
    UINT8 *bda;

    ame_connection_handle = (UINT16)emconinfo_getConnHandle();

    // save address of the connected device and print it out.
    memcpy(ame_remote_addr, (UINT8 *)emconninfo_getPeerPubAddr(), sizeof(ame_remote_addr));

    ble_trace5("ame_connection_up: %08x%04x type %d bonded %d handle %04x\n",
                (ame_remote_addr[5] << 24) + (ame_remote_addr[4] << 16) +
                (ame_remote_addr[3] << 8) + ame_remote_addr[2],
                (ame_remote_addr[1] << 8) + ame_remote_addr[0],
                emconninfo_getPeerAddrType(), emconninfo_deviceBonded(), ame_connection_handle);

    // Stop advertising
    bleprofile_Discoverable(NO_DISCOVERABLE, NULL);

    bleprofile_StopConnIdleTimer();

    is_connected = 1;
}

// This function will be called when connection goes down
void ame_connection_down(void)
{
    LOG_I("-s");
    ble_trace1("handle:%d\n", ame_connection_handle);

	memset (ame_remote_addr, 0, 6);
	ame_connection_handle = 0;

    // restart adv
    bleprofile_Discoverable(LOW_UNDIRECTED_DISCOVERABLE, ame_hostinfo.bdaddr);
}

void ame_advertisement_stopped(void)
{
    LOG_I("-s");
}


void led_init(void) {
    LOG_I("-s");
    gpio_configurePin(POUT_LEDR/16, POUT_LEDR%16, PWM0_OUTPUT_ENABLE_P26, 0);
    gpio_configurePin(POUT_LEDG/16, POUT_LEDG%16, PWM1_OUTPUT_ENABLE_P27, 0);
    gpio_configurePin(POUT_LEDB/16, POUT_LEDB%16, PWM2_OUTPUT_ENABLE_P28, 0);
}

void led_start(UINT8 color) {
	led_pwm_color = color;
	pwm_start(led_pwm_color, LHL_CLK, 1024 , 0);

	// start timer (20msec periodic)
    bt_clock_based_periodic_timer_Enable(ame_timer_expired_callback,
        NULL, 20000/625);
}

int ame_timer_expired_callback(void* context)
{
    UINT16 cnt_inc = 1024;
    UINT16 cnt_keep= 1025;
    UINT16 cnt_dec = 1024;
    UINT16 cnt_sum = cnt_inc + cnt_keep + cnt_dec;
    UINT16 speed = 50;
    UINT8 repeat = 5;

    led_value+=speed;

    if (led_value < cnt_inc) {
        pwm_transitionToSubstituteValues(led_pwm_color, cnt_inc-led_value , 0);
    } else if (led_value < cnt_inc + cnt_keep) {
        pwm_transitionToSubstituteValues(led_pwm_color, 0 , 0);
    } else if (led_value < cnt_sum){
        pwm_transitionToSubstituteValues(led_pwm_color, led_value-(cnt_inc+cnt_keep) , 0);
    } else {
        led_repeat_ctr++;
        led_value = 0;
        pwm_transitionToSubstituteValues(led_pwm_color, 2048 , 0);
    }

    if (led_repeat_ctr >= repeat) {
    	// stop timer
        pwm_transitionToSubstituteValues(led_pwm_color, 0x03ff , 0);
        led_value = 0;
        led_repeat_ctr = 0;
        bt_clock_based_periodic_timer_Disable();
    }
    // Context was not allocated and so does not need to be freed. So return no action.
    return BLE_APP_EVENT_NO_ACTION;
}

void ame_timeout(UINT32 arg)
{
    LOG_I("-s");
    ble_trace3("openrt_timeout:%d,adv:%d, scn:%d",
        ame_timer_count,
        bleprofile_GetDiscoverable(),
        blecen_GetScan());

    if(ame_timer_count == 3 ) {
        led_init();
    }

    ame_timer_count++;
}

void ame_fine_timeout(UINT32 arg)
{
}

//
// Process write request or command from peer device
//
int ame_write_handler(LEGATTDB_ENTRY_HDR *p)
{
    LOG_I("-s");
    UINT8  writtenbyte;
    UINT16 handle   = legattdb_getHandle(p);
    int    len      = legattdb_getAttrValueLen(p);
    UINT8  *attrPtr = legattdb_getAttrValue(p);

    // update color
    UINT8 color = attrPtr[0];
    switch (color) {
    case BLE_COMMAND_R:
    	led_start(LED_PWM_R);
    	break;
    case BLE_COMMAND_G:
    	led_start(LED_PWM_G);
    	break;
    case BLE_COMMAND_B:
    	led_start(LED_PWM_B);
    	break;
    }

    return 0;
}

static void ame_gpio_interrupt_handler(void *parameter, UINT8 arg) {
     LOG_I("-s");

    UINT8 pin = (UINT8)(UINT32)parameter;
    UINT8 p = PIN_SWITCH;
    gpio_clearPinInterruptStatus(p >> 0x04, p & 0x0f);

    UINT8 input;
    if (gpio_getPinInput(p >> 0x04, p & 0x0f)) {
        input |= 0x01 << pin;
    } else {
        input &= ~(0x01 << pin);
    }
    ble_trace2("[GPIO Interrupt %02x] input: 0x%02x\n", pin, input);

    if (input > 0) {
    	// button up
    } else {
    	// button down
        ble_trace0("\nstart advertisement\n");
        bleprofile_Discoverable(HIGH_UNDIRECTED_DISCOVERABLE, NULL);
    }

}


// following should go to bleprofile.c

// Start connection idle timer if it is not running
void bleprofile_StartConnIdleTimer(UINT8 timeout, BLEAPP_TIMER_CB cb)
{
    LOG_I("-s");
    if(emconinfo_getAppTimerId() < 0)
    {
        emconinfo_setIdleConnTimeout(timeout);
        blecm_startConnIdleTimer(cb);
        ble_trace1("profile:idletimer(%d)", timeout);
    }
}

// Stop connection idle timer if it is running
void bleprofile_StopConnIdleTimer(void)
{
    LOG_I("-s");
    if(emconinfo_getAppTimerId() >= 0)
    {
        blecm_stopConnIdleTimer();
        emconinfo_setAppTimerId(-1);
        ble_trace0("profile:idletimer stopped");
    }
}

// Send request to client to update connection parameters
void bleprofile_SendConnParamUpdateReq(UINT16 minInterval, UINT16 maxInterval, UINT16 slaveLatency, UINT16 timeout)
{
    LOG_I("-s");
    if (minInterval > maxInterval)
        return;

    lel2cap_sendConnParamUpdateReq(minInterval, maxInterval, slaveLatency, timeout);
}
