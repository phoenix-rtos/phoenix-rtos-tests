import itertools
import re
from colorama import Style, Fore


ANSI_ESCAPE = re.compile(r'(?:\x1B[@-_]|[\x80-\x9F])[0-?]*[ -/]*[@-~]')


def bold(s: str) -> str:
    return Style.BRIGHT + s + Style.RESET_ALL


def red(s: str) -> str:
    return Fore.RED + s + Fore.RESET


def green(s: str) -> str:
    return Fore.GREEN + s + Fore.RESET


def yellow(s: str) -> str:
    return Fore.YELLOW + s + Fore.RESET


def blue(s: str) -> str:
    return Fore.BLUE + s + Fore.RESET


def magenta(s: str) -> str:
    return Fore.MAGENTA + s + Fore.RESET


def remove_ansi_sequences(line: str) -> str:
    return ANSI_ESCAPE.sub('', line)


def escape_invalid_xml_characters(s: str) -> str:
    """Escapes characters that are invalid in XML."""
    if not hasattr(escape_invalid_xml_characters, "translation_map"):
        translation_map = {
            ord("\0"): "\\0",
            ord("\a"): "\\a",
            ord("\b"): "\\b",
            ord("\v"): "\\v",
            ord("\f"): "\\f",
            ord("\\"): "\\\\",
        }

        for i in itertools.chain(
            range(0x08 + 1), range(0x0B, 0x0C + 1), range(0x0E, 0x1F + 1),
            range(0x7F, 0x84 + 1), range(0x86, 0x9F + 1)
        ):
            if i not in translation_map:
                translation_map[i] = f"\\x{i:02x}"

        escape_invalid_xml_characters.translation_map = translation_map

    return s.translate(escape_invalid_xml_characters.translation_map)
