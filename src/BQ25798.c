#include "BQ25798.h"
#include <stddef.h>

// Read an unsigned big-endian integer from an unaligned byte buffer
static uint16_t bq25798_read_u16_be(const uint8_t *data){
	return (uint16_t)(((uint16_t)data[0] << 8U) | ((uint16_t)data[1]));
}

static int32_t bq25798_read_i16_be(const uint8_t *data){
	uint16_t value = bq25798_read_u16_be(data);
	return (value & 0x8000U) != 0U ? (int32_t)value - 65536 : (int32_t)value;
}

static uint32_t bq25798_pack_u32_le(const uint8_t *data){
	return ((uint32_t)data[0]) |
		   ((uint32_t)data[1] << 8U) |
		   ((uint32_t)data[2] << 16U) |
		   ((uint32_t)data[3] << 24U);
}

static uint16_t bq25798_pack_u16_le(const uint8_t *data){
	return (uint16_t)(((uint16_t)data[0]) |
					  ((uint16_t)data[1] << 8U));
}

static bq25798_result_t bq25798_read(bq25798_t *device, uint8_t reg, uint8_t *data, uint16_t length){
	if(device == NULL || device->read == NULL || data == NULL){
		return BQ25798_NULL_ARGUMENT;
	}
	
	device->last_io_error = device->read(device->context, BQ25798_I2C_ADDRESS, reg, data, length);
	
	return device->last_io_error == 0 ? BQ25798_OK : BQ25798_IO_ERROR;
}

bq25798_result_t bq25798_read_u8(bq25798_t *device, uint8_t reg, uint8_t *value){
	uint8_t data;
	if(value == NULL){ 
		return BQ25798_NULL_ARGUMENT; 
	}
	
	if(reg > 0x48U){ 
		return BQ25798_INVALID_REGISTER; 
	}
	
	bq25798_result_t result = bq25798_read(device, reg, &data, 1U);
	if(result == BQ25798_OK){ 
		*value = data; 
	}
	
	return result;
}

static bool bq25798_is_u16_register(uint8_t reg){
	return reg == 0x01U || 
		   reg == 0x03U || 
		   reg == 0x06U ||
		   reg == 0x0BU || 
		   reg == 0x19U ||
		   (reg >= 0x31U && reg <= 0x45U && (reg & 1U) != 0U);
}

bq25798_result_t bq25798_read_u16(bq25798_t *device, uint8_t reg, uint16_t *value){
	uint8_t data[2];
	if(value == NULL){ 
		return BQ25798_NULL_ARGUMENT; 
	}
	
	if(!bq25798_is_u16_register(reg)){ 
		return BQ25798_INVALID_REGISTER; 
	}
	
	bq25798_result_t result = bq25798_read(device, reg, data, 2U);
	if(result == BQ25798_OK){ 
		*value = bq25798_read_u16_be(data); 
	}
	
	return result;
}

static bq25798_result_t bq25798_write_u8(bq25798_t *device, uint8_t reg, uint8_t value){
	if(device == NULL || device->read == NULL){
		return BQ25798_NULL_ARGUMENT;
	}
	
	if(device->write == NULL){
		return BQ25798_WRITE_NOT_AVAILABLE;
	}
	
	device->last_io_error = device->write(device->context, BQ25798_I2C_ADDRESS, reg, &value, 1U);
	
	return device->last_io_error == 0 ? BQ25798_OK : BQ25798_IO_ERROR;
}

bq25798_result_t bq25798_init(bq25798_t *device, bq25798_read_fn read, void *context){
	if(device == NULL || read == NULL){ 
		return BQ25798_NULL_ARGUMENT; 
	}
	
	device->read = read;
	device->context = context;
	device->last_io_error = 0;
	
	return BQ25798_OK;
}

bq25798_result_t bq25798_read_identity(bq25798_t *device, bq25798_identity_t *identity){
	uint8_t data;
	if(identity == NULL){ 
		return BQ25798_NULL_ARGUMENT; 
	}
	
	bq25798_result_t result = bq25798_read_u8(device, BQ25798_REG_PART_INFORMATION, &data);
	if(result != BQ25798_OK){ 
		return result; 
	}

	identity->raw = data;
	identity->part_number = (data >> 3U) & 7U;
	identity->revision = data & 7U;

	return identity->part_number == BQ25798_PART_NUMBER ? BQ25798_OK : BQ25798_WRONG_DEVICE;
}

