#include "scanner.h"

#include "nrf_ble_scan.h"
#include "nrf_sdh_ble.h"
#include "nrfx.h"
#include "nrf_log.h"
#include <string.h>

#define MIN_CONNECTION_INTERVAL   MSEC_TO_UNITS(200, UNIT_1_25_MS)      /**< Determines minimum connection interval in milliseconds. */
#define MAX_CONNECTION_INTERVAL   MSEC_TO_UNITS(300, UNIT_1_25_MS)       /**< Determines maximum connection interval in milliseconds. */
#define SLAVE_LATENCY             0                                     /**< Determines slave latency in terms of connection events. */
#define SUPERVISION_TIMEOUT       MSEC_TO_UNITS(1500, UNIT_10_MS)       /**< Determines supervision time-out in units of 10 milliseconds. */


static nrf_ble_scan_t scan;
static const char* target_name = 0;
void scan_ble_event_handler(ble_evt_t const * p_evt, void * p_context);


typedef struct adv_data_segement {
	uint8_t type;
	const uint8_t *data;
	uint16_t len;
}adv_data_segement_t;

/*
 * This function parses the advertisement
 *	so we can read stuff like 
 *	flags, custom manufature data, and name
 *	
 *	returns true if found.
 **/
static bool get_adv_segment(const ble_gap_evt_adv_report_t * adv_report, uint8_t segment_search, adv_data_segement_t* segment_out) {
	const ble_data_t * adv_data = &adv_report->data;
	const uint8_t * dptr = adv_data->p_data;
	
	for (uint16_t i = 0; i < adv_data->len; i++) {
		uint8_t segment_len		= dptr[i];
		uint8_t segment_type	= dptr[i + 1];
		if (segment_type == segment_search) {
			segment_out->type = segment_type;
			segment_out->len = segment_len;
			segment_out->data = &dptr[i + 2];
			return true;
		}
		i += segment_len;
	}
	return false;
}

void scanner_init() {
	nrf_ble_scan_init_t init;
	memset(&init, 0, sizeof(init));
	
	ble_gap_conn_params_t connection_param;
	connection_param.conn_sup_timeout = SUPERVISION_TIMEOUT;
	connection_param.min_conn_interval = MIN_CONNECTION_INTERVAL;
	connection_param.max_conn_interval = MAX_CONNECTION_INTERVAL;
	connection_param.slave_latency = SLAVE_LATENCY;

	ble_gap_scan_params_t scanparams;
	memset(&scanparams, 0, sizeof(scanparams));
	scanparams.active = 0;
	scanparams.interval =  MSEC_TO_UNITS(250, UNIT_0_625_MS);  
	scanparams.window =  MSEC_TO_UNITS(50, UNIT_0_625_MS);
	scanparams.timeout = 0; 
	
	init.p_conn_param = &connection_param;
	init.p_scan_param = &scanparams;

	uint32_t err = nrf_ble_scan_init(&scan, &init, NULL);
	APP_ERROR_CHECK(err);
	
	NRF_SDH_BLE_OBSERVER(san_obs, BLE_ADV_BLE_OBSERVER_PRIO, scan_ble_event_handler, NULL);
}

// Stop looking for device.
void scanner_stop_scanning() {
	nrf_ble_scan_stop();
}


/*
 *Scan endlessly untill a device with the given name is found.
 **/
void scanner_connect_to_device(const char* name) {
	if (!name) {
		NRF_LOG_ERROR("Null name passed into scanner_connect_to_device");
		return;
	}
	
	target_name = name;
	
	/*
	 * the scan behavior parameters where defined in the scanner_init scanparams struct.
	 *	timeout is set to 0 so it will scan indenfinitely until it finds the device.
	 **/
	nrf_ble_scan_start(&scan);
}

void scanner_connect_to_addr(const ble_gap_addr_t *peer_addr) {
	/*
	*These params are only for connection.
	*When sd_ble_gap_connect is called, internaly the
	*device will look for another advertisement to connect
	*to, that has the same peer_addres that is passed in.
	*
	*These parameters are for that 
	*internal, 'attempting to connect' 
	**/
	ble_gap_scan_params_t connection_scanParams;
	memset(&connection_scanParams, 0, sizeof(connection_scanParams));
	connection_scanParams.active = 0;
	connection_scanParams.interval = MSEC_TO_UNITS(150, UNIT_0_625_MS);
	connection_scanParams.window =   MSEC_TO_UNITS(50, UNIT_0_625_MS);
	connection_scanParams.timeout = 500;
	
	uint32_t err = sd_ble_gap_connect(peer_addr, &connection_scanParams, &scan.conn_params, 1); 
	APP_ERROR_CHECK(err);
}

void scan_ble_event_handler(ble_evt_t const * p_evt, void * p_context) {
	/*
	 *dispatch the events to the
	 *scanner sdk.
	 *
	 *This could go directly on the observers
	 *callback and we could get callbacks from the 
	 *	nrf_ble_scan_t (scan) .evt_handler
	 *	but I prefer to access the bluetooth events directly.
	 *
	 **/
	nrf_ble_scan_on_ble_evt(p_evt, &scan);
	
	if (p_evt->header.evt_id == BLE_GAP_EVT_TIMEOUT) {
		// Scan/Connection timeout event.
		const ble_gap_evt_timeout_t * params = &p_evt->evt.gap_evt.params.timeout;
		if (params->src == BLE_GAP_TIMEOUT_SRC_CONN) {
			// Timeout while connecting
			NRF_LOG_ERROR("Connection timeout. Rerstarting scan...");
			nrf_ble_scan_start(&scan);
		} else if (params->src == BLE_GAP_TIMEOUT_SRC_SCAN) {
			/*
			 *	Timeout from scan.
			 *	
			 *	If in scan_init we had set 
			 *	scanparams.timeout  to a non-zero
			 *	value, this event would be triggered
			 *	when that timeout elapsed.
			 *	
			 *	This event is not expected in this 
			 *	applicaton as 0 timeout will never
			 *	timeout.
			 **/
		}
	}
	
	/*
	 *we only care about advertisement reports here.
	 *
	 **/
	if (p_evt->header.evt_id != BLE_GAP_EVT_ADV_REPORT) {
		return;
	}
	
	const ble_gap_evt_adv_report_t*  adv_report  =  &p_evt->evt.gap_evt.params.adv_report;
	
	//extract name from advertisement
	adv_data_segement_t name_segment;
	if (!get_adv_segment(adv_report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, &name_segment) &&
		get_adv_segment(adv_report, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, &name_segment)) {
			// Neither short or complete name exists. So cant be the device.
			return;
	}
	
	if (name_segment.len != strlen(target_name) || strncmp((const char *)name_segment.data, target_name, name_segment.len) != 0) {
		//names dont match. Exit
		return;
	}
	
	//Found device. Connect
	scanner_connect_to_addr(&adv_report->peer_addr);
}