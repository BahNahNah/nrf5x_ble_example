#include "nrfx.h"
#include "nrf_log.h"


#include "device.h"
#include "bluetooth.h"
#include "advertising.h"
#include "services.h"
#include "uart.h"

#include "ble.h"
#include "nrf_sdh_ble.h"

#include "nrf_ble_gatt.h"

#define SERVICE_UUID 0xFFFF
#define CHARACTERISTIC_UUID 0xAAAA

nrf_ble_gatt_t gatt;
ble_gatts_char_handles_t my_char;


void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
void add_services();
void setup_uart();

int main() {
	
	device_init();
	bluetooth_init();
	
	NRF_LOG_INFO("Device started.");
	bluetooth_set_name("samu_example");
	add_services();
	setup_uart();
	
	advertising_init();
	advertising_start();
	
	APP_ERROR_CHECK(nrf_ble_gatt_init(&gatt, NULL));
	
	//get ble events
	NRF_SDH_BLE_OBSERVER(main_obs, BLE_ADV_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
	
	device_run();
	
	return 0;
}

void setup_uart() {
	nrfx_uart_config_t config = NRFX_UART_DEFAULT_CONFIG;
	//pins for nina-b1 dev kit
	config.pseltxd = 6;
	config.pselrts = 8;
	config.pselcts = 18;
	config.pselrxd = 5;
	
	//pins for nrf51 dev kit (rigato?)
	/*
	config.pseltxd = 6;
	config.pselrts = 5;
	config.pselcts = 7;
	config.pselrxd = 8;
	*/
	uart_init(&config, NULL);
}

void send_data_to_uart(const uint8_t* data, uint16_t len) {
	//append length byte
	uint8_t len_byte = len;
	uart_write_queue(&len_byte, sizeof(len_byte));
	//then data
	uart_write_queue((uint8_t *)data, len);
}

void add_services() {
	//base uuid
	uint8_t uuid128[16] = { 0x71, 0x88, 0x44, 0x1f, 0x36, 0x92, 0x06, 0xb0, 0xe6, 0x11, 0xe3, 0x11, 0x77, 0x30, 0xd6, 0x11 };

	uint8_t uuid_type		= services_add_uuid(uuid128);
	uint16_t service_handle = services_add_service(uuid_type, SERVICE_UUID);
	my_char = services_add_characteristic(uuid_type, CHARACTERISTIC_UUID, service_handle, CHFLAG(CH_NOTIFY | CH_WRITE));
}


void ble_evt_handler(ble_evt_t const * p_evt, void * p_context) {

	/*
	 * gatt will automaticly repy
	 * to some very common and not
	 * usefull, but reuired events.
	 *  (like MTU exchange etc..)
	 *  
	 * this could go on its own observer but eh.
	 **/
	nrf_ble_gatt_on_ble_evt(p_evt, &gatt);
	
	uint16_t conn = p_evt->evt.common_evt.conn_handle;
	
	
	switch (p_evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED: {
		NRF_LOG_INFO("Device connected!");
	}break;
	case BLE_GAP_EVT_DISCONNECTED: {
		NRF_LOG_INFO("Device disconnected.");
		/*
		 *	because NRF_SDH_BLE_CENTRAL_LINK_COUNT is 1 (in sdk_config.h)
		 *	advertising will automaticly stop on any connection.
		 *	So restart connection when the link is free.
		 **/
		advertising_start();
	}break;
	case BLE_GATTS_EVT_WRITE: {
		const ble_gatts_evt_write_t *params = &p_evt->evt.gatts_evt.params.write;
		NRF_LOG_INFO("<%i> Recieved %i bytes on handle %i", conn, params->len, params->handle);
		NRF_LOG_HEXDUMP_INFO(params->data, params->len);
		//send over com
		send_data_to_uart(params->data, params->len);
		/*
		 *	notification enable/disable are 2 bytes
		 *	always on the cccd_handle.
		 *	
		 **/
		if (params->handle == my_char.cccd_handle && params->len) {
			if (params->data[0]) { 
				NRF_LOG_INFO("Notifications where enabled for the characeristic.");
			} else {
				NRF_LOG_INFO("Notifications where disabled for the characeristic.");
			}
		}
	}break;
	default:
		break;
	}
	
}