// Read the current condition of the charger from REG1B-REG21 (does not clear these flags)
bq25798_result_t bq25798_read_status(bq25798_t *device, bq25798_status_t *status){
	bq25798_status_t decoded = {0};
	if(status == NULL){ 
		return BQ25798_NULL_ARGUMENT; 
	}

	bq25798_result_t result = bq25798_read(device, BQ25798_REG_STATUS_0, decoded.raw, 7U);
	if(result != BQ25798_OK){ 
		return result; 
	}

	decoded.input_current_regulation =  (decoded.raw[0] & 0x80U) != 0U;
	decoded.input_voltage_regulation =  (decoded.raw[0] & 0x40U) != 0U;
	decoded.watchdog_expired =          (decoded.raw[0] & 0x20U) != 0U;
	decoded.power_good =                (decoded.raw[0] & 0x08U) != 0U;
	decoded.ac2_present =               (decoded.raw[0] & 0x04U) != 0U;
	decoded.ac1_present =               (decoded.raw[0] & 0x02U) != 0U;
	decoded.vbus_present =              (decoded.raw[0] & 0x01U) != 0U;
	decoded.charge_state =              (decoded.raw[1] >> 5U) & 7U;
	decoded.input_type =                (decoded.raw[1] >> 1U) & 15U;
	decoded.detection_complete =        (decoded.raw[1] & 1U) != 0U;
	decoded.ico_state =                 decoded.raw[2] >> 6U;
	decoded.thermal_regulation =        (decoded.raw[2] & 0x04U) != 0U;
	decoded.detection_running =         (decoded.raw[2] & 0x02U) != 0U;
	decoded.vbat_above_uvlo =           (decoded.raw[2] & 0x01U) != 0U;
	decoded.input_fets2_detected =      (decoded.raw[3] & 0x80U) != 0U;
	decoded.input_fets1_detected =      (decoded.raw[3] & 0x40U) != 0U;
	decoded.adc_done =                  (decoded.raw[3] & 0x20U) != 0U;
	decoded.minimum_system_regulation = (decoded.raw[3] & 0x10U) != 0U;
	decoded.expired_charge_timers =     decoded.raw[3] & 0x0EU;
	decoded.temperature_status =        decoded.raw[4];
	decoded.fault_status_0 =            decoded.raw[5];
	decoded.fault_status_1 =            decoded.raw[6];
	decoded.protection_active =         ((decoded.raw[5] & 0x7FU) != 0U) || ((decoded.raw[6] & 0xF4U) != 0U);
	
	*status = decoded;

	return BQ25798_OK;
}

bq25798_result_t bq25798_read_configuration(bq25798_t *device, bq25798_configuration_t *configuration){
	bq25798_configuration_t decoded = {0};
	if(configuration == NULL){ 
		return BQ25798_NULL_ARGUMENT; 
	}

	bq25798_result_t result = bq25798_read(device, 0x00U, decoded.raw, BQ25798_CONFIGURATION_LENGTH);
	if(result != BQ25798_OK){ 
		return result; 
	}
	
	decoded.cell_count =                   (uint8_t)((decoded.raw[0x0AU] >> 6U) + 1U);
	decoded.minimum_system_mv =            (uint16_t)(2500U + (decoded.raw[0] & 0x3FU) * 250U);
	decoded.charge_voltage_mv =            (uint16_t)((bq25798_read_u16_be(&decoded.raw[1]) & 0x7FFU) * 10U);
	decoded.charge_current_ma =            (uint16_t)((bq25798_read_u16_be(&decoded.raw[3]) & 0x1FFU) * 10U);
	decoded.input_voltage_limit_mv =       (uint16_t)(decoded.raw[5] * 100U);
	decoded.input_current_limit_ma =       (uint16_t)((bq25798_read_u16_be(&decoded.raw[6]) & 0x1FFU) * 10U);
	decoded.precharge_current_ma =         (uint16_t)((decoded.raw[8] & 0x3FU) * 40U);
	decoded.precharge_threshold_code =     decoded.raw[8] >> 6U;
	decoded.termination_current_ma =       (uint16_t)((decoded.raw[9] & 0x1FU) * 40U);
	decoded.recharge_offset_mv =           (uint16_t)(50U + (decoded.raw[0x0A] & 0x0FU) * 50U);
	decoded.recharge_deglitch_code =       (decoded.raw[0x0A] >> 4U) & 3U;
	decoded.charging_enabled =             (decoded.raw[0x0F] & 0x20U) != 0U;
	decoded.termination_enabled =          (decoded.raw[0x0F] & 0x02U) != 0U;
	decoded.ico_enabled =                  (decoded.raw[0x0F] & 0x10U) != 0U;
	decoded.high_impedance =               (decoded.raw[0x0F] & 0x04U) != 0U;
	decoded.input_current_limit_enabled =  (decoded.raw[0x14] & 0x04U) != 0U;
	decoded.external_input_limit_enabled = (decoded.raw[0x14] & 0x02U) != 0U;

	*configuration = decoded;

	return BQ25798_OK;
}

// Only used on the explicitly selected ordinary control registers below.
static bq25798_result_t bq25798_update_control(bq25798_t *device, uint8_t reg, uint8_t clear_mask, uint8_t set_mask, uint8_t verify_mask){
	uint8_t previous, actual;
	
	bq25798_result_t result = bq25798_read_u8(device, reg, &previous);
	if(result != BQ25798_OK){
		return result;
	}

	uint8_t wanted = (uint8_t)((previous & (uint8_t)~clear_mask) | set_mask);
	result = bq25798_write_u8(device, reg, wanted);
	if(result != BQ25798_OK){
		return result;
	}

	result = bq25798_read_u8(device, reg, &actual);
	if(result != BQ25798_OK){
		return result;
	}

	return (actual & verify_mask) == (wanted & verify_mask) ? BQ25798_OK : BQ25798_VERIFY_FAILED;
}

bq25798_result_t bq25798_set_write_callback(bq25798_t *device, bq25798_write_fn write){
	if(device == NULL || device->read == NULL || write == NULL){
		return BQ25798_NULL_ARGUMENT;
	}
	
	device->write = write;
	
	return BQ25798_OK;
}

