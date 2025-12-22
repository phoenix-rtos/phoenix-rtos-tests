import psh.tools.psh as psh
import pytest

TEST_DIR = "test_dir"

echo_resp_code_list = [
    ("", "", "success"),
    ("hello", "hello", "success"),
    ("hello world", "hello world", "success"),
    ("lorem $ipsum dolor$ales", "lorem  dolor", "success"),
    ("hello > /", ".*EISDIR", "success"),
    ("-illegal_arg", ".*", "fail"),
]


@pytest.mark.parametrize("msg, resp, exp_res", echo_resp_code_list)
def test_echo(pexpect_psh, msg, resp, exp_res):
    psh.assert_cmd(
        pexpect_psh,
        f"echo {msg}",
        expected=rf"{resp}\r+\n",
        result=exp_res,
        msg=f'echo "{msg}" failed',
        is_regex=True,
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
