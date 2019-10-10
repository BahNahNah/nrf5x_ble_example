#include "uart.h"

#include "nrf_log.h"

static nrfx_uart_t uart = NRFX_UART_INSTANCE(0);
static uint16_t tx_total_queued = 0;
static uint16_t tx_sent = 0;
static uint8_t tx_buffer[500];
static bool tx_in_progress = false;

static void uart_default_event_handler(nrfx_uart_event_t const * p_event, void * p_context) {
	
	switch (p_event->type) {
	case NRFX_UART_EVT_TX_DONE: {
		NRF_LOG_INFO("Uart sent %i bytes.", p_event->data.rxtx.bytes);
		
		/*
		 * handling the queued up writes.
		 **/
		tx_in_progress = false;
		tx_sent += p_event->data.rxtx.bytes;
		if (tx_sent < tx_total_queued) {
			uart_write_queue_flush();
		} else {
			//reset buffer
			tx_sent = 0;
			tx_total_queued = 0;
		}
	}break;
	case NRFX_UART_EVT_RX_DONE: {
		NRF_LOG_INFO("Uart recieved %i bytes.", p_event->data.rxtx.bytes);
	}break;
	case NRFX_UART_EVT_ERROR: {
		NRF_LOG_ERROR("Uart error: %i", p_event->type);
	}break;
	}
	
}

void uart_init(nrfx_uart_config_t *config, nrfx_uart_event_handler_t _evt_handler) {
	nrfx_uart_event_handler_t evt_hndl = _evt_handler;
	if (!evt_hndl) {
		/*
		 *	just default to logging if
		 *	no event handler was set
		 **/
		evt_hndl = uart_default_event_handler;
	}
	
	uint32_t err = nrfx_uart_init(&uart, config, evt_hndl);
	APP_ERROR_CHECK(err);
	nrfx_uart_rx_enable(&uart);
}

/*
 *	the buffer must live up to 
 *	the NRFX_UART_EVT_RX_DONE
 *	event has triggered.
 *
 **/
void uart_read(uint8_t * rx_buffer, uint16_t bytes_to_read) {
	ret_code_t err = nrfx_uart_rx(&uart, rx_buffer, bytes_to_read);
	APP_ERROR_CHECK(err);
}



/*
 *
 *	the data must live up to 
 *	the NRFX_UART_EVT_TX_DONE
 *	event has triggered.
 *	
 **/
void uart_write(uint8_t *data, uint16_t len) {
	uint32_t err;
	
	/*
	 *Not  a recommended approach.
	 *looping means the device can not enter sleep.
	 *
	 *My recommendation is to write into a internal buffer
	 *instead of directly to uart. then, on TX_DONE, read
	 *off the next part of the write buffer.
	 * (look at uart_write_queue for example)
	 **/
	do {
		err = nrfx_uart_tx(&uart, data, len);
		if (err == NRF_ERROR_BUSY) {
			NRF_LOG_WARNING("Uart send returned NRF_ERROR_BUSY. Wait for NRFX_UART_EVT_TX_DONE before sending next packet!");
		}
	} while (err == NRF_ERROR_BUSY);
	
	APP_ERROR_CHECK(err);
}

/*
 *uart_init must have been called with a NULL callback for this to work.
 *requires the uart callback that is defined at the top of this file.
 *
 *queues uart writes and resets when the buffer has been fully sent.
 *this is not optimal as it requires some downtime for the
 *buffer to be reset.
 *
 *a better approach would be a circular buffer.
 **/
void uart_write_queued(uint8_t *data, uint16_t len) {
	uart_write_queue_append(data, len);
	uart_write_queue_flush();
}

/*
 *Write to the uart tx queue but dont atcualy send
 **/
void uart_write_queue_append(uint8_t *data, uint16_t len) {
	memcpy(&tx_buffer[tx_total_queued], data, len);
	tx_total_queued += len;
}

/*
 *send whats in the tx queue
 **/
void uart_write_queue_flush() {
	uint16_t bytes_remaning =  tx_total_queued - tx_sent;
	if (tx_in_progress || !bytes_remaning) {
		return;
	}
	tx_in_progress = true;
	uart_write(&tx_buffer[tx_sent], bytes_remaning);
}