bq25798_result_t bq25798_mppt_read_configuration(bq25798_t *device, bq25798_mppt_configuration_t *configuration){
	if(configuration == NULL){
		return BQ25798_NULL_ARGUMENT;
	}
	
	uint8_t raw;
	
	bq25798_result_t result = bq25798_read_u8(device, 0x15U, &raw);
	if(result != BQ25798_OK){
		return result;
	}

	static const uint16_t delays[] = {50U, 300U, 2000U, 5000U};
	static const uint16_t intervals[] = {30U, 120U, 600U, 1800U};
	bq25798_mppt_configuration_t decoded = {0};
	decoded.raw = raw;
	decoded.settings.ratio = (bq25798_mppt_ratio_t)(raw >> 5U);
	decoded.settings.delay = (bq25798_mppt_delay_t)((raw >> 3U) & 3U);
	decoded.settings.interval = (bq25798_mppt_interval_t)((raw >> 1U) & 3U);
	decoded.ratio_basis_points = (uint16_t)(5625U + (raw >> 5U) * 625U);
	decoded.sample_delay_ms = delays[(raw >> 3U) & 3U];
	decoded.sample_interval_seconds = intervals[(raw >> 1U) & 3U];
	decoded.enabled = (raw & 1U) != 0U;
	*configuration = decoded;

	return BQ25798_OK;
}

bq25798_result_t bq25798_mppt_configure(bq25798_t *device, const bq25798_mppt_settings_t *settings){
	if((settings == NULL) || (device == NULL) || (device->read == NULL)){
		return BQ25798_NULL_ARGUMENT;
	}

	if(((uint32_t)settings->ratio > 7U) || 
		((uint32_t)settings->delay > 3U) ||
		((uint32_t)settings->interval > 3U)){
		return BQ25798_INVALID_ARGUMENT;
	}
	
	if(device->write == NULL){
		return BQ25798_WRITE_NOT_AVAILABLE;
	}
	
	uint8_t current;
	bq25798_result_t result = bq25798_read_u8(device, 0x15U, &current);
	if(result != BQ25798_OK){
		return result;
	}

	if(current & 1U){
		return BQ25798_BUSY;
	}
	
	uint8_t wanted = (uint8_t)(((uint8_t)settings->ratio << 5U) |
							   ((uint8_t)settings->delay << 3U) | 
							   ((uint8_t)settings->interval << 1U));
	
	return bq25798_update_control(device, 0x15U, 0xFFU, wanted, 0xFFU);
}

bq25798_result_t bq25798_mppt_set_enabled(bq25798_t *device, bool enabled){
	if((device == NULL) || (device->read == NULL)){
		return BQ25798_NULL_ARGUMENT;
	}
	
	if(device->write == NULL){
		return BQ25798_WRITE_NOT_AVAILABLE;
	}
	
	if(enabled){
		uint8_t control0, control3;
		bq25798_configuration_t config;
		bq25798_status_t status;
		bq25798_result_t result = bq25798_read_configuration(device, &config);
		
		if(result != BQ25798_OK){
			return result;
		}
		
		control0 = config.raw[0x0F]; control3 = config.raw[0x13];
		if((control0 & 0x18U) || (control3 & 0x02U)){
			return BQ25798_BUSY;
		}
		
		if(config.high_impedance || (control0 & 1U) || (config.raw[0x12] & 0xC0U)){
			return BQ25798_INVALID_ARGUMENT;
		}

		result = bq25798_read_status(device, &status);
		if(result != BQ25798_OK){
			return result;
		}
		
		if (!status.power_good || 
			!status.vbus_present ||
			status.minimum_system_regulation || 
			status.protection_active ||
			status.watchdog_expired){
			return BQ25798_NOT_READY;
		}
	}

	return bq25798_update_control(device, 0x15U, 1U, enabled ? 1U : 0U, 0xFFU);
}

// Set how long the good boy watis before barking
bq25798_result_t bq25798_set_watchdog(bq25798_t *device, uint8_t code){
	if(code > 7U){
		return BQ25798_INVALID_ARGUMENT;
	}

	return bq25798_update_control(device, 0x10U, 0x0FU, code, 0xF7U);
}

// Register writes are for good boys
bq25798_result_t bq25798_kick_watchdog(bq25798_t *device){
	uint8_t control;
	bq25798_result_t result = bq25798_read_u8(device, 0x10U, &control);
	if(result != BQ25798_OK){
		return result;
	}

	return bq25798_write_u8(device, 0x10U, (uint8_t)(control | 0x08U));
}

bq25798_result_t bq25798_set_watchdog_charge_stop(bq25798_t *device, bool enabled){
	// Otherwise watchdog expiration would retain EN_CHG
	return bq25798_update_control(device, 0x09U, 0x60U, enabled ? 0x20U : 0U, 0xBFU);
}

bq25798_result_t bq25798_invalidate_adc_state(bq25798_t *device){
	if(device == NULL){
		return BQ25798_NULL_ARGUMENT;
	}

	device->adc_configured = false;
	device->adc_one_shot_pending = false;
	
	return BQ25798_OK;
}

bq25798_result_t bq25798_reset_registers(bq25798_t *device){
	uint8_t control;
	if((device == NULL) || (device->read == NULL)){
		return BQ25798_NULL_ARGUMENT;
	}
	
	if(device->write == NULL){
		return BQ25798_WRITE_NOT_AVAILABLE;
	}
	
	bq25798_result_t result = bq25798_read_u8(device, 0x09U, &control);
	if(result != BQ25798_OK){
		return result;
	}

	(void)bq25798_invalidate_adc_state(device);

	return bq25798_write_u8(device, 0x09U, (uint8_t)(control | 0x40U));
}

