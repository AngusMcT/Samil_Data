import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor

from . import SamilData

DEPENDENCIES = ["samil"]

CONF_SAMIL_ID = "samil_id"
CONF_SENSOR_TYPE = "sensor_type"

SENSOR_TYPES = {
    "temperature": {
        "name": "Samil Temp",
        "unit": "°C",
        "accuracy": 1,
        "device_class": "temperature",
        "state_class": "measurement",
    },
    "solar_voltage": {
        "name": "Samil Solar Voltage",
        "unit": "V",
        "accuracy": 1,
        "device_class": "voltage",
        "state_class": "measurement",
    },
    "solar_current": {
        "name": "Samil Solar Power",
        "unit": "A",
        "accuracy": 2,
        "device_class": "current",
        "state_class": "measurement",
    },
    "energy_today": {
        "name": "Samil Energy Today",
        "unit": "kWh",
        "accuracy": 2,
        "device_class": "energy",
        "state_class": "total_increasing",
    },
    "grid_current": {
        "name": "Samil Grid Power",
        "unit": "A",
        "accuracy": 2,
        "device_class": "current",
        "state_class": "measurement",
    },
    "grid_voltage": {
        "name": "Samil Grid Voltage",
        "unit": "V",
        "accuracy": 1,
        "device_class": "voltage",
        "state_class": "measurement",
    },
    "grid_frequency": {
        "name": "Samil Grid Frequency",
        "unit": "Hz",
        "accuracy": 2,
        "device_class": "frequency",
        "state_class": "measurement",
    },
    "solar_output": {
        "name": "Samil Solar Output",
        "unit": "W",
        "accuracy": 0,
        "device_class": "power",
        "state_class": "measurement",
    },
    "online": {
        "name": "Samil Online",
        "unit": None,
        "accuracy": 0,
        "device_class": None,
        "state_class": None,
    },
}

CONFIG_SCHEMA = sensor.sensor_schema(
    None,
    entity_category=None,
).extend(
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

    sensor_type = config[CONF_SENSOR_TYPE]

    if sensor_type == "temperature":
        cg.add(parent.set_samil_temp_sensor(var))

    elif sensor_type == "solar_voltage":
        cg.add(parent.set_samil_vpv_sensor(var))

    elif sensor_type == "solar_current":
        cg.add(parent.set_samil_ipv_sensor(var))

    elif sensor_type == "energy_today":
        cg.add(parent.set_samil_e_day_sensor(var))

    elif sensor_type == "grid_current":
        cg.add(parent.set_samil_iac_sensor(var))

    elif sensor_type == "grid_voltage":
        cg.add(parent.set_samil_vac_sensor(var))

    elif sensor_type == "grid_frequency":
        cg.add(parent.set_samil_fac_sensor(var))

    elif sensor_type == "solar_output":
        cg.add(parent.set_samil_pac_sensor(var))

    elif sensor_type == "online":
        cg.add(parent.set_samil_online_sensor(var))
