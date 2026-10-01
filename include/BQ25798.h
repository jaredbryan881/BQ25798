#ifndef BQ25798_H
#define BQ25798_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BQ25798_I2C_ADDRESS 0x6BU
#define BQ25798_PART_NUMBER 3U
#define BQ25798_REG_PART_INFORMATION 0x48U
#define BQ25798_REG_STATUS_0 0x1BU
#define BQ25798_REG_FAULT_STATUS_0 0x20U
#define BQ25798_CONFIGURATION_LENGTH 0x1BU

typedef enum {
	BQ25798_OK = 0,
	BQ25798_NULL_ARGUMENT,
	BQ25798_INVALID_REGISTER,
	BQ25798_IO_ERROR,
	BQ25798_WRONG_DEVICE
} bq25798_result_t;

// Perform a register-address write followed by a repeated START and a read.
typedef int32_t (*bq25798_read_fn)(void *context, 
								   uint8_t address, 
								   uint8_t reg, 
								   uint8_t *data, 
								   uint16_t length);

typedef struct {
	bq25798_read_fn read;
	void *context;
	int32_t last_io_error;
} bq25798_t;

typedef struct {
	uint8_t raw;
	uint8_t part_number;
	uint8_t revision;
} bq25798_identity_t;

typedef struct {
	uint8_t raw[7];
	bool input_current_regulation;
	bool input_voltage_regulation;
	bool watchdog_expired;
	bool power_good;
	bool ac2_present;
	bool ac1_present;
	bool vbus_present;
	uint8_t charge_state;
	uint8_t input_type;
	bool detection_complete;
	uint8_t ico_state;
	bool thermal_regulation;
	bool detection_running;
	bool vbat_above_uvlo;
	bool input_fets2_detected;
	bool input_fets1_detected;
	bool adc_done;
	bool minimum_system_regulation;
	uint8_t expired_charge_timers;
	uint8_t temperature_status;
	uint8_t fault_status_0;
	uint8_t fault_status_1;
	bool protection_active;
} bq25798_status_t;

typedef struct {
	uint8_t raw[BQ25798_CONFIGURATION_LENGTH];
	uint8_t cell_count;
	uint16_t minimum_system_mv;
	uint16_t charge_voltage_mv;
	uint16_t charge_current_ma;
	uint16_t input_voltage_limit_mv;
	uint16_t input_current_limit_ma;
} bq25798_configuration_t;

// Initialize the driver (does not actually initialize the charger)
bq25798_result_t bq25798_init(bq25798_t *device, bq25798_read_fn read, void *context);
bq25798_result_t bq25798_read_u8(bq25798_t *device, uint8_t reg, uint8_t *value);
bq25798_result_t bq25798_read_u16(bq25798_t *device, uint8_t reg, uint16_t *value);
bq25798_result_t bq25798_read_identity(bq25798_t *device, bq25798_identity_t *identity);
bq25798_result_t bq25798_read_status(bq25798_t *device, bq25798_status_t *status);
bq25798_result_t bq25798_read_configuration(bq25798_t *device, bq25798_configuration_t *configuration);

#ifdef __cplusplus
}
#endif
#endif
