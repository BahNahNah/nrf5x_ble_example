#pragma once

#include <stdint.h>
#include "ble_srv_common.h"

#ifdef __cplusplus
extern "C" {
#endif
	

/*
 * bt services
 * https://devzone.nordicsemi.com/nordic/short-range-guides/b/bluetooth-low-energy/posts/ble-services-a-beginners-tutorial
 **/

//makes it easier to impliment imo
typedef enum char_props {
	CH_BROADCAST		= 1 << 0,
	CH_READ				= 1 << 1,
	CH_WR_WO_RESP		= 1 << 2,
	CH_WRITE			= 1 << 3,
	CH_NOTIFY			= 1 << 4,
	CH_INDICATE			= 1 << 5,
	CH_AUTH_SIGNED_WR	= 1 << 6,
	CH_PROP_COUNT,
}char_props_e;


#define CHFLAG(FLAG) ((char_props_e)(FLAG))


uint8_t services_add_uuid(uint8_t uuid128[16]);
uint16_t services_add_service(uint8_t uuid_type, uint16_t uuid);
ble_gatts_char_handles_t services_add_characteristic(uint8_t uuid_type, uint16_t uuid, uint16_t parent_service_handle, char_props_e props);

		
#ifdef __cplusplus
}
#endif