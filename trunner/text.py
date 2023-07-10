from colorama import Style, Fore


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
