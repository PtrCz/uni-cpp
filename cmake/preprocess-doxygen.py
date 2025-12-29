import argparse
from pathlib import Path

"""
This is a very simple preprocessor we use on source files before they are processed by Doxygen.

It's used here to hide implementation details from the docs in an easy way.

It allows private inheritance from classes in an `impl` namespace to be excluded from Doxygen's inheritance diagrams and the 'Class Hierarchy' page.
It is more maintainable than using `\\copydoc`, `\\cond` and `\\endcond`.

Note: \\hideinheritancegraph and INLINE_INHERITED_MEMB are not enough, because doxygen still shows the implementation details in the 'Class Hierarchy' page etc.

These are the currently available commands:
- PREPROCESSOR_START
- START {NAME}
- END {NAME}
- PASTE {NAME}
- HIDE_INHERITANCE

Commands are intended to be used in source code comments. Each command must be prefixed with "DOXYGEN-PREPROCESSOR: ". Only one command per line is allowed.
Lines with commands are removed from the output except for the HIDE_INHERITANCE command.

Preprocessing begins when the PREPROCESSOR_START command is used. This command can appear at most once per file.

START, END, PASTE commands:
- START {NAME} starts a named block of code.
- END {NAME} ends a named block of code.
- PASTE {NAME} inserts the contents of a previously defined named block.

Note 1: The lines between the START and END commands are not removed from the output.
Note 2: Nested START and END blocks are not supported. Using any commands inside of the START and END blocks is not supported. The only valid command after START is the matching END.

Using named blocks allows reusing documentation without having to use `\\copydoc` and without having to redeclare the functions.
It improves the code and documentation maintainability.

HIDE_INHERITANCE:
Use this command on a line that has a class inheritance list, i.e. the line that contains a colon (`:`) and the list of base classes.
This command removes the inheritance from the Doxygen output. It removes the colon and everything after it.

To keep this preprocessor simple, it assumes that all the above listed rules are followed.

Example input file:

```cpp

// DOXYGEN-PREPROCESSOR: PREPROCESSOR_START

namespace impl
{
    class Base
    {
    protected:
        // DOXYGEN-PREPROCESSOR: START Base

        /// @brief does the foo thing
        ///
        void foo();

        // DOXYGEN-PREPROCESSOR: END Base

        void foo()
        {
            // do the foo thing
        }
    };
} // namespace impl

class Derived : private impl::Base // DOXYGEN-PREPROCESSOR: HIDE_INHERITANCE
{
public:
#ifndef UNI_CPP_DOXYGEN
    using impl::Base::foo;
#endif

    // DOXYGEN-PREPROCESSOR: PASTE Base
};

```

After preprocessing:

```cpp

namespace impl
{
    class Base
    {
    protected:
        /// @brief does the foo thing
        ///
        void foo();

        void foo()
        {
            // do the foo thing
        }
    };
} // namespace impl

class Derived
#ifndef UNI_CPP_DOXYGEN
    : private impl::Base
#endif
{
public:
#ifndef UNI_CPP_DOXYGEN
    using impl::Base::foo;
#endif

#ifdef UNI_CPP_DOXYGEN

    /// @brief does the foo thing
    ///
    void foo();

#endif // UNI_CPP_DOXYGEN
};

```

"""

COMMAND_PREFIX = 'DOXYGEN-PREPROCESSOR:'
VALID_COMMANDS = {'PREPROCESSOR_START', 'START', 'END', 'PASTE', 'HIDE_INHERITANCE'}

def line_has_command(line: str) -> bool:
    return COMMAND_PREFIX in line

def get_command_name(line: str) -> str:
    if not line_has_command(line):
        raise ValueError(f'Internal error! Call to \'get_command_name\' with a line that is not a command.')

    command_index = line.find(COMMAND_PREFIX) + len(COMMAND_PREFIX)
    command = line[command_index:].strip()

    space_index = command.find(' ')

    command_name = command if space_index == -1 else command[:space_index]

    if command_name not in VALID_COMMANDS:
        raise ValueError(f'Invalid command: \'{command_name}\'. Must be one of {VALID_COMMANDS}') 

    return command_name

def get_block_name(line: str) -> str:

    command_name = get_command_name(line)

    command_index = line.find(COMMAND_PREFIX) + len(COMMAND_PREFIX)
    command = line[command_index:].strip()
    
    block_name = command[len(command_name):].strip()

    space_index = block_name.find(' ')

    name = block_name if space_index == -1 else block_name[:space_index]

    if len(name) == 0:
        raise ValueError(f'\'{line}\': Name must not be empty!')

    return name

def remove_comments(line: str) -> str:
    # Note: for simplicity it assumes that there are no string literals in this line

    result = ''
    i = 0

    while i < len(line):
        if line[i:i+2] == '//':
            break

        elif line[i:i+2] == '/*':
            i += 2 # '/*'

            while i < len(line) - 1 and line[i:i+2] != '*/':
                i += 1

            i += 2 # '*/'

        else:
            result += line[i]
            i += 1

    return result

def hide_inheritance_from_line(original_line: str) -> list[str]:
    line = remove_comments(original_line)
    colon_index = line.find(':')

    if colon_index == -1:
        raise ValueError(f'\'{original_line}\': HIDE_INHERITANCE command used on a line without a colon')
    
    before_colon = line[:colon_index]

    output_lines: list[str] = [] if before_colon.strip() == '' else [before_colon]

    output_lines.extend([
        '#ifndef UNI_CPP_DOXYGEN',
        line[colon_index:], # contents after and including the colon
        '#endif'
    ])

    return output_lines

def preprocess(input: Path) -> list[str]:
    lines: list[str] = input.read_text(encoding='utf-8').splitlines()
    output_lines: list[str] = []

    # Copy all the lines before "DOXYGEN-PREPROCESSOR: PREPROCESSOR_START" (possibly all)
    while len(lines) != 0:
        line = lines.pop(0)

        if line_has_command(line) and get_command_name(line) == 'PREPROCESSOR_START':
            break

        output_lines.append(line)

    blocks: dict[str, list[str]] = {}
    current_block: list[str] | None = None
    current_block_name: str | None = None

    # Preprocess
    for line in lines:

        if line_has_command(line):
            command = get_command_name(line)

            if command == 'PASTE':
                block_name = get_block_name(line)

                if block_name not in blocks:
                    raise ValueError(f'Block \'{block_name}\' not defined before PASTE command')

                output_lines.append('#ifdef UNI_CPP_DOXYGEN')
                output_lines.append('')
                output_lines.extend(blocks[block_name])
                output_lines.append('')
                output_lines.append('#endif // UNI_CPP_DOXYGEN')
                
            elif command == 'HIDE_INHERITANCE':
                output_lines.extend(hide_inheritance_from_line(line))

            elif command == 'START':
                current_block = []
                current_block_name = get_block_name(line)

            elif command == 'END':
                block_name = get_block_name(line)

                if block_name != current_block_name:
                    raise ValueError(f'END command with the name \'{block_name}\' doesn\'t match the previous name (\'{current_block_name}\')!')

                blocks[current_block_name] = current_block
                current_block = None
                current_block_name = None

        else:
            if current_block is not None:
                current_block.append(line)

            output_lines.append(line)

    return output_lines

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='uni-cpp doxygen preprocessor')
    parser.add_argument('input', type=Path, help='Input file path')
    parser.add_argument('output', type=Path, help='Output file path')
    args = parser.parse_args()

    lines: list[str] = preprocess(args.input)

    output_filepath: Path = args.output

    output_filepath.write_text(data='\n'.join(lines), encoding='utf-8')