bq25798_result_t bq25798_set_ibat_discharge_sensing(bq25798_t *device, bool enabled){
	return bq25798_update_control(device, 0x14U, 0x20U, enabled ? 0x20U : 0U, 0xBFU);
}

static bool bq25798_on_grid(uint16_t value, uint16_t minimum, uint16_t maximum, uint16_t step){
	return ((value >= minimum) && (value <= maximum) && ((uint16_t)(value - minimum) % step == 0U));
}

static bool bq25798_valid_charge_voltage(uint8_t cells, uint16_t mv){
	static const uint16_t minimum[4] = {3000U, 5000U, 10000U, 14000U};
	static const uint16_t maximum[4] = {4990U, 9990U, 13990U, 18800U};
	return (cells >= 1U) && (cells <= 4U) && 
			bq25798_on_grid(mv, minimum[cells - 1U], maximum[cells - 1U], 10U);
}

static bq25798_result_t bq25798_update_limit_u16(bq25798_t *device, uint8_t reg, uint16_t mask, uint16_t code){
	uint16_t previous, actual;
	uint8_t data[2];

	if(device == NULL || device->read == NULL){
		return BQ25798_NULL_ARGUMENT;
	}

	if(device->write == NULL){
		return BQ25798_WRITE_NOT_AVAILABLE;
	}

	bq25798_result_t result = bq25798_read_u16(device, reg, &previous);
	if(result != BQ25798_OK){
		return result;
	}

	uint16_t wanted = (uint16_t)((previous & (uint16_t)~mask) | (code & mask));
	data[0] = (uint8_t)(wanted >> 8U);
	data[1] = (uint8_t)wanted;

	device->last_io_error = device->write(device->context, BQ25798_I2C_ADDRESS, reg, data, 2U);
	if(device->last_io_error != 0){
		return BQ25798_IO_ERROR;
	}

	result = bq25798_read_u16(device, reg, &actual);
	if(result != BQ25798_OK){
		return result;
	}

	return actual == wanted ? BQ25798_OK : BQ25798_VERIFY_FAILED;
}

bq25798_result_t bq25798_set_charge_voltage(bq25798_t *device, uint16_t mv){
	if(!bq25798_on_grid(mv, 3000U, 18800U, 10U)){
		return BQ25798_INVALID_ARGUMENT;
	}
	
	bq25798_configuration_t current;
	bq25798_result_t result = bq25798_read_configuration(device, &current);
	if(result != BQ25798_OK){
		return result;
	}
	
	if(!bq25798_valid_charge_voltage(current.cell_count, mv) || mv <= current.minimum_system_mv){
		return BQ25798_INVALID_ARGUMENT;
	}
	
	return bq25798_update_limit_u16(device, 0x01U, 0x07FFU, mv / 10U);
}

bq25798_result_t bq25798_set_minimum_system_voltage(bq25798_t *device, uint16_t mv){
	if(!bq25798_on_grid(mv, 2500U, 16000U, 250U)){
		return BQ25798_INVALID_ARGUMENT;
	}
	
	bq25798_configuration_t current;
	bq25798_result_t result = bq25798_read_configuration(device, &current);
	if(result != BQ25798_OK){
		return result;
	}
	
	if(mv >= current.charge_voltage_mv){
		return BQ25798_INVALID_ARGUMENT;
	}
	
	return bq25798_update_control(device, 0x00U, 0x3FU, (uint8_t)((mv - 2500U) / 250U), 0xFFU);
}

bq25798_result_t bq25798_set_charge_current(bq25798_t *device, uint16_t ma){
	if(!bq25798_on_grid(ma, 50U, 5000U, 10U)){
		return BQ25798_INVALID_ARGUMENT;
	}
	
	return bq25798_update_limit_u16(device, 0x03U, 0x01FFU, ma / 10U);
}

bq25798_result_t bq25798_set_input_voltage_limit(bq25798_t *device, uint16_t mv){
	if(!bq25798_on_grid(mv, 3600U, 22000U, 100U)){
		return BQ25798_INVALID_ARGUMENT;
	}
	
	return bq25798_update_control(device, 0x05U, 0xFFU, (uint8_t)(mv / 100U), 0xFFU);
}

bq25798_result_t bq25798_set_input_current_limit(bq25798_t *device, uint16_t ma){
	if(!bq25798_on_grid(ma, 100U, 3300U, 10U)){
		return BQ25798_INVALID_ARGUMENT;
	}

	return bq25798_update_limit_u16(device, 0x06U, 0x01FFU, ma / 10U);
}

bq25798_result_t bq25798_set_precharge_current(bq25798_t *device, uint16_t ma){
	if(!bq25798_on_grid(ma, 40U, 2000U, 40U)){
		return BQ25798_INVALID_ARGUMENT;
	}

	return bq25798_update_control(device, 0x08U, 0x3FU, (uint8_t)(ma / 40U), 0xFFU);
}

