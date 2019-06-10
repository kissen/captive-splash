#! /usr/bin/env python3


from pathlib import Path
from sys import argv, stderr


class Binary:
    def __init__(self, file_path: str, object_name: str=None):
        self.file_path = file_path

        if object_name is None:
            self.object_name = self.__generate_object_name()
        else:
            self.object_name = object_name

    def __generate_object_name(self) -> str:
        filename = Path(self.file_path).resolve().stem
        filename = ''.join(c if not c.isspace() else '_' for c in filename)
        filename = ''.join(c for c in filename if ord(c) < 127)
        return filename

    def generate_header(self):
        with open(self.file_path, mode='rb') as f:
            print('#pragma once')
            print()
            print('/*\n * generated with bintohead.py from file\n * %s\n */' % self.file_path)
            print()
            print('static const char %s[] = {' % self.object_name)

            first = True
            col = 0

            c = f.read(1)

            while c:
                # printing commas and newlines
                if first:
                    first = False
                    print('\t', end='')
                    col = 8
                else:
                    if col > 70:
                        print(',\n\t', end='')
                        col = 8
                    else:
                        print(', ', end='')
                        col += 2

                # printing the byte
                print_byte = str(ord(c))
                print(print_byte, end='')
                col += len(print_byte)

                # read next byte
                c = f.read(1)

            print('\n};')


def main():
    if len(argv) == 1 or len(argv) > 3:
        print('usage: %s [FILE_PATH] [OBJECT NAME]' % argv[0], file=stderr)
    else:
        file_path = argv[1]
        object_name = argv[2] if len(argv) == 3 else None
        Binary(file_path, object_name).generate_header()


if __name__ == '__main__':
    try:
        main()
    except (BrokenPipeError, KeyboardInterrupt):
        pass
    except Exception as e:
        print('%s: %s' % (argv[0], str(e)), file=stderr)
        exit(1)
