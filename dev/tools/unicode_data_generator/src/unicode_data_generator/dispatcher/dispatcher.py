from . import context

from ..ucd.manager import UCDManager
from ..ucd.parser import Parser
from ..ucd.code_point_data import CodePointData
from ..datasets.datasets import Dataset, TestDataset
from ..datasets.datasets import available_datasets, available_test_datasets
from ..encoders.encoders import available_encoders
from ..emitter.emitter import Emitter
from ..emitter.test_emitter import TestEmitter

class CommandDispatcher:
    def __init__(self, global_context: context.GlobalContext):
        self.global_context = global_context

    def generate(self, context: context.GenerateContext):
        datasets: set[type[Dataset]] = set()
        test_datasets: set[type[TestDataset]] = set()

        match context.target:
            case 'all':
                datasets = set(available_datasets().values())
                test_datasets = set(available_test_datasets().values())

            case 'tables':
                if context.dataset == 'all':
                    datasets = set(available_datasets().values())
                else:
                    datasets = {available_datasets()[context.dataset]} # type: ignore

            case 'tests':
                if context.dataset == 'all':
                    test_datasets = set(available_test_datasets().values())
                else:
                    test_datasets = {available_test_datasets()[context.dataset]} # type: ignore

        necessary_ucd_files: set[str] = set()

        necessary_ucd_files.update(
            *(dataset.necessary_ucd_files() for dataset in datasets),
            *(test_dataset.necessary_ucd_files() for test_dataset in test_datasets),
        )

        code_point_data = self.get_code_point_data_from_ucd_files(necessary_ucd_files, context.unicode_version)

        sorted_datasets: list[type[Dataset]] = sorted(datasets, key=lambda dataset: dataset.identifier())

        for dataset in sorted_datasets:
            print(f'[*] Generating {dataset.pretty_name()} data')

            d = dataset(code_point_data)

            d.test_data()
            
            encoder = available_encoders()[dataset.optimal_encoder()]
            e = encoder(d, context.use_precomputed_tuning or False, context.unicode_version)

            e.test_data()

            emitter = Emitter(self.global_context.output_dir, context.unicode_version)

            emitter.emit(d, e)

        sorted_test_datasets: list[type[TestDataset]] = sorted(test_datasets, key=lambda test_dataset: test_dataset.identifier())

        for test_dataset in sorted_test_datasets:
            print(f'[*] Generating {test_dataset.pretty_name()} test data')

            d = test_dataset(code_point_data)

            d.test_data()

            emitter = TestEmitter(self.global_context.output_dir, context.unicode_version)

            emitter.emit(d)

            
    def analyze(self, context: context.AnalyzeContext):
        if context.dataset == 'all':
            datasets: set[type[Dataset]] = set(available_datasets().values())
        else:
            datasets: set[type[Dataset]] = {available_datasets()[context.dataset]}

        necessary_ucd_files: set[str] = set()
        necessary_ucd_files.update(*(dataset.necessary_ucd_files() for dataset in datasets))

        code_point_data = self.get_code_point_data_from_ucd_files(necessary_ucd_files, context.unicode_version)

        sorted_datasets: list[type[Dataset]] = sorted(datasets, key=lambda dataset: dataset.identifier())

        for dataset in sorted_datasets:
            d = dataset(code_point_data)
            d.analyze(self.global_context.output_dir, context.unicode_version)


    def get_code_point_data_from_ucd_files(self, files: set[str], unicode_version: str) -> CodePointData:
        parser = Parser(unicode_version, self.global_context.cache_dir)

        if parser.has_cached_data_from_files(files):
            return parser.load_data_from_files_from_cache(files)

        else:
            ucd_manager = UCDManager(unicode_version, self.global_context.cache_dir)

            for filepath in files:
                ucd_manager.load_file(filepath)

            return parser.parse_files(ucd_manager.get_loaded_files())


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
                for dataset_id in sorted(available_datasets().keys()):
                    print(dataset_id)

            case 'tests':
                for test_dataset_id in sorted(available_test_datasets().keys()):
                    print(test_dataset_id)