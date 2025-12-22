import psh.tools.psh as psh
import pytest

TEST_DIR = "test_dir"

echo_resp_code_list = [
    ("", "", 0),
    ("hello", "hello", 0),
    ("hello world", "hello world", 0),
    ("lorem $ipsum dolor$ales", "lorem  dolor", 0),
    ("hello > /", ".*EISDIR", 0),
    ("-illegal_arg", ".*", 1),
]


@pytest.mark.parametrize("msg, resp, code", echo_resp_code_list)
def test_echo(pexpect_psh, msg, resp, code):
    psh.assert_cmd(
        pexpect_psh,
        f"echo {msg}",
        expected=f"{resp}\r+\n",
        result="dont-check",
        msg=f'echo "{msg}" failed',
        is_regex=True,
    )
    psh.assert_cmd(
        pexpect_psh,
        "echo $?",
        expected=f"{code}",
        result="success",
        msg=f"last exit status was not the expected {code}",
    )


def is_dir_in_files(dirname: str, files: list[psh.File]) -> bool:
    return any(file.is_dir and file.name == dirname for file in files)


def test_mkdir_rmdir(pexpect_psh):
    psh.assert_cmd(pexpect_psh, f"mkdir {TEST_DIR}")

    files = psh.ls(pexpect_psh, "/")
    assert is_dir_in_files(TEST_DIR, files)
    psh.assert_cmd(pexpect_psh, f"rmdir {TEST_DIR}")
    files = psh.ls(pexpect_psh, "/")
    assert not is_dir_in_files(TEST_DIR, files)
