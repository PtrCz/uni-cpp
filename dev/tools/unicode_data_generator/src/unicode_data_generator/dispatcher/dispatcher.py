from . import context

class CommandDispatcher:
    def __init__(self, global_context: context.GlobalContext):
        self.global_context = global_context
        print(f'Global: {self.global_context}')

    def generate(self, context: context.GenerateContext):
        print(f'Generate: {context}')

    def analyze(self, context: context.AnalyzeContext):
        print(f'Analyze: {context}')

    def clean(self, context: context.CleanContext):
        print(f'Clean: {context}')

    def list(self, context: context.ListContext):
        print(f'List: {context}')