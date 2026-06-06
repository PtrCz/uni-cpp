from . import context

from ..core import datasets
from ..ucd.manager import UCDManager
from ..ucd.parser import Parser
from ..datasets.datasets import available_datasets
from ..encoders.encoders import available_encoders
from ..emitter.emitter import Emitter

class CommandDispatcher:
    def __init__(self, global_context: context.GlobalContext):
        self.global_context = global_context

    def generate(self, context: context.GenerateContext):
        datasets_: set[datasets.Dataset] = set()
        test_datasets: set[datasets.Dataset] = set()

        match context.target:
            case 'all':
                datasets_ = set(datasets.datasets().values())
                test_datasets = set(datasets.test_datasets().values())

            case 'tables':
                if context.dataset == 'all':
                    datasets_ = set(datasets.datasets().values())
                else:
                    datasets_ = {datasets.datasets()[context.dataset]} # type: ignore

            case 'tests':
                if context.dataset == 'all':
                    test_datasets = set(datasets.test_datasets().values())
                else:
                    test_datasets = {datasets.test_datasets()[context.dataset]} # type: ignore

        necessary_ucd_files: set[str] = set()

        necessary_ucd_files.update(
            *(dataset.necessary_ucd_files for dataset in datasets_),
            *(dataset.necessary_ucd_files for dataset in test_datasets),
        )

        parser = Parser(context.unicode_version, self.global_context.cache_dir)

        if parser.has_cached_data_from_files(necessary_ucd_files):
            code_point_data = parser.load_data_from_files_from_cache(necessary_ucd_files)

        else:
            ucd_manager = UCDManager(context.unicode_version, self.global_context.cache_dir)

            for filepath in necessary_ucd_files:
                ucd_manager.load_file(filepath)

            code_point_data = parser.parse_files(ucd_manager.get_loaded_files())

        ds = {available_datasets()[ds.name] for ds in datasets_}
        
        for dataset in ds:
            print(f'[*] Generating {dataset.pretty_name()} data')

            d = dataset(code_point_data)

            d.test_data()
            
            encoder = available_encoders()['multistage_lookup_tables']
            e = encoder(d, context.use_precomputed_tuning or False, context.unicode_version)

            tables = e.encoded_tables()
            e.test_data()

            emitter = Emitter(self.global_context.output_dir, context.unicode_version)

            emitter.emit(d, e)

            
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
                for dataset in datasets.datasets().keys():
                    print(dataset)

            case 'tests':
                for test in datasets.test_datasets().keys():
                    print(test)