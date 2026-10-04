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

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SamilData),
        cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
    }
).extend(cv.polling_component_schema("30s"))


async def to_code(config):
    parent = await cg.get_variable(config[CONF_UART_ID])

    var = cg.new_Pvariable(config[cv.CONF_ID], parent)

    await cg.register_component(var, config)
