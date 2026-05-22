from . import context

from ..core import datasets

class CommandDispatcher:
    def __init__(self, global_context: context.GlobalContext):
        self.global_context = global_context

    def generate(self, context: context.GenerateContext):
        print(f'Generate: {context}')

    def analyze(self, context: context.AnalyzeContext):
        print(f'Analyze: {context}')

    def clean(self, context: context.CleanContext):
        def delete_dir(name: str, path):
            print(f'[*] Removing the {name} directory: {path}')

            import shutil

            try:
                shutil.rmtree(path)
            except OSError:
                print(f'[-] Failed to remove the {name} directory')

        def get_dir(path, unicode_version: str | None):
            if unicode_version is None:
                return path
            else:
                return path / unicode_version

        cache_dir = get_dir(self.global_context.cache_dir, context.unicode_version)
        output_dir = get_dir(self.global_context.output_dir, context.unicode_version)

        match context.target:
            case 'all':
                delete_dir('cache', cache_dir)
                delete_dir('output', output_dir)
            
            case 'cache':
                delete_dir('cache', cache_dir)

            case 'output':
                delete_dir('output', output_dir)


    def list(self, context: context.ListContext):
        match context.target:
            case 'datasets':
                for dataset in datasets.datasets():
                    print(dataset.name)

            case 'tests':
                for test in datasets.test_datasets():
                    print(test.name)