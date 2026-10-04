import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import uart

DEPENDENCIES = ["uart"]

samil_ns = cg.esphome_ns.namespace("samil")

SamilData = samil_ns.class_(
    "SamilData",
    cg.PollingComponent,
    uart.UARTDevice,
)

CONF_UART_ID = "uart_id"

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(SamilData),
    })
    .extend(cv.polling_component_schema("30s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[cv.CONF_ID])

    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
