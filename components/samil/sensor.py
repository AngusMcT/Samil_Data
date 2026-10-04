import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor

from . import SamilData


CONF_SAMIL_ID = "samil_id"
CONF_SENSOR_TYPE = "sensor_type"


SENSOR_TYPES = {
    "temperature": "set_samil_temp_sensor",
    "solar_voltage": "set_samil_vpv_sensor",
    "solar_current": "set_samil_ipv_sensor",
    "energy_today": "set_samil_e_day_sensor",
    "grid_current": "set_samil_iac_sensor",
    "grid_voltage": "set_samil_vac_sensor",
    "grid_frequency": "set_samil_fac_sensor",
    "solar_output": "set_samil_pac_sensor",
    "online": "set_samil_online_sensor",
}


CONFIG_SCHEMA = sensor.sensor_schema().extend(
    {
        cv.GenerateID(CONF_SAMIL_ID): cv.use_id(SamilData),
        cv.Required(CONF_SENSOR_TYPE): cv.enum(
            SENSOR_TYPES,
            lower=True,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SAMIL_ID])
    var = await sensor.new_sensor(config)

    setter = SENSOR_TYPES[config[CONF_SENSOR_TYPE]]
    cg.add(getattr(parent, setter)(var))
    
