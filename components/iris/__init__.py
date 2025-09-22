from esphome import automation
import esphome.codegen as cg
from esphome.components import remote_transmitter
from esphome.components import remote_receiver
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_ADDRESS

CODEOWNERS = ["@swoboda1337"]
DEPENDENCIES = ["remote_transmitter", "remote_receiver"]
MULTI_CONF = True
CONF_TRANSMITTER_ID = "transmitter_id"
CONF_RECEIVER_ID = "receiver_id"
CONF_COMMAND = "command"
CONF_MODE = "mode"
CONF_REPEAT = "repeat"

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
        cv.GenerateID(CONF_TRANSMITTER_ID): cv.use_id(
            remote_transmitter.RemoteTransmitterComponent
        ),
        cv.GenerateID(CONF_RECEIVER_ID): cv.use_id(
            remote_receiver.RemoteReceiverComponent
        ),
        cv.Required(CONF_ADDRESS): cv.hex_uint16_t,
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    receiver = await cg.get_variable(config[CONF_RECEIVER_ID])
    cg.add(var.set_tx(transmitter))
    cg.add(var.set_rx(receiver))
    cg.add(var.set_address(config[CONF_ADDRESS]))

@automation.register_action(
    "iris.send_command",
    IrisSendCommandAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(IrisComponent),
            cv.Required(CONF_COMMAND): cv.templatable(cv.enum(IRIS_COMMAND, upper=True)),
            cv.Required(CONF_MODE): cv.templatable(cv.enum(IRIS_MODE, upper=True)),
            cv.Optional(CONF_REPEAT): cv.templatable(cv.int_range(min=0, max=5)),
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
    if CONF_REPEAT in config:
        repeat = await cg.templatable(config[CONF_REPEAT], args, cg.uint32)
        cg.add(var.set_repeat(repeat))
    return var

# Remove the following block unless you implement IrisSetCodeAction in C++
# @automation.register_action(
#     "iris.set_code",
#     IrisSetCodeAction,
#     cv.Schema(
#         {
#             cv.Required(CONF_ID): cv.use_id(IrisComponent),
#         }
#     ),
# )
# async def iris_set_code_to_code(config, action_id, template_arg, args):
#     paren = await cg.get_variable(config[CONF_ID])
#     var = cg.new_Pvariable(action_id, template_arg, paren)
#     return var