bq25798_result_t bq25798_set_termination_current(bq25798_t *device, uint16_t ma){
	if(!bq25798_on_grid(ma, 40U, 1000U, 40U)){
		return BQ25798_INVALID_ARGUMENT;
	}

	return bq25798_update_control(device, 0x09U, 0x5FU, (uint8_t)(ma / 40U), 0xBFU);
}

bq25798_result_t bq25798_set_recharge(bq25798_t *device, uint16_t offset_mv, uint8_t deglitch_code){
	if(!bq25798_on_grid(offset_mv, 50U, 800U, 50U) || deglitch_code > 3U){
		return BQ25798_INVALID_ARGUMENT;
	}

	uint8_t code = (uint8_t)((deglitch_code << 4U) | ((offset_mv - 50U) / 50U));

	return bq25798_update_control(device, 0x0AU, 0x3FU, code, 0xFFU);
}

static bq25798_result_t bq25798_update_charger_control(bq25798_t *device, uint8_t mask, bool enabled){
	return bq25798_update_control(device, 0x0FU, (uint8_t)(mask | 0x08U), enabled ? mask : 0U, 0xF6U);
}

bq25798_result_t bq25798_set_charging_enabled(bq25798_t *device, bool enabled){
	return bq25798_update_charger_control(device, 0x20U, enabled);
}

bq25798_result_t bq25798_set_termination_enabled(bq25798_t *device, bool enabled){
	return bq25798_update_charger_control(device, 0x02U, enabled);
}

bq25798_result_t bq25798_set_ico_enabled(bq25798_t *device, bool enabled){
	return bq25798_update_charger_control(device, 0x10U, enabled);
}

bq25798_result_t bq25798_set_input_current_limit_enabled(bq25798_t *device, bool enabled){
	return bq25798_update_control(device, 0x14U, 0x04U, enabled ? 0x04U : 0U, 0xBFU);
}

static bool bq25798_valid_timers(const bq25798_charge_timers_t *timers){
	return (uint32_t)timers->fast_charge_duration <= 3U && timers->top_off_code <= 3U;
}

static uint8_t bq25798_timer_control(const bq25798_charge_timers_t *timers){
	uint8_t value = (uint8_t)((timers->top_off_code << 6U) | ((uint8_t)timers->fast_charge_duration << 1U));
	
	if(timers->trickle_enabled){
		value |= 0x20U;
	}
	
	if(timers->precharge_enabled){
		value |= 0x10U;
	}
	
	if(timers->fast_charge_enabled){
		value |= 0x08U;
	}
	
	if(timers->double_during_regulation){
		value |= 0x01U;
	}
	
	return value;
}

bq25798_result_t bq25798_set_charge_timers(bq25798_t *device, const bq25798_charge_timers_t *timers){
	if(timers == NULL){
		return BQ25798_NULL_ARGUMENT;
	}

	if(!bq25798_valid_timers(timers)){
		return BQ25798_INVALID_ARGUMENT;
	}

	bq25798_result_t result = bq25798_update_control(device, 0x0EU, 0xFFU, bq25798_timer_control(timers), 0xFFU);
	if(result != BQ25798_OK){
		return result;
	}

	return bq25798_update_control(device, 0x0DU, 0x80U, timers->precharge_half_hour ? 0x80U : 0U, 0xFFU);
}

bq25798_result_t bq25798_validate_charge_profile(const bq25798_charge_profile_t *profile){
	if(profile == NULL){
		return BQ25798_NULL_ARGUMENT;
	}

	if(!bq25798_valid_charge_voltage(profile->cell_count, profile->charge_voltage_mv) ||
			!bq25798_on_grid(profile->minimum_system_mv, 2500U, 16000U, 250U) ||
			profile->minimum_system_mv >= profile->charge_voltage_mv ||
			!bq25798_on_grid(profile->charge_current_ma, 50U, 5000U, 10U) ||
			!bq25798_on_grid(profile->input_voltage_limit_mv, 3600U, 22000U, 100U) ||
			!bq25798_on_grid(profile->input_current_limit_ma, 100U, 3300U, 10U) ||
			!bq25798_on_grid(profile->precharge_current_ma, 40U, 2000U, 40U) ||
			!bq25798_on_grid(profile->termination_current_ma, 40U, 1000U, 40U) ||
			!bq25798_on_grid(profile->recharge_offset_mv, 50U, 800U, 50U) ||
			profile->recharge_deglitch_code > 3U || !bq25798_valid_timers(&profile->timers)){
		return BQ25798_INVALID_ARGUMENT;
	}

	if(profile->termination_enabled && profile->termination_current_ma >= profile->charge_current_ma){
		return BQ25798_INVALID_ARGUMENT;
	}

	return BQ25798_OK;
}

