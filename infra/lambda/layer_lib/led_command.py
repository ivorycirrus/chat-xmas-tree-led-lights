import boto3
from langchain_aws.chat_models import ChatBedrockConverse, ChatBedrock
from langchain_core.prompts import ChatPromptTemplate
from libs.prompts import LEDCMD_SYSTEM_MSG, LEDCMD_HUMAN_QUERY, LEDPLAN_SYSTEM_MSG, LEDPLAN_HUMAN_QUERY
from libs.model import LedCommandList, LedTaskPlans
from libs.agent import LedCommandGraph


class LedCommandService:
    def __init__(self, model_id):
        session = boto3.Session()
        bedrock_client = session.client(service_name='bedrock-runtime')
        bedrock_chat_model = model_id
        # self.bedrock_chat = ChatBedrockConverse(
        #     client=bedrock_client,
        #     model_id=bedrock_chat_model,
        #     temperature=0.3,
        #     max_tokens=4096
        # )
        self.bedrock_chat = ChatBedrock(
            client=bedrock_client,
            model_id=bedrock_chat_model,
            temperature=0.3,
            max_tokens=4096
        )

    def simple_led_command(self, num_of_leds, query):
        template = ChatPromptTemplate.from_messages([
            ("system", LEDCMD_SYSTEM_MSG),
            ("human", LEDCMD_HUMAN_QUERY),
        ])
        chain = template | self.bedrock_chat.with_structured_output(LedCommandList)
        response = chain.invoke({"num_of_leds": num_of_leds, "query": query})
        return response.model_dump()

    def simple_led_command_without_types(self, num_of_leds, query):
        template = ChatPromptTemplate.from_messages([
            ("system", LEDCMD_SYSTEM_MSG),
            ("human", LEDCMD_HUMAN_QUERY),
        ])
        messages = template.format_messages(num_of_leds=num_of_leds, query=query)
        response = self.bedrock_chat.invoke(messages)
        return response.content

    def simple_led_plan(self, num_of_leds, query):
        template = ChatPromptTemplate.from_messages([
            ("system", LEDPLAN_SYSTEM_MSG),
            ("human", LEDPLAN_HUMAN_QUERY),
        ])
        chain = template | self.bedrock_chat.with_structured_output(LedTaskPlans)
        response = chain.invoke({"num_of_leds": num_of_leds, "query": query})
        return response.model_dump()

    def led_command_with_planning_agent(self, num_of_leds, query):
        graph = LedCommandGraph(self.bedrock_chat)
        result = graph.invoke(num_of_leds, query)
        return result.model_dump()
