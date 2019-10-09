#include "device.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nordic_common.h"
#include "nrf_sdh.h"
#include "nrf_pwr_mgmt.h"
#include "app_timer.h"



//main event loop
void device_run() {
	for (;;) {
		if (NRF_LOG_PROCESS() == false) {
			nrf_pwr_mgmt_run(); //puts device to sleep when idle. saves power.
		}
	}
}


static void device_init_logging() {
	ret_code_t err_code = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(err_code);

	NRF_LOG_DEFAULT_BACKENDS_INIT();
}
static void device_init_pwr_management() {
	ret_code_t err_code;
	err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);
}

static void device_init_timers() {
	ret_code_t err_code = app_timer_init();
	APP_ERROR_CHECK(err_code);
}
void device_init() {
	device_init_logging();
	device_init_pwr_management();
	device_init_timers();
}