bq25798_result_t bq25798_apply_charge_profile(bq25798_t *device, const bq25798_charge_profile_t *profile){
	bq25798_result_t result = bq25798_validate_charge_profile(profile);
	if(result != BQ25798_OK){
		return result;
	}

	if(device == NULL || device->read == NULL){
		return BQ25798_NULL_ARGUMENT;
	}

	if(device->write == NULL){
		return BQ25798_WRITE_NOT_AVAILABLE;
	}

	bq25798_configuration_t current;
	result = bq25798_read_configuration(device, &current);
	if(result != BQ25798_OK){
		return result;
	}

	if(current.cell_count != profile->cell_count){
		return BQ25798_INVALID_ARGUMENT;
	}

	result = bq25798_set_charging_enabled(device, false);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_ico_enabled(device, false);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_input_current_limit_enabled(device, true);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_input_current_limit(device, profile->input_current_limit_ma);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_input_voltage_limit(device, profile->input_voltage_limit_mv);
	if(result != BQ25798_OK){
		goto failed;
	}

	if(profile->charge_voltage_mv <= current.minimum_system_mv){
		result = bq25798_set_minimum_system_voltage(device, profile->minimum_system_mv);
		if(result != BQ25798_OK){
			goto failed;
		}
	}
	
	result = bq25798_set_charge_voltage(device, profile->charge_voltage_mv);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_minimum_system_voltage(device, profile->minimum_system_mv);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_charge_current(device, profile->charge_current_ma);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_precharge_current(device, profile->precharge_current_ma);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_termination_current(device, profile->termination_current_ma);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_recharge(device, profile->recharge_offset_mv, profile->recharge_deglitch_code);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_charge_timers(device, &profile->timers);
	if(result != BQ25798_OK){
		goto failed;
	}
	
	result = bq25798_set_termination_enabled(device, profile->termination_enabled);
	if(result != BQ25798_OK){
		goto failed;
	}

	// Recheck the entire profile before enabling
	{
		bq25798_configuration_t verified;
		result = bq25798_read_configuration(device, &verified);
		if(result != BQ25798_OK){
			goto failed;
		}
		
		if(verified.cell_count != profile->cell_count ||
				verified.minimum_system_mv != profile->minimum_system_mv ||
				verified.charge_voltage_mv != profile->charge_voltage_mv ||
				verified.charge_current_ma != profile->charge_current_ma ||
				verified.input_voltage_limit_mv != profile->input_voltage_limit_mv ||
				verified.input_current_limit_ma != profile->input_current_limit_ma ||
				verified.precharge_current_ma != profile->precharge_current_ma ||
				verified.termination_current_ma != profile->termination_current_ma ||
				verified.recharge_offset_mv != profile->recharge_offset_mv ||
				verified.recharge_deglitch_code != profile->recharge_deglitch_code ||
				verified.termination_enabled != profile->termination_enabled ||
				verified.charging_enabled || verified.ico_enabled ||
				!verified.input_current_limit_enabled ||
				verified.high_impedance != current.high_impedance ||
				verified.external_input_limit_enabled != current.external_input_limit_enabled ||
				verified.raw[0x0E] != bq25798_timer_control(&profile->timers) ||
				((verified.raw[0x0D] & 0x80U) != 0U) != profile->timers.precharge_half_hour){
			
			result = BQ25798_VERIFY_FAILED;
			
			goto failed;
		}
	}
	result = bq25798_set_charging_enabled(device, profile->charging_enabled);
	if(result == BQ25798_OK){
		return result;
	}
failed:
	{
		int32_t original_io_error = device->last_io_error;
		(void)bq25798_set_charging_enabled(device, false);
		device->last_io_error = original_io_error;
	}

	return result;
}

static uint16_t bq25798_adc_channels(uint8_t disable0, uint8_t disable1){
	uint16_t channels = 0U;
	const uint8_t bits0[11] = {0x80U,0x40U,0x20U,0U,0U,0x10U,0x08U,0x04U,0x02U,0U,0U};
	const uint8_t bits1[11] = {0U,0U,0U,0x10U,0x20U,0U,0U,0U,0U,0x80U,0x40U};
	for(uint8_t index = 0; index < 11U; index++){
		if((disable0 & bits0[index]) == 0U && (disable1 & bits1[index]) == 0U){
			channels |= (uint16_t)(1U << index);
		}
	}

	return channels;
}

bq25798_result_t bq25798_adc_read_configuration(bq25798_t *device, bq25798_adc_configuration_t *configuration){
	uint8_t data[3];
	if(configuration == NULL){
		return BQ25798_NULL_ARGUMENT;
	}
	
	bq25798_result_t result = bq25798_read(device, 0x2EU, data, 3U);
	if(result != BQ25798_OK){
		return result;
	}

	bq25798_adc_configuration_t decoded = {0};
	decoded.raw_control = data[0];
	decoded.raw_disable_0 = data[1];
	decoded.raw_disable_1 = data[2];
	decoded.channels = bq25798_adc_channels(data[1], data[2]);
	decoded.enabled = (data[0] & 0x80U) != 0U;
	decoded.mode = (data[0] & 0x40U) != 0U ? BQ25798_ADC_ONE_SHOT : BQ25798_ADC_CONTINUOUS;
	decoded.resolution = (bq25798_adc_resolution_t)((data[0] >> 4U) & 3U);
	decoded.averaging = (data[0] & 0x08U) != 0U;
	*configuration = decoded;
	
	return BQ25798_OK;
}

