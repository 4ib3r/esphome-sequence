import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import light, switch
from esphome.const import CONF_ID, CONF_TIMEOUT, CONF_TRIGGER_ID

CODEOWNERS = ["@4ib3r"]
AUTO_LOAD = ["switch", "light"]

sequence_ns = cg.esphome_ns.namespace("sequence")
Sequence = sequence_ns.class_("Sequence", cg.Component)
SequenceStage = sequence_ns.class_("SequenceStage")

AdvanceAction = sequence_ns.class_("AdvanceAction", automation.Action)
OffAction = sequence_ns.class_("OffAction", automation.Action)
ResetAction = sequence_ns.class_("ResetAction", automation.Action)
SetStageAction = sequence_ns.class_("SetStageAction", automation.Action)

CONF_STAGES = "stages"
CONF_SWITCHES = "switches"
CONF_LIGHTS = "lights"
CONF_ON_ENTER = "on_enter"
CONF_ON_OFF = "on_off"
CONF_LOOP = "loop"
CONF_STAGE = "stage"
CONF_STAGE_ID = "stage_id"

STAGE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_STAGE_ID): cv.declare_id(SequenceStage),
        cv.Optional(CONF_SWITCHES, default=[]): cv.ensure_list(
            cv.use_id(switch.Switch)
        ),
        cv.Optional(CONF_LIGHTS, default=[]): cv.ensure_list(
            cv.use_id(light.LightState)
        ),
        cv.Optional(CONF_ON_ENTER): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                    automation.Trigger.template()
                ),
            },
            single=True,
        ),
    }
)


def _validate_stage(config):
    if (
        not config[CONF_SWITCHES]
        and not config[CONF_LIGHTS]
        and CONF_ON_ENTER not in config
    ):
        raise cv.Invalid(
            "each stage needs at least one of: switches, lights, on_enter"
        )
    return config


CONFIG_SCHEMA = cv.ensure_list(
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(Sequence),
            cv.Optional(CONF_TIMEOUT): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_LOOP, default=True): cv.boolean,
            cv.Required(CONF_STAGES): cv.ensure_list(
                cv.All(STAGE_SCHEMA, _validate_stage)
            ),
            cv.Optional(CONF_ON_OFF): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        automation.Trigger.template()
                    ),
                },
                single=True,
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    for conf in config:
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)

        cg.add(var.set_loop(conf[CONF_LOOP]))
        if CONF_TIMEOUT in conf:
            cg.add(var.set_timeout(conf[CONF_TIMEOUT]))

        for stage_conf in conf[CONF_STAGES]:
            stage = cg.new_Pvariable(stage_conf[CONF_STAGE_ID])
            cg.add(var.add_stage(stage))
            for sw_id in stage_conf[CONF_SWITCHES]:
                sw = await cg.get_variable(sw_id)
                cg.add(stage.add_switch(sw))
            for light_id in stage_conf[CONF_LIGHTS]:
                lt = await cg.get_variable(light_id)
                cg.add(stage.add_light(lt))
            if CONF_ON_ENTER in stage_conf:
                enter_conf = stage_conf[CONF_ON_ENTER]
                trigger = cg.new_Pvariable(enter_conf[CONF_TRIGGER_ID])
                await automation.build_automation(trigger, [], enter_conf)
                cg.add(stage.set_on_enter(trigger))

        if CONF_ON_OFF in conf:
            off_conf = conf[CONF_ON_OFF]
            trigger = cg.new_Pvariable(off_conf[CONF_TRIGGER_ID])
            await automation.build_automation(trigger, [], off_conf)
            cg.add(var.set_off_trigger(trigger))


SEQUENCE_ACTION_SCHEMA = automation.maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(Sequence),
    }
)


@automation.register_action(
    "sequence.advance", AdvanceAction, SEQUENCE_ACTION_SCHEMA, synchronous=True
)
async def advance_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "sequence.off", OffAction, SEQUENCE_ACTION_SCHEMA, synchronous=True
)
async def off_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "sequence.reset", ResetAction, SEQUENCE_ACTION_SCHEMA, synchronous=True
)
async def reset_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "sequence.set_stage",
    SetStageAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(Sequence),
            cv.Required(CONF_STAGE): cv.templatable(cv.int_),
        }
    ),
    synchronous=True,
)
async def set_stage_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    template_ = await cg.templatable(config[CONF_STAGE], args, int)
    cg.add(var.set_stage(template_))
    return var
