from esphome import automation
import esphome.codegen as cg
from esphome.components import remote_transmitter
from esphome.components import remote_receiver
from esphome.components import cc1101
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_ADDRESS

CODEOWNERS = ["@swoboda1337"]
DEPENDENCIES = ["remote_transmitter", "remote_receiver"]
MULTI_CONF = True
CONF_TRANSMITTER_ID = "transmitter_id"
CONF_RECEIVER_ID = "receiver_id"
CONF_COMMAND = "command"
CONF_MODE = "mode"
CONF_CC1101_ID = "cc1101_id"

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
        cv.Required(CONF_CC1101_ID): cv.use_id(cc1101.CC1101Component),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cc1101_comp = await cg.get_variable(config[CONF_CC1101_ID])
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_cc1101(cc1101_comp))

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