bq25798_result_t bq25798_adc_configure(bq25798_t *device, bq25798_adc_mode_t mode, bq25798_adc_resolution_t resolution, uint16_t channels){
	if(device == NULL || device->read == NULL){
		return BQ25798_NULL_ARGUMENT;
	}
	
	if((mode != BQ25798_ADC_CONTINUOUS && mode != BQ25798_ADC_ONE_SHOT) || 
		(resolution != BQ25798_ADC_15_BIT && resolution != BQ25798_ADC_14_BIT && resolution != BQ25798_ADC_13_BIT) || 
		channels == 0U ||
		(channels & (uint16_t)~BQ25798_ADC_ALL) != 0U){
		return BQ25798_INVALID_ARGUMENT;
	}
	
	if(device->write == NULL){
		return BQ25798_WRITE_NOT_AVAILABLE;
	}

	device->adc_configured = false;
	device->adc_one_shot_pending = false;
	bq25798_adc_configuration_t previous, actual;
	
	bq25798_result_t result = bq25798_adc_read_configuration(device, &previous);
	if(result != BQ25798_OK){
		return result;
	}

	uint8_t control = (uint8_t)((previous.raw_control & 0x03U) | (mode == BQ25798_ADC_ONE_SHOT ? 0x40U : 0U) | ((uint8_t)resolution << 4U));
	result = bq25798_write_u8(device, 0x2EU, control);
	if(result != BQ25798_OK){
		return result;
	}
	
	uint8_t disable0 = previous.raw_disable_0 & 0x01U;
	uint8_t disable1 = previous.raw_disable_1 & 0x0FU;

	if((channels & BQ25798_ADC_IBUS) == 0U){
		disable0 |= 0x80U;
	}

	if((channels & BQ25798_ADC_IBAT) == 0U){
		disable0 |= 0x40U;
	}
	
	if((channels & BQ25798_ADC_VBUS) == 0U){
		disable0 |= 0x20U;
	}
	
	if((channels & BQ25798_ADC_VBAT) == 0U){
		disable0 |= 0x10U;
	}
	
	if((channels & BQ25798_ADC_VSYS) == 0U){
		disable0 |= 0x08U;
	}
	
	if((channels & BQ25798_ADC_TS) == 0U){
		disable0 |= 0x04U;
	}
	
	if((channels & BQ25798_ADC_TDIE) == 0U){
		disable0 |= 0x02U;
	}
	
	if((channels & BQ25798_ADC_DP) == 0U){
		disable1 |= 0x80U;
	}
	
	if((channels & BQ25798_ADC_DM) == 0U){
		disable1 |= 0x40U;
	}
	
	if((channels & BQ25798_ADC_VAC2) == 0U){
		disable1 |= 0x20U;
	}
	
	if((channels & BQ25798_ADC_VAC1) == 0U){
		disable1 |= 0x10U;
	}
	
	result = bq25798_write_u8(device, 0x2FU, disable0);
	if(result != BQ25798_OK){
		return result;
	}
	
	result = bq25798_write_u8(device, 0x30U, disable1);
	if(result != BQ25798_OK){
		return result;
	}
	
	if(mode == BQ25798_ADC_CONTINUOUS){
		control |= 0x80U;
		result = bq25798_write_u8(device, 0x2EU, control);
		if(result != BQ25798_OK){
			return result;
		}
	}

	result = bq25798_adc_read_configuration(device, &actual);
	if(result != BQ25798_OK){
		return result;
	}

	if(actual.raw_control != control ||  actual.raw_disable_0 != disable0 || actual.raw_disable_1 != disable1){
		return BQ25798_VERIFY_FAILED;
	}

	device->adc_configured = true;
	
	return BQ25798_OK;
}

bq25798_result_t bq25798_adc_start_one_shot(bq25798_t *device){
	if(device == NULL){
		return BQ25798_NULL_ARGUMENT;
	}

	if(!device->adc_configured){
		return BQ25798_NOT_CONFIGURED;
	}
	
	if(device->adc_one_shot_pending){
		return BQ25798_BUSY;
	}
	
	bq25798_adc_configuration_t configuration;
	bq25798_result_t result = bq25798_adc_read_configuration(device, &configuration);
	if(result != BQ25798_OK){
		return result;
	}
	
	if(configuration.mode != BQ25798_ADC_ONE_SHOT){
		return BQ25798_INVALID_ARGUMENT;
	}
	
	if(configuration.enabled){
		return BQ25798_BUSY;
	}
	
	result = bq25798_write_u8(device, 0x2EU, (uint8_t)(configuration.raw_control | 0x80U));
	if(result == BQ25798_OK){
		device->adc_one_shot_pending = true;
	}
	
	return result;
}

bq25798_result_t bq25798_adc_disable(bq25798_t *device){
	if(device == NULL){
		return BQ25798_NULL_ARGUMENT;
	}
	
	device->adc_configured = false;
	device->adc_one_shot_pending = false;

	return bq25798_update_control(device, 0x2EU, 0x80U, 0U, 0xFFU);
}

