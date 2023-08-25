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
