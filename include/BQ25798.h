#ifndef BQ25798_H
#define BQ25798_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Config bits
#define BQ25798_I2C_ADDRESS 0x6BU
#define BQ25798_PART_NUMBER 3U
#define BQ25798_REG_PART_INFORMATION 0x48U
#define BQ25798_REG_STATUS_0 0x1BU
#define BQ25798_REG_FAULT_STATUS_0 0x20U
#define BQ25798_REG_CHARGER_FLAG_0 0x22U
#define BQ25798_REG_CHARGER_MASK_0 0x28U
#define BQ25798_CONFIGURATION_LENGTH 0x1BU

// ADC bits
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

// Event bits
#define BQ25798_EVENT_VBUS_PRESENT_CHANGED    (UINT32_C(1) << 0U)
#define BQ25798_EVENT_AC1_PRESENT_CHANGED     (UINT32_C(1) << 1U)
#define BQ25798_EVENT_AC2_PRESENT_CHANGED     (UINT32_C(1) << 2U)
#define BQ25798_EVENT_POWER_GOOD_CHANGED      (UINT32_C(1) << 3U)
#define BQ25798_EVENT_POOR_SOURCE             (UINT32_C(1) << 4U)
#define BQ25798_EVENT_WATCHDOG_EXPIRED        (UINT32_C(1) << 5U)
#define BQ25798_EVENT_VINDPM_ENTERED          (UINT32_C(1) << 6U)
#define BQ25798_EVENT_IINDPM_ENTERED          (UINT32_C(1) << 7U)
#define BQ25798_EVENT_BC12_DONE_CHANGED       (UINT32_C(1) << 8U)
#define BQ25798_EVENT_BATTERY_PRESENT_CHANGED (UINT32_C(1) << 9U)
#define BQ25798_EVENT_THERMAL_REGULATION      (UINT32_C(1) << 10U)
#define BQ25798_EVENT_INPUT_TYPE_CHANGED      (UINT32_C(1) << 12U)
#define BQ25798_EVENT_ICO_CHANGED             (UINT32_C(1) << 14U)
#define BQ25798_EVENT_CHARGE_STATE_CHANGED    (UINT32_C(1) << 15U)
#define BQ25798_EVENT_TOPOFF_TIMER_EXPIRED    (UINT32_C(1) << 16U)
#define BQ25798_EVENT_PRECHARGE_TIMER_EXPIRED (UINT32_C(1) << 17U)
#define BQ25798_EVENT_TRICKLE_TIMER_EXPIRED   (UINT32_C(1) << 18U)
#define BQ25798_EVENT_CHARGE_TIMER_EXPIRED    (UINT32_C(1) << 19U)
#define BQ25798_EVENT_MINIMUM_SYSTEM_CHANGED  (UINT32_C(1) << 20U)
#define BQ25798_EVENT_ADC_DONE                (UINT32_C(1) << 21U)
#define BQ25798_EVENT_DPDM_DONE               (UINT32_C(1) << 22U)
#define BQ25798_EVENT_TS_HOT                  (UINT32_C(1) << 24U)
#define BQ25798_EVENT_TS_WARM                 (UINT32_C(1) << 25U)
#define BQ25798_EVENT_TS_COOL                 (UINT32_C(1) << 26U)
#define BQ25798_EVENT_TS_COLD                 (UINT32_C(1) << 27U)
#define BQ25798_EVENT_BATTERY_TOO_LOW_FOR_OTG (UINT32_C(1) << 28U)

// Fault bits
#define BQ25798_FAULT_VAC1_OVP         (UINT16_C(1) << 0U)
#define BQ25798_FAULT_VAC2_OVP         (UINT16_C(1) << 1U)
#define BQ25798_FAULT_CONVERTER_OCP    (UINT16_C(1) << 2U)
#define BQ25798_FAULT_IBAT_OCP         (UINT16_C(1) << 3U)
#define BQ25798_FAULT_IBUS_OCP         (UINT16_C(1) << 4U)
#define BQ25798_FAULT_VBAT_OVP         (UINT16_C(1) << 5U)
#define BQ25798_FAULT_VBUS_OVP         (UINT16_C(1) << 6U)
#define BQ25798_FAULT_IBAT_REGULATION  (UINT16_C(1) << 7U)
#define BQ25798_FAULT_THERMAL_SHUTDOWN (UINT16_C(1) << 10U)
#define BQ25798_FAULT_OTG_UVP          (UINT16_C(1) << 12U)
#define BQ25798_FAULT_OTG_OVP          (UINT16_C(1) << 13U)
#define BQ25798_FAULT_VSYS_OVP         (UINT16_C(1) << 14U)
#define BQ25798_FAULT_VSYS_SHORT       (UINT16_C(1) << 15U)

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

