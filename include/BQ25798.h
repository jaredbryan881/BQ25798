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
	BQ25798_WRONG_DEVICE,
	BQ25798_INVALID_ARGUMENT,
	BQ25798_WRITE_NOT_AVAILABLE,
	BQ25798_NOT_READY,
	BQ25798_BUSY,
	BQ25798_NOT_CONFIGURED,
	BQ25798_VERIFY_FAILED
} bq25798_result_t;

// Perform a register-address write followed by a repeated START and a read.
typedef int32_t (*bq25798_read_fn)(void *context, 
								   uint8_t address, 
								   uint8_t reg, 
								   uint8_t *data, 
								   uint16_t length);

// Write register-address and data bytes
typedef int32_t (*bq25798_write_fn)(void *context, 
									uint8_t address,
									uint8_t reg, 
									const uint8_t *data, 
									uint16_t length);

typedef struct {
	bq25798_read_fn read;
	void *context;
	int32_t last_io_error;
	bq25798_write_fn write;
	bool adc_configured;
	bool adc_one_shot_pending;
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
	uint16_t precharge_current_ma;
	uint16_t termination_current_ma;
	uint16_t recharge_offset_mv; // voltage below VREG
	uint8_t recharge_deglitch_code; // 0=64ms, 1=256ms, 2=1024ms, 3=2048ms
	uint8_t precharge_threshold_code; // 0=15%, 1=62.2%, 2=66.7%, 3=71.4% of VREG
	bool charging_enabled;
	bool termination_enabled;
	bool ico_enabled;
	bool high_impedance;
	bool input_current_limit_enabled;
	bool external_input_limit_enabled;
} bq25798_configuration_t;

// Register codes for CHG_TMR
typedef enum {
	BQ25798_CHARGE_TIMER_5_HOURS = 0,
	BQ25798_CHARGE_TIMER_8_HOURS = 1,
	BQ25798_CHARGE_TIMER_12_HOURS = 2,
	BQ25798_CHARGE_TIMER_24_HOURS = 3
} bq25798_charge_timer_t;

typedef struct {
	bq25798_charge_timer_t fast_charge_duration;
	uint8_t top_off_code; // 0=off, 1=15min, 2=30min, 3=45min
	bool trickle_enabled;
	bool precharge_enabled;
	bool fast_charge_enabled;
	bool double_during_regulation;
	bool precharge_half_hour; // false=2 hours, true=30 min
} bq25798_charge_timers_t;

typedef struct {
	uint8_t cell_count;
	uint16_t minimum_system_mv;
	uint16_t charge_voltage_mv;
	uint16_t charge_current_ma;
	uint16_t input_voltage_limit_mv;
	uint16_t input_current_limit_ma;
	uint16_t precharge_current_ma;
	uint16_t termination_current_ma;
	uint16_t recharge_offset_mv;
	uint8_t recharge_deglitch_code;
	bq25798_charge_timers_t timers;
	bool termination_enabled;
	bool charging_enabled;
} bq25798_charge_profile_t;

// Logical ADC channel bits follow the order of the telemetry register pairs.
#define BQ25798_ADC_IBUS  (1U << 0U)
#define BQ25798_ADC_IBAT  (1U << 1U)
#define BQ25798_ADC_VBUS  (1U << 2U)
#define BQ25798_ADC_VAC1  (1U << 3U)
#define BQ25798_ADC_VAC2  (1U << 4U)
#define BQ25798_ADC_VBAT  (1U << 5U)
#define BQ25798_ADC_VSYS  (1U << 6U)
#define BQ25798_ADC_TS    (1U << 7U)
#define BQ25798_ADC_TDIE  (1U << 8U)
#define BQ25798_ADC_DP    (1U << 9U)
#define BQ25798_ADC_DM    (1U << 10U)
#define BQ25798_ADC_ALL   0x07FFU
#define BQ25798_ADC_POWER 0x01FFU

typedef enum { 
	BQ25798_ADC_CONTINUOUS = 0,
	BQ25798_ADC_ONE_SHOT = 1
} bq25798_adc_mode_t;

typedef enum {
	BQ25798_ADC_15_BIT = 0,
	BQ25798_ADC_14_BIT = 1,
	BQ25798_ADC_13_BIT = 2
} bq25798_adc_resolution_t;

