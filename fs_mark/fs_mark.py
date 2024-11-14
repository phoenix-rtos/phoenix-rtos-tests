import trunner
import time
import re
import pexpect
import timing_data as t_data
import psh.tools.psh as psh

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Status


def clean(dut, test_name, ctx: TestContext) -> int:
    # Dummyfs doesn't need clean since target is rebooted after every test
    if not ctx.target.rootfs:
        return 0

    if test_name and ('-k' in test_name or '-L' in test_name or '-F' in test_name):
        args = test_name.split()
        args = args[1:]

        def get_dir(x):
            if args[x[0] - 1] == '-d':
                return x[1]

        dirs = list(map(lambda x: get_dir(x), enumerate(args)))
        dirs = [x for x in dirs if x is not None]

        dut.expect(re.escape(ctx.target.shell_prompt), timeout=120)
        psh._send(dut, f'/bin/fs_mark_clean {" ".join(dirs)}')

        idx = dut.expect([re.escape(ctx.target.shell_prompt),
                          r".+?Error in remove_dir_recursive.+?",
                          pexpect.TIMEOUT], timeout=1800)

        if idx == 0:
            return 0
        elif idx == 1:
            return -1
        elif idx == 2:
            return -2

    else:
        return 0


def harness(dut: Dut, ctx: TestContext, result: TestResult):
    target = trunner.ctx.target.name
    test_status = Status.OK

    test_name = None
    test_msg = ''

    loop_start = None
    loop_end = None
    loop_time = 1200
    first_loop_done = False

    NAME = r".+?#  (?P<name>(/bin/)?fs_mark.+?)\r+\n"
    MSG_LINE = r"(?P<line>(\s+\d+){3}.+?)\r+\n"
    NO_SPC = r"Insufficient free space.+?\r+\n"
    END = r"Average Files/sec:.+?\r+\n"
    ERR = r".+?(?P<err>EIO|ENOSPC|ENOMEM).+?"
    NO_CONT_BLOCK = r"(?P<msg>Lack of contiguous memory block of size \d+ bytes.+?)\r+\n"

    while True:
        if loop_start and loop_end:
            loop_time = 3 * (loop_end - loop_start)
            loop_start = None
            loop_end = None

        try:
            idx = dut.expect([
                NAME,
                MSG_LINE,
                NO_SPC,
                END,
                NO_CONT_BLOCK,
                ERR
            ], timeout=loop_time)
            parsed = dut.match.groupdict()

        except pexpect.TIMEOUT:
            test_msg = 'Got timeout, probably fs hang-up'
            test_status = Status.FAIL
            break

        if idx == 0:
            test_name = parsed["name"]
            loop_start = time.time()
        elif idx == 1:
            first_loop_done = True
            splitted_line = parsed["line"].split()
            f_use = splitted_line[0]
            count = splitted_line[1]
            size = splitted_line[2]
            files_sec = splitted_line[3]
            app_overhead = splitted_line[4]

            line_dict = {}
            line_dict['creatMin'] = splitted_line[5]
            line_dict['creatAvg'] = splitted_line[6]
            line_dict['creatMax'] = splitted_line[7]
            line_dict['writeMin'] = splitted_line[8]
            line_dict['writeAvg'] = splitted_line[9]
            line_dict['writeMax'] = splitted_line[10]
            line_dict['closeMin'] = splitted_line[17]
            line_dict['closeAvg'] = splitted_line[18]
            line_dict['closeMax'] = splitted_line[19]

            # If files are kept, fs_mark will unlink them so unlink time can be caught
            if not ('-k' in test_name or '-L' in test_name or '-F' in test_name):
                line_dict['unlinkMin'] = splitted_line[20]
                line_dict['unlinkAvg'] = splitted_line[21]
                line_dict['unlinkMax'] = splitted_line[22]

            for name, value in line_dict.items():
                if not t_data.timing_dict[(target, name)][0] <= int(value) <= t_data.timing_dict[(target, name)][1]:
                    test_status = Status.FAIL
                    test_msg += ''.join(('\n\t', name, ' exec time - ', value, ' out of range [',
                                         str(t_data.timing_dict[(
                                             target, name)][0]), ' - ',
                                         str(t_data.timing_dict[(target, name)][1]), ']'))

            if test_status == Status.FAIL:
                test_msg += "\n\n\tF_Use%: " + str(f_use)
                test_msg += "\n\tCount: " + str(count)
                test_msg += "\n\tSize: " + str(size)
                test_msg += "\n\tFiles/sec: " + str(files_sec)
                test_msg += "\n\tApp overhead: " + str(app_overhead) + "\n\t"
                break

        elif idx in [2, 3, 4]:
            # Tests have to run at least 1 loop
            if not first_loop_done:
                test_status = Status.FAIL
                if idx == 2:
                    test_msg = 'Insufficient free space'
                elif idx == 3:
                    loop_end = time.time()
                    test_msg = 'Got no timings'
                elif idx == 4:
                    test_msg = parsed['msg']

            break
        elif idx == 5:
            test_msg = parsed['err']
            test_status = Status.FAIL
            break

    ret = clean(dut, test_name, ctx)
    if ret < 0 and first_loop_done:
        test_status = Status.FAIL
        if ret == -1:
            test_msg = 'Error while cleaning'
        elif ret == -2:
            test_msg = 'Timeout during cleaning'

    return TestResult(msg=test_msg, status=test_status)
