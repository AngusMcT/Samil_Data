import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor

from . import SamilData

DEPENDENCIES = ["samil"]

CONF_SAMIL_ID = "samil_id"
CONF_SENSOR_TYPE = "sensor_type"

SENSOR_TYPES = {
    "temperature": (
        "set_samil_temp_sensor",
        "Samil Temp",
        "°C",
        1,
    ),
    "solar_voltage": (
        "set_samil_vpv_sensor",
        "Samil Solar Voltage",
        "V",
        1,
    ),
    "solar_current": (
        "set_samil_ipv_sensor",
        "Samil Solar Power",
        "A",
        2,
    ),
    "energy_today": (
        "set_samil_e_day_sensor",
        "Samil Energy Today",
        "kWh",
        2,
    ),
    "grid_current": (
        "set_samil_iac_sensor",
        "Samil Grid Power",
        "A",
        2,
    ),
    "grid_voltage": (
        "set_samil_vac_sensor",
        "Samil Grid Voltage",
        "V",
        1,
    ),
    "grid_frequency": (
        "set_samil_fac_sensor",
        "Samil Grid Frequency",
        "Hz",
        2,
    ),
    "solar_output": (
        "set_samil_pac_sensor",
        "Samil Solar Output",
        "W",
        0,
    ),
    "online": (
        "set_samil_online_sensor",
        "Samil Online",
        None,
        0,
    ),
}


CONFIG_SCHEMA = sensor.sensor_schema().extend(
    {
        cv.GenerateID(CONF_SAMIL_ID): cv.use_id(SamilData),
        cv.Required(CONF_SENSOR_TYPE): cv.enum(
            {key: key for key in SENSOR_TYPES},
            lower=True,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SAMIL_ID])
    var = await sensor.new_sensor(config)

    setter, default_name, default_unit, default_accuracy = SENSOR_TYPES[
        config[CONF_SENSOR_TYPE]
    ]

    cg.add(getattr(parent, setter)(var))