bq25798_result_t bq25798_adc_read(bq25798_t *device, bq25798_adc_t *telemetry){
	if(device == NULL || telemetry == NULL){
		return BQ25798_NULL_ARGUMENT;
	}
	
	if(!device->adc_configured){
		return BQ25798_NOT_CONFIGURED;
	}
	
	bq25798_adc_configuration_t configuration;
	bq25798_result_t result = bq25798_adc_read_configuration(device, &configuration);
	if(result != BQ25798_OK){
		return result;
	}
	
	bool one_shot = (configuration.mode == BQ25798_ADC_ONE_SHOT);
	if(one_shot){
		if(!device->adc_one_shot_pending || configuration.enabled){
			return BQ25798_NOT_READY;
		}
		
		uint8_t status;
		result = bq25798_read_u8(device, 0x1EU, &status);
		if(result != BQ25798_OK){
			return result;
		}
		if((status & 0x20U) == 0U){
			return BQ25798_NOT_READY;
		}
	}
	else if(!configuration.enabled){
		return BQ25798_NOT_READY;
	}
	
	uint8_t control14;
	result = bq25798_read_u8(device, 0x14U, &control14);
	if(result != BQ25798_OK){
		return result;
	}
	
	bq25798_adc_t decoded = {0};
	result = bq25798_read(device, 0x31U, decoded.raw, 22U);
	if(result != BQ25798_OK){
		return result;
	}
	
	decoded.channels = configuration.channels;
	decoded.completed_one_shot = one_shot;
	decoded.ibat_discharge_sensing_enabled = (control14 & 0x20U) != 0U;
	decoded.ibus_ma = bq25798_read_i16_be(&decoded.raw[0]);
	decoded.ibat_ma = bq25798_read_i16_be(&decoded.raw[2]);
	decoded.vbus_mv = bq25798_read_u16_be(&decoded.raw[4]);
	decoded.vac1_mv = bq25798_read_u16_be(&decoded.raw[6]);
	decoded.vac2_mv = bq25798_read_u16_be(&decoded.raw[8]);
	decoded.vbat_mv = bq25798_read_u16_be(&decoded.raw[10]);
	decoded.vsys_mv = bq25798_read_u16_be(&decoded.raw[12]);
	decoded.ts_percent = (float)bq25798_read_u16_be(&decoded.raw[14]) * (100.0F / 1024.0F);
	decoded.die_temperature_mc = bq25798_read_i16_be(&decoded.raw[16]) * 500;
	decoded.dp_mv = bq25798_read_u16_be(&decoded.raw[18]);
	decoded.dm_mv = bq25798_read_u16_be(&decoded.raw[20]);
	
	*telemetry = decoded;
	
	if(one_shot){
		device->adc_one_shot_pending = false;
	}
	
	return BQ25798_OK;
}

// Interrupts and events
bq25798_result_t bq25798_set_interrupt_masks(bq25798_t *device, uint32_t charger_masks, uint16_t fault_masks){
	const uint32_t valid_charger = UINT32_C(0x1F7FD7FF);
	const uint16_t valid_fault = UINT16_C(0xF4FF);
	uint8_t wanted[6], actual[6];
	if(((charger_masks & ~valid_charger) != 0U) || ((fault_masks & (uint16_t)~valid_fault) != 0U)){
		return BQ25798_INVALID_ARGUMENT;
	}
	
	if((device == NULL) || (device->read == NULL)){
		return BQ25798_NULL_ARGUMENT;
	}
	
	if(device->write == NULL){
		return BQ25798_WRITE_NOT_AVAILABLE;
	}
	
	wanted[0] = (uint8_t)charger_masks;
	wanted[1] = (uint8_t)(charger_masks >> 8U);
	wanted[2] = (uint8_t)(charger_masks >> 16U);
	wanted[3] = (uint8_t)(charger_masks >> 24U);
	wanted[4] = (uint8_t)fault_masks;
	wanted[5] = (uint8_t)(fault_masks >> 8U);
	
	device->last_io_error = device->write(device->context, BQ25798_I2C_ADDRESS, BQ25798_REG_CHARGER_MASK_0, wanted, sizeof(wanted));
	if(device->last_io_error != 0){
		return BQ25798_IO_ERROR;
	}
	
	bq25798_result_t result = bq25798_read(device, BQ25798_REG_CHARGER_MASK_0, actual, sizeof(actual));
	if(result != BQ25798_OK){
		return result;
	}
	
	for(uint8_t i = 0U; i < sizeof(wanted); i++){
		if(actual[i] != wanted[i]){
			return BQ25798_VERIFY_FAILED;
		}
	}

	return BQ25798_OK;
}

// Read event flags from REG22-REG27. Unlike read_status, those flags are cleared by the read.
bq25798_result_t bq25798_read_events(bq25798_t *device, bq25798_events_t *events){ bq25798_events_t decoded = {0};
	if(events == NULL){
		return BQ25798_NULL_ARGUMENT;
	}
	// read all six latched flag registers
	bq25798_result_t result = bq25798_read(device, BQ25798_REG_CHARGER_FLAG_0, decoded.raw, sizeof(decoded.raw));
	if(result != BQ25798_OK){
		return result;
	}
	
	decoded.charger = bq25798_pack_u32_le(decoded.raw);
	decoded.fault = bq25798_pack_u16_le(&decoded.raw[4]);
	decoded.any_fault = (decoded.fault != 0U);
	decoded.any = (decoded.charger != 0U) || decoded.any_fault;
	*events = decoded;

	return BQ25798_OK;
}

bq25798_result_t bq25798_read_interrupt_masks(bq25798_t *device, bq25798_interrupt_masks_t *masks){
	bq25798_interrupt_masks_t decoded = {0};
	if(masks == NULL){
		return BQ25798_NULL_ARGUMENT;
	}

	bq25798_result_t result = bq25798_read(device, BQ25798_REG_CHARGER_MASK_0, decoded.raw, sizeof(decoded.raw));
	if(result != BQ25798_OK){
		return result;
	}
	
	decoded.charger = bq25798_pack_u32_le(decoded.raw);
	decoded.fault = bq25798_pack_u16_le(&decoded.raw[4]);
	*masks = decoded;
	
	return BQ25798_OK;
}