// MPPT
typedef enum {
	BQ25798_MPPT_RATIO_56_25_PERCENT = 0,
	BQ25798_MPPT_RATIO_62_50_PERCENT,
	BQ25798_MPPT_RATIO_68_75_PERCENT,
	BQ25798_MPPT_RATIO_75_00_PERCENT,
	BQ25798_MPPT_RATIO_81_25_PERCENT,
	BQ25798_MPPT_RATIO_87_50_PERCENT,
	BQ25798_MPPT_RATIO_93_75_PERCENT,
	BQ25798_MPPT_RATIO_100_00_PERCENT
} bq25798_mppt_ratio_t;

typedef enum {
	BQ25798_MPPT_DELAY_50_MS = 0,
	BQ25798_MPPT_DELAY_300_MS,
	BQ25798_MPPT_DELAY_2_SECONDS,
	BQ25798_MPPT_DELAY_5_SECONDS
} bq25798_mppt_delay_t;

typedef enum {
	BQ25798_MPPT_INTERVAL_30_SECONDS = 0,
	BQ25798_MPPT_INTERVAL_2_MINUTES,
	BQ25798_MPPT_INTERVAL_10_MINUTES,
	BQ25798_MPPT_INTERVAL_30_MINUTES
} bq25798_mppt_interval_t;

typedef struct {
	bq25798_mppt_ratio_t ratio;
	bq25798_mppt_delay_t delay;
	bq25798_mppt_interval_t interval;
} bq25798_mppt_settings_t;

typedef struct {
	uint8_t raw; 
	bq25798_mppt_settings_t settings;
	uint16_t ratio_basis_points;
	uint16_t sample_delay_ms;
	uint16_t sample_interval_seconds;
	bool enabled;
} bq25798_mppt_configuration_t;

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

// Events and interrupts
typedef struct {
	uint8_t raw[6];
	uint32_t charger;
	uint16_t fault;
	bool any;
	bool any_fault;
} bq25798_events_t;

typedef struct {
	uint8_t raw[6];
	uint32_t charger;
	uint16_t fault;
} bq25798_interrupt_masks_t;

// Initialize the driver (does not actually initialize the charger)
bq25798_result_t bq25798_init(bq25798_t *device, bq25798_read_fn read, void *context);

// Basic IO
bq25798_result_t bq25798_read_u8(bq25798_t *device, uint8_t reg, uint8_t *value);
bq25798_result_t bq25798_read_u16(bq25798_t *device, uint8_t reg, uint16_t *value);
bq25798_result_t bq25798_read_identity(bq25798_t *device, bq25798_identity_t *identity);
bq25798_result_t bq25798_read_status(bq25798_t *device, bq25798_status_t *status);
bq25798_result_t bq25798_read_configuration(bq25798_t *device, bq25798_configuration_t *configuration);
bq25798_result_t bq25798_set_write_callback(bq25798_t *device, bq25798_write_fn write);

// Watchdog control
bq25798_result_t bq25798_set_watchdog(bq25798_t *device, uint8_t code);
bq25798_result_t bq25798_kick_watchdog(bq25798_t *device);
bq25798_result_t bq25798_set_watchdog_charge_stop(bq25798_t *device, bool enabled);

// Charge configuration/profile control
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
bq25798_result_t bq25798_set_ibat_discharge_sensing(bq25798_t *device, bool enabled);

// MPPT control
bq25798_result_t bq25798_mppt_configure(bq25798_t *device, const bq25798_mppt_settings_t *settings);
bq25798_result_t bq25798_mppt_read_configuration(bq25798_t *device, bq25798_mppt_configuration_t *configuration);
bq25798_result_t bq25798_mppt_set_enabled(bq25798_t *device, bool enabled);

// ADC control
bq25798_result_t bq25798_adc_configure(bq25798_t *device, bq25798_adc_mode_t mode, bq25798_adc_resolution_t resolution, uint16_t channels);
bq25798_result_t bq25798_adc_read_configuration(bq25798_t *device, bq25798_adc_configuration_t *configuration);
bq25798_result_t bq25798_adc_start_one_shot(bq25798_t *device);
bq25798_result_t bq25798_adc_disable(bq25798_t *device);
bq25798_result_t bq25798_adc_read(bq25798_t *device, bq25798_adc_t *telemetry);
bq25798_result_t bq25798_invalidate_adc_state(bq25798_t *device);

// Event and interrupt control
bq25798_result_t bq25798_read_events(bq25798_t *device, bq25798_events_t *events);
bq25798_result_t bq25798_read_interrupt_masks(bq25798_t *device, bq25798_interrupt_masks_t *masks);
bq25798_result_t bq25798_set_interrupt_masks(bq25798_t *device, uint32_t charger_masks, uint16_t fault_masks);
bq25798_result_t bq25798_reset_registers(bq25798_t *device);

#ifdef __cplusplus
}
#endif
#endif
