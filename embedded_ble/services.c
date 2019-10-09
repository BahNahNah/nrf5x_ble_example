#include "services.h"


#include "nrfx.h"
#include <string.h>
#include "nrf_log.h"

/*
 * set NRF_SDH_BLE_VS_UUID_COUNT
 * in sdk_config to however
 * manu UUIDs you wana add
 **/

//defined at the bottom coz its kinda big.
static ble_gatt_char_props_t props_from_flags(char_props_e props);

/*
 * ndk takes a 128bit uuid 
 * and "shrinks" it to just 
 * a uint8_t type identifier
 * so it can be used other places
 * more efficciently.
 **/
uint8_t services_add_uuid(uint8_t uuid128[16]) {
	
	ble_uuid128_t uuid_base;
	memcpy(uuid_base.uuid128, uuid128, sizeof(uuid_base.uuid128));
	
	uint8_t type;
	uint32_t err = sd_ble_uuid_vs_add(&uuid_base, &type);
	APP_ERROR_CHECK(err);
	return type;
}

//register service so it can be discovered and displayed on other devices.
uint16_t services_add_service(uint8_t uuid_type, uint16_t uuid) {
		
	ble_uuid_t full_uuid;
	full_uuid.type = uuid_type;
	full_uuid.uuid = uuid;
	
	uint16_t handle;
	uint32_t err_no = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &full_uuid, &handle);
	APP_ERROR_CHECK(err_no);
	NRF_LOG_INFO("+service (uuid 0x%x)", uuid);
	
	return handle;
}

//Add a characteristic to a service for data communication.
ble_gatts_char_handles_t services_add_characteristic(uint8_t uuid_type, uint16_t uuid, uint16_t parent_service_handle, char_props_e props) {
	const uint16_t max_length = 55;
	
	ble_add_char_params_t params;
	memset(&params, 0, sizeof(params));
	
	params.uuid_type = uuid_type;
	params.uuid = uuid;
	
	params.is_var_len = 1;
	params.read_access = SEC_OPEN;
	params.write_access = SEC_OPEN;
	params.cccd_write_access = SEC_OPEN;
	params.init_len = max_length;
	params.max_len =  max_length;
	params.p_init_value = 0;
	
	params.char_props = props_from_flags(props);

	ble_gatts_char_handles_t handles;
	uint32_t err_no = characteristic_add(parent_service_handle, &params, &handles);
	
	NRF_LOG_INFO("+char (0x%x) <value_handle: 0x%x|cccd_handle: 0x%x>", uuid, handles.value_handle, handles.cccd_handle);
	
	APP_ERROR_CHECK(err_no);
	return handles;
}



// converts the bitflag (char_props_e)
// into the atcual struct (ble_gatt_char_props_t)
// needed by the sdk
 ble_gatt_char_props_t props_from_flags(char_props_e props) {
	ble_gatt_char_props_t chprop;
	memset(&chprop, 0, sizeof(chprop));
	
	uint16_t ch_count = (((uint32_t)CH_PROP_COUNT) - 1) / 8;
	
	for (int i = 0; i < CH_PROP_COUNT; i++) {
		char_props_e p = CHFLAG(1 << i);
		
		if (!(props & p)) {
			continue;
		}
		switch (p) {
		case CH_BROADCAST:
			chprop.broadcast = 1;
			break;
		case CH_READ:
			chprop.read = 1;
			break;
		case CH_WR_WO_RESP:
			chprop.write_wo_resp = 1;
			break;
		case CH_WRITE:
			chprop.write = 1;
			break;
		case CH_NOTIFY:
			chprop.notify = 1;
			break;
		case CH_INDICATE:
			chprop.indicate = 1;
			break;
		case CH_AUTH_SIGNED_WR:
			chprop.auth_signed_wr = 1;
			break;
		default:
			break;
		}
	
	}
	return chprop;
}
