#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


void bluetooth_init();
void bluetooth_set_name(const char* name);
void bluetooth_set_scan_power(int8_t power);

void bluetooth_notify(uint16_t conn, uint16_t handle, void* data, uint16_t data_len);
void bluetooth_write(uint16_t conn, uint16_t handle, void* data, uint16_t data_len, uint8_t write_command);
void bluetooth_write_command(uint16_t conn, uint16_t handle, void* data, uint16_t data_len);
void bluetooth_write_request(uint16_t conn, uint16_t handle, void* data, uint16_t data_len);


	
#ifdef __cplusplus
}
#endif