import operator
from typing import Annotated, TypedDict, List, Dict
from langchain_core.prompts import ChatPromptTemplate
from langgraph.graph import StateGraph, START, END
from langgraph.types import Send

from libs.model import LedTask, LedTaskPlans, LedTaskResult, LedCommandList
from libs.prompts import LEDCMD_SYSTEM_MSG, LEDCMD_HUMAN_QUERY, LEDPLAN_SYSTEM_MSG, LEDPLAN_HUMAN_QUERY


class AgentState(TypedDict):
    """Agnet's overall state"""
    query: str
    num_of_leds: int
    tasks: Annotated[List[LedTask], operator.add]
    results: Annotated[List[LedTaskResult], operator.add]
    final_result: List[LedCommandList]


class SubtaskState(TypedDict):
    """A subtask states for parallel execution"""
    num_of_leds: int
    task: LedTask


class LedCommandGraph:
    def __init__(self, bedrock_chat):
        self.graph = self.create_graph(bedrock_chat)

    def graph(self):
        return self.graph

    def create_graph(self, bedrock_chat):
        ####################
        # Graph nodes
        ####################
        def create_plan(state: AgentState) -> AgentState:
            """Task planning: break down subtasks for complete the task of user input"""
            prompt = ChatPromptTemplate.from_messages([
                ("system", LEDPLAN_SYSTEM_MSG),
                ("human", LEDPLAN_HUMAN_QUERY),
            ])
            chain = prompt | bedrock_chat.with_structured_output(LedTaskPlans)
            response = chain.invoke({"query": state["query"], "num_of_leds": state["num_of_leds"]})
            return {"tasks": response.subtasks}

        def create_led_command_task(state: SubtaskState) -> Dict:
            """create LED commands: generate control command for subtask"""
            prompt = ChatPromptTemplate.from_messages([
                ("system", LEDCMD_SYSTEM_MSG),
                ("human", LEDCMD_HUMAN_QUERY),
            ])
            chain = prompt | bedrock_chat.with_structured_output(LedCommandList)
            response = chain.invoke({"query": state["task"].action, "num_of_leds": state["num_of_leds"]})
            return {"results": [{"taskid": state["task"].taskid, "result": response}]}

        def continue_to_led_command_task(state: AgentState):
            """Task distrubution: task distribution for parallel processing"""
            # Create parallel branches for evaluating each solution
            branches = [Send("execute_tasks", {"num_of_leds": state["num_of_leds"], "task":s}) for s in state["tasks"]]
            return branches

        def combine_results(state: AgentState) -> AgentState:
            """Aggregation: re-order LED commands sequencely"""
            sorted_results = sorted(state["results"], key=lambda x: x["taskid"])
            return {"final_result": sorted_results}

        ####################
        # Build Graph
        ####################
        # 상태 그래프 생성
        workflow = StateGraph(AgentState)
        # 작업 계획 노드 추가
        workflow.add_node("create_plan", create_plan)
        # 병렬 실행 노드 추가
        workflow.add_node("execute_tasks", create_led_command_task)
        # 결과 취합 노드 추가
        workflow.add_node("combine_results", combine_results)
        # 엣지 연결
        workflow.add_edge(START, "create_plan")
        workflow.add_conditional_edges("create_plan", continue_to_led_command_task, ["execute_tasks"])
        workflow.add_edge("execute_tasks", "combine_results")
        workflow.add_edge("combine_results", END)
        # 그래프 컴파일
        graph = workflow.compile()
        return graph

    def invoke(self, num_of_leds, query):
        initial_state = {
            "query": query,
            "num_of_leds": num_of_leds,
            "tasks": [],
            "results": [],
            "final_result": ""
        }
        response = self.graph.invoke(initial_state)
        result = LedCommandList(commands=[])
        for c in response['final_result']:
            result.commands += c['result'].commands
        return result

