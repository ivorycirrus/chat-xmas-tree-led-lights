from typing import List, Union
from pydantic import BaseModel

#####################################
# LED Commands
#####################################

class SetCommand(BaseModel):
    """The 'set' command changes color of LED lights. Change the color of the LED between 'led_id_from' and 'led_id_to'. 'color' means the value of [red, green, blue], and each color has a value between 0 and 255."""
    cmd: str  # "set"
    led_id_from: int
    led_id_to: int
    color: List[int]

class SleepCommand(BaseModel):
    """The 'sleep' command allows how much time waited. 'milis' means how long you have to wait in miliseconds"""
    cmd: str  # "sleep"
    milis: int

class RepeatCommand(BaseModel):
    """The 'repeat' command sets whether to repeat the entire list of commands. This command can only be used once at the end of the entire command list, and is ignored if defined in the middle."""
    cmd: str  # "repeat"
    repeat: bool

# 전체 명령 리스트의 타입 정의
class LedCommandList(BaseModel):
    """A super set of LED command lists."""
    commands: List[Union[SetCommand, SleepCommand, RepeatCommand]]

#####################################
# LED Task Plans
#####################################

class LedTask(BaseModel):
    """A subtask to create LED commands."""
    taskid: int
    action: str

class LedTaskPlans(BaseModel):
    """List of task plans to controlling LEDs. It contains subtasks."""
    subtasks: List[LedTask]

class LedTaskResult(BaseModel):
    """A results of subtask. It contains subtask id and LED commands"""
    taskid: int
    result: LedCommandList