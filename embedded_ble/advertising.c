#include "advertising.h"

#include "ble_advdata.h"
#include "ble_advertising.h"
#include "nrf_sdh_ble.h"

#include "nrfx.h"

#define COMPANY_ID 0x6969


static uint8_t adv_data_buffer[BLE_GAP_ADV_SET_DATA_SIZE_MAX]; 
static ble_gap_adv_data_t encoded_adv_data;


/* Normally you wold just call
 * BLE_ADVERTISING_DEF(adv)
 * it would do some stuff in the background to handle the events, 
 * but its using c-style struct layout so its not great for c++ projects.
 * 
 * insted just register the observer manualy and dispatch
 * to the sdk-callback anyway.
 * 
 * NRF_SDH_BLE_OBSERVER is in 'nrf_sdh_ble.h'
 **/
//

//advertisement module instance.
static ble_advertising_t adv;

/*this observer really isnt nessisary. 
 *it basicly just restarts advertising on disconnect
 */

// NRF_SDH_BLE_OBSERVER(adv_ble_obs, BLE_ADV_BLE_OBSERVER_PRIO, ble_advertising_on_ble_evt, &adv);




void advertising_init() {
	//reset adv state
	memset(&adv, 0, sizeof(adv)); 
	
	//advertisement needs to be encoded. adv_data_buffer is where its going to be stored.
	memset(&encoded_adv_data, 0, sizeof(encoded_adv_data));
	encoded_adv_data.adv_data.p_data = adv_data_buffer;
	encoded_adv_data.adv_data.len = BLE_GAP_ADV_SET_DATA_SIZE_MAX;
	
	ble_advdata_t adv_data;
	memset(&adv_data, 0, sizeof(adv_data));
	adv_data.name_type				= BLE_ADVDATA_FULL_NAME; //include device name in advertisement
	adv_data.flags					= BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
	adv_data.include_appearance		= false;
	
	
	// set this at some point if you want data in the adv packet.
	ble_advdata_manuf_data_t manuf_specific_data;
	memset(&manuf_specific_data, 0, sizeof(manuf_specific_data));
	manuf_specific_data.data.p_data = 0;
	manuf_specific_data.data.size   = 0;
	manuf_specific_data.company_identifier = COMPANY_ID;
	
	
	adv_data.p_manuf_specific_data = &manuf_specific_data;
	
	ble_advertising_init_t init;
	memset(&init, 0, sizeof(init));
	init.advdata = adv_data;
	init.config.ble_adv_fast_enabled  = true;
	init.config.ble_adv_fast_interval = 400;
	init.config.ble_adv_fast_timeout  = BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED;
	
	uint32_t err = ble_advertising_init(&adv, &init);
	APP_ERROR_CHECK(err);
	ble_advertising_conn_cfg_tag_set(&adv, 1);
 }


void advertising_start() {
	ret_code_t err_code;
	err_code = ble_advertising_start(&adv, BLE_ADV_MODE_FAST);
	APP_ERROR_CHECK(err_code);
}
void advertising_stop() {
	ret_code_t err_code = sd_ble_gap_adv_stop(adv.adv_handle); 
	if (err_code != NRF_ERROR_INVALID_STATE) { //invalid state is retuned if the device isnt advertising. So dosent matter.
		APP_ERROR_CHECK(err_code);
	}
}