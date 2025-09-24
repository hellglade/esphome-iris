import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import cc1101
from esphome.components import gpio
from esphome.const import CONF_ID, CONF_ADDRESS

CODEOWNERS = ["@swoboda1337"]
MULTI_CONF = True
CONF_COMMAND = "command"
CONF_MODE = "mode"
CONF_GDO0 = "gdo0"
CONF_EMITTER = "emitter"

iris_ns = cg.esphome_ns.namespace("iris")
IrisComponent = iris_ns.class_("IrisComponent", cg.Component)
IrisSendCommandAction = iris_ns.class_("IrisSendCommandAction", automation.Action)

IrisCommand = iris_ns.enum("IrisCommand")
IRIS_COMMAND = {
    "POWER": IrisCommand.IRIS_POWER,
    "BLUE": IrisCommand.IRIS_BLUE,
    "MAGENTA": IrisCommand.IRIS_MAGENTA,
    "RED": IrisCommand.IRIS_RED,
    "LIME": IrisCommand.IRIS_LIME,
    "GREEN": IrisCommand.IRIS_GREEN,
    "AQUA": IrisCommand.IRIS_AQUA,
    "WHITE": IrisCommand.IRIS_WHITE,
    "MODE1": IrisCommand.IRIS_MODE1,
    "MODE2": IrisCommand.IRIS_MODE2,
    "MODE3": IrisCommand.IRIS_MODE3,
    "MODE4": IrisCommand.IRIS_MODE4,
    "BRIGHTNESS": IrisCommand.IRIS_BRIGHTNESS,
}

IrisMode = iris_ns.enum("IrisMode")
IRIS_MODE = {
    "POOL": IrisMode.IRIS_POOL,
    "SPA": IrisMode.IRIS_SPA,
    "POOLSPA": IrisMode.IRIS_POOLSPA,
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(IrisComponent),
        cv.Required(CONF_ADDRESS): cv.hex_uint16_t,
        cv.Required(CONF_GDO0): cv.pin,
        cv.Required(CONF_EMITTER): cv.pin,
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_address(config[CONF_ADDRESS]))
    gdo0 = await cg.gpio_pin_expression(config[CONF_GDO0])
    cg.add(var.set_config_gdo0(gdo0))
    if CONF_EMITTER in config:
        emitter = await cg.gpio_pin_expression(config[CONF_EMITTER])
        cg.add(var.set_config_emitter(emitter))    


@automation.register_action(
    "iris.send_command",
    IrisSendCommandAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(IrisComponent),
            cv.Required(CONF_COMMAND): cv.templatable(cv.enum(IRIS_COMMAND, upper=True)),
            cv.Required(CONF_MODE): cv.templatable(cv.enum(IRIS_MODE, upper=True)),
        }
    ),
)
async def iris_send_command_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    command = await cg.templatable(config[CONF_COMMAND], args, IrisCommand)
    mode = await cg.templatable(config[CONF_MODE], args, IrisMode)
    cg.add(var.set_command(command))
    cg.add(var.set_mode(mode))
    return var
