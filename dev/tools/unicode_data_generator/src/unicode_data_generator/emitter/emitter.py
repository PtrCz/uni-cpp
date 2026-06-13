from pathlib import Path

from .identifiers import header_guard_macro_for_file, namespace_for_dataset
from .integers import format_int_as_hex_with_prefix, get_int_type_name

from ..core.internal_error import internal_error
from ..encoders.interface import Encoder 
from ..datasets.interface import Dataset
from ..core.tables import Table

from ..encoders import (
    multistage_lookup_tables
)

class Emitter:
    def __init__(self, output_dir: Path, unicode_version: str):
        self.unicode_version = unicode_version
        self.output_dir = output_dir / unicode_version / 'cpp'
        self._clear_current_output()


    @property
    def _indent(self) -> str:
        return '    ' * self._indent_level


    def emit(self, dataset: Dataset, encoder: Encoder):
        print(f'[*] Emitting {dataset.pretty_name()} files')

        dataset_dir = self.output_dir / dataset.identifier()

        self._emit_data_hpp_file(dataset, encoder, dataset_dir / 'data.hpp')
        self._emit_data_file(dataset, encoder, dataset_dir / 'data_inline.hpp', False)
        self._emit_data_file(dataset, encoder, dataset_dir / 'data_embed.hpp', True)

        for name, encoded_table in encoder.encoded_tables().tables.items():
            self._emit_embed_data_file(dataset_dir / 'data' / (name + '.dat'), encoded_table)

        for name, extra_table in dataset.extra_tables().tables.items():
            self._emit_embed_data_file(dataset_dir / 'data' / (name + '.dat'), extra_table)


    def _emit_embed_data_file(self, filepath: Path, table: Table):
        bytes_per_value: int = table.optimal_value_size()
        are_values_signed: bool = table.are_values_signed()

        filepath.parent.mkdir(parents=True, exist_ok=True)

        with open(filepath, 'wb') as file:
            file.write(b''.join(
                value.to_bytes(bytes_per_value, 'little', signed=are_values_signed)
                for value in table.values
            ))


    def _emit_data_hpp_file(self, dataset: Dataset, encoder: Encoder, filepath: Path):
        print(f'[*] Emitting file: \'{dataset.identifier()}/data.hpp\'')
        self._clear_current_output()

        self._write_header_comments()
        self._write_header_guard_start(dataset, 'data')
        self._write_line('#include <cstdint>')
        self._write_line()
        self._write_line('#include "../../embed/support.hpp"')
        self._write_line()
        self._write_line('#ifdef UNI_CPP_IMPL_HAS_EMBED')
        self._write_line('#include "data_embed.hpp"')
        self._write_line('#else')
        self._write_line('#include "data_inline.hpp"')
        self._write_line('#endif')
        self._write_line()
        self._write_namespace_start(dataset)

        values = dataset.extra_values()

        for name, value in values.values.items():
            type_name: str = get_int_type_name(value.optimal_size(), value.is_signed())

            self._write_line(f'inline constexpr {type_name} {name} = {format_int_as_hex_with_prefix(value.value)};')

        if len(values.values) != 0:
            self._write_line()

        self._write_lookup_function(dataset, encoder)    

        self._write_namespace_end(dataset)

        self._write_header_guard_end(dataset, 'data')
        self._write_current_output_to_file(filepath)


    def _emit_data_file(self, dataset: Dataset, encoder: Encoder, filepath: Path, use_embed: bool):
        print(f'[*] Emitting file: \'{dataset.identifier()}/{filepath.name}\'')

        def write_table_inline(name: str, table: Table):
            value_type: str = get_int_type_name(table.optimal_value_size(), table.are_values_signed())

            self._write_line(f'inline constexpr std::array<{value_type}, {len(table.values)}> {name}{{')

            self._write_line(f'    {(', '.join(format_int_as_hex_with_prefix(value) for value in table.values))}')

            self._write_line('};')


        def write_table_embed(name: str, table: Table):
            data_file_size: int = table.optimal_value_size() * len(table.values)
            value_type: str = get_int_type_name(table.optimal_value_size(), table.are_values_signed())

            if table.optimal_value_size() == 1 and table.are_values_signed() == False:
                self._write_line(f'inline constexpr std::array<{value_type}, {len(table.values)}> {name}{{')
                self._write_line(f'    #embed "data/{name}.dat"')
                self._write_line('};')

            else:
                parse_embed_call = \
                    f'embed::parse<{value_type}, {data_file_size}>(std::array<std::uint8_t, {data_file_size}>{{'
                
                self._write_line(f'inline constexpr std::array<{value_type}, {len(table.values)}> {name} = {parse_embed_call}')

                self._write_line(f'    #embed "data/{name}.dat"')
                self._write_line('});')


        self._clear_current_output()

        total_size: int = encoder.encoded_tables().total_size() + dataset.extra_tables().total_size()

        self._write_header_comments()
        self._write_line(f'// {dataset.pretty_name().title()} data: {total_size:_} bytes'.replace('_', '\''))
        self._write_line()
        self._write_header_guard_start(dataset, filepath.stem)

        self._write_line('#include <cstdint>')
        self._write_line('#include <array>')
        self._write_line()

        if use_embed:
            self._write_line('#include "../../embed/parse.hpp"')
            self._write_line()
            self._write_line('#include "../../embed/start_embed_code.hpp"')
            self._write_line()

        self._write_namespace_start(dataset)

        tables = [
            *sorted(encoder.encoded_tables().tables.values(), key=lambda table: table.name),
            *sorted(dataset.extra_tables().tables.values(), key=lambda table: table.name),
        ]

        write_table = write_table_embed if use_embed else write_table_inline

        for i, table in enumerate(tables):
            if i != 0:
                self._write_line()

            self._write_line(f'// {table.total_size():_} bytes'.replace('_', '\''))
            write_table(table.name, table)

        self._write_namespace_end(dataset)

        if use_embed:
            self._write_line('#include "../../embed/end_embed_code.hpp"')
            self._write_line()

        self._write_header_guard_end(dataset, filepath.stem)

        self._write_current_output_to_file(filepath)


    def _write_header_comments(self):
        self._write_line(f'// DO NOT EDIT THIS FILE! THIS FILE WAS GENERATED BY `dev/tools/unicode_data_generator`.')

        if self.unicode_version != 'latest':
            self._write_line(f'// Unicode version: {self.unicode_version}')

        self._write_line()


    def _write_header_guard_start(self, dataset: Dataset, file_ident: str):
        macro_ident = header_guard_macro_for_file(dataset, file_ident)
        
        self._write_line(f'#ifndef {macro_ident}')
        self._write_line(f'#define {macro_ident}')
        self._write_line()


    def _write_header_guard_end(self, dataset: Dataset, file_ident: str):
        macro_ident = header_guard_macro_for_file(dataset, file_ident)

        self._write_line(f'#endif // {macro_ident}')


    def _write_namespace_start(self, dataset: Dataset):
        self._write_line(f'namespace {namespace_for_dataset(dataset)}')
        self._write_line('{')

        self._indent_level += 1


    def _write_namespace_end(self, dataset: Dataset):
        self._indent_level -= 1

        self._write_line(f'}} // namespace {namespace_for_dataset(dataset)}')
        self._write_line()


    def _write_lookup_function(self, dataset: Dataset, encoder: Encoder):
        data = dataset.primary_data()

        property_type_name: str = get_int_type_name(data.optimal_value_size(), data.are_values_signed())

        self._write_line(f'[[nodiscard]] constexpr {property_type_name} lookup(const std::uint32_t code_point) noexcept')
        self._write_line('{')

        self._indent_level += 1

        if type(encoder) is multistage_lookup_tables.MultistageLookupTables:

            self._write_line('// See `dev/docs/multistage-lookup-tables.md`.')
            self._write_line()
            self._write_line(f'const std::uint32_t quot = code_point / {encoder.block_size};')
            self._write_line(f'const std::uint32_t rem  = code_point % {encoder.block_size};')
            self._write_line()
            
            if encoder.stage1_needs_extra_lookup:
                self._write_line('const std::uint32_t stage2_offset = stage2_offsets[stage1[quot]];')
            else:
                self._write_line('const std::uint32_t stage2_offset = stage1[quot];')
            
            self._write_line()

            if encoder.stage2_holds_property_values_inplace:
                self._write_line('return stage2[stage2_offset + rem];')
            else:
                self._write_line('const auto stage2_value = stage2[stage2_offset + rem];')
                self._write_line('return stage3[stage2_value];')

        else:
            internal_error('Failed to emit a lookup function for an unknown encoder!')

        self._indent_level -= 1
        self._write_line('}')


    def _write_current_output_to_file(self, filepath: Path):
        filepath.parent.mkdir(parents=True, exist_ok=True)

        with open(filepath, 'w', encoding='utf-8') as file:
            file.write(self._current_output)


    def _write_line(self, line: str | None = None):
        if line is None or len(line.strip()) == 0:
            self._current_output += '\n'

        else:
            self._current_output += f'{self._indent}{line}\n'


    def _clear_current_output(self):
        self._current_output: str = ''
        self._indent_level: int = 0