#include "bluetooth.h"

#include "nordic_common.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrfx.h"
#include "nrf_log.h"

#include <string.h>

#define APP_BLE_CONN_CFG_TAG 1


/*
 * Central		- Scanning device
 * Peripheral	- Advertising device
 *
 *
 *
 **/

//enables the softdevice and the ble events in the background
void bluetooth_init() {
	ret_code_t err_code;

	err_code = nrf_sdh_enable_request();
	APP_ERROR_CHECK(err_code);

	// Configure the BLE stack using the default settings.
	// Fetch the start address of the application RAM.
	uint32_t ram_start = 0;
	err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
	APP_ERROR_CHECK(err_code);

	// Enable BLE stack.
	err_code = nrf_sdh_ble_enable(&ram_start);
	APP_ERROR_CHECK(err_code);
}

//Set the device name. 
void bluetooth_set_name(const char* name) {
	uint32_t      err_code;
	ble_gap_conn_sec_mode_t sec_mode;

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
	err_code = sd_ble_gap_device_name_set( &sec_mode,(const uint8_t *)name,strlen(name));
	APP_ERROR_CHECK(err_code);
}

// MAX tx power fotrr nrf52832 is 4db
// This is for scanning power
void bluetooth_set_scan_power(int8_t power) {
	sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_SCAN_INIT, 0, power);
}

//peripheral -> central communication
//notifications must be enabled on the characteristic
void bluetooth_notify(uint16_t conn, uint16_t handle, void* data, uint16_t data_len) {
	uint16_t v_size = data_len;
	
	ble_gatts_hvx_params_t hvx_params;
	
	memset(&hvx_params, 0, sizeof(hvx_params));
	
	hvx_params.handle = handle;
	hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
	hvx_params.offset = 0;
	hvx_params.p_len  = &v_size;
	hvx_params.p_data = (uint8_t*)data;

	uint32_t err_code = sd_ble_gatts_hvx(conn, &hvx_params);

	if (err_code) {
		NRF_LOG_INFO("bluetooth_notify error %i conn %i", err_code, conn);
	}
	APP_ERROR_CHECK(err_code);
}

//central -> periferal communication
void bluetooth_write(uint16_t conn, uint16_t handle, void* data, uint16_t data_len, uint8_t write_command) {
	uint32_t err_code;	
	ble_gattc_write_params_t write_params;		
	
	write_params.write_op = write_command;       //BLE_GATT_OP_WRITE_CMD BLE_GATT_OP_WRITE_REQ
	write_params.handle   = handle;			
	write_params.offset   = 0;		
	write_params.p_value  = (uint8_t*)data;		
	write_params.len      = data_len;															
	
	err_code = sd_ble_gattc_write(conn, &write_params);
	if (err_code) {
		NRF_LOG_ERROR("bluetooth_write error %i conn %i", err_code, conn);
	}
	
	APP_ERROR_CHECK(err_code);
}

//Event when sent (UDP-Like)
void bluetooth_write_command(uint16_t conn, uint16_t handle, void* data, uint16_t data_len) {
	bluetooth_write(conn, handle, data, data_len, BLE_GATT_OP_WRITE_CMD);
}

//Event on conformation of write (TCP-Like)
void bluetooth_write_request(uint16_t conn, uint16_t handle, void* data, uint16_t data_len) {
	bluetooth_write(conn, handle, data, data_len, BLE_GATT_OP_WRITE_REQ);
}
