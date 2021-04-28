class Color:
    """This class is used to color a string"""

    FAIL = '\033[91m'
    OK = '\033[92m'
    SKIP = '\033[93m'
    BOLD = '\033[1m'
    END = '\033[0m'

    @staticmethod
    def colorify(string, color):
        return f"{color}{string}{Color.END}"
