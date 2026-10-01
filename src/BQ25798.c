#include "BQ25798.h"
#include <stddef.h>

// Read an unsigned big-endian integer from an unaligned byte buffer
static uint16_t bq25798_read_u16_be(const uint8_t *data){
	return (uint16_t)(((uint16_t)data[0] << 8U) | ((uint16_t)data[1]));
}

static bq25798_result_t bq25798_read(bq25798_t *device, uint8_t reg, uint8_t *data, uint16_t length){
	if(device == NULL || device->read == NULL || data == NULL){
		return BQ25798_NULL_ARGUMENT;
	}
	
	device->last_io_error = device->read(device->context, BQ25798_I2C_ADDRESS, reg, data, length);
	
	return device->last_io_error == 0 ? BQ25798_OK : BQ25798_IO_ERROR;
}

static bool bq25798_is_u16_register(uint8_t reg){
	return reg == 0x01U || 
		   reg == 0x03U || 
		   reg == 0x06U ||
		   reg == 0x0BU || 
		   reg == 0x19U ||
		   (reg >= 0x31U && reg <= 0x45U && (reg & 1U) != 0U);
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
	
	decoded.cell_count = (uint8_t)((decoded.raw[0x0AU] >> 6U) + 1U);
	decoded.minimum_system_mv = (uint16_t)(2500U + (decoded.raw[0] & 0x3FU) * 250U);
	decoded.charge_voltage_mv = (uint16_t)((bq25798_read_u16_be(&decoded.raw[1]) & 0x7FFU) * 10U);
	decoded.charge_current_ma = (uint16_t)((bq25798_read_u16_be(&decoded.raw[3]) & 0x1FFU) * 10U);
	decoded.input_voltage_limit_mv = (uint16_t)(decoded.raw[5] * 100U);
	decoded.input_current_limit_ma = (uint16_t)((bq25798_read_u16_be(&decoded.raw[6]) & 0x1FFU) * 10U);
	
	*configuration = decoded;

	return BQ25798_OK;
}