typedef struct {
	uint8_t raw_control;
	uint8_t raw_disable_0;
	uint8_t raw_disable_1;
	uint16_t channels;
	bq25798_adc_mode_t mode;
	bq25798_adc_resolution_t resolution;
	bool enabled;
	bool averaging;
} bq25798_adc_configuration_t;

typedef struct {
	uint8_t raw[22];
	uint16_t channels;
	bool completed_one_shot;
	bool ibat_discharge_sensing_enabled;
	int32_t ibus_ma; // negative reverse current
	int32_t ibat_ma; // charging positive, discharging negative
	uint16_t vbus_mv; 
	uint16_t vac1_mv;
	uint16_t vac2_mv;
	uint16_t vbat_mv;
	uint16_t vsys_mv;
	float ts_percent; // [% of REGN]
	int32_t die_temperature_mc; // [mC]
	uint16_t dp_mv, dm_mv;
} bq25798_adc_t;

// Initialize the driver (does not actually initialize the charger)
bq25798_result_t bq25798_init(bq25798_t *device, bq25798_read_fn read, void *context);

bq25798_result_t bq25798_read_u8(bq25798_t *device, uint8_t reg, uint8_t *value);
bq25798_result_t bq25798_read_u16(bq25798_t *device, uint8_t reg, uint16_t *value);
bq25798_result_t bq25798_read_identity(bq25798_t *device, bq25798_identity_t *identity);
bq25798_result_t bq25798_read_status(bq25798_t *device, bq25798_status_t *status);
bq25798_result_t bq25798_read_configuration(bq25798_t *device, bq25798_configuration_t *configuration);

bq25798_result_t bq25798_set_write_callback(bq25798_t *device, bq25798_write_fn write);
bq25798_result_t bq25798_set_watchdog(bq25798_t *device, uint8_t code);
bq25798_result_t bq25798_kick_watchdog(bq25798_t *device);
bq25798_result_t bq25798_set_ibat_discharge_sensing(bq25798_t *device, bool enabled);

bq25798_result_t bq25798_set_charge_voltage(bq25798_t *device, uint16_t mv);
bq25798_result_t bq25798_set_charge_current(bq25798_t *device, uint16_t ma);
bq25798_result_t bq25798_set_minimum_system_voltage(bq25798_t *device, uint16_t mv);
bq25798_result_t bq25798_set_input_voltage_limit(bq25798_t *device, uint16_t mv);
bq25798_result_t bq25798_set_input_current_limit(bq25798_t *device, uint16_t ma);
bq25798_result_t bq25798_set_precharge_current(bq25798_t *device, uint16_t ma);
bq25798_result_t bq25798_set_termination_current(bq25798_t *device, uint16_t ma);
bq25798_result_t bq25798_set_recharge(bq25798_t *device, uint16_t offset_mv, uint8_t deglitch_code);
bq25798_result_t bq25798_set_charging_enabled(bq25798_t *device, bool enabled);
bq25798_result_t bq25798_set_termination_enabled(bq25798_t *device, bool enabled);
bq25798_result_t bq25798_set_ico_enabled(bq25798_t *device, bool enabled);
bq25798_result_t bq25798_set_input_current_limit_enabled(bq25798_t *device, bool enabled);
bq25798_result_t bq25798_set_charge_timers(bq25798_t *device, const bq25798_charge_timers_t *timers);
bq25798_result_t bq25798_validate_charge_profile(const bq25798_charge_profile_t *profile);
bq25798_result_t bq25798_apply_charge_profile(bq25798_t *device, const bq25798_charge_profile_t *profile);

bq25798_result_t bq25798_adc_configure(bq25798_t *device, bq25798_adc_mode_t mode, bq25798_adc_resolution_t resolution, uint16_t channels);
bq25798_result_t bq25798_adc_read_configuration(bq25798_t *device, bq25798_adc_configuration_t *configuration);
bq25798_result_t bq25798_adc_start_one_shot(bq25798_t *device);
bq25798_result_t bq25798_adc_disable(bq25798_t *device);
bq25798_result_t bq25798_adc_read(bq25798_t *device, bq25798_adc_t *telemetry);

#ifdef __cplusplus
}
#endif
#endif
