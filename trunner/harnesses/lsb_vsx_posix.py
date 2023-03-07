#
# Phoenix-RTOS test runner
#
# The harness for the LSB_VSX-2.0-1 (POSIX) Test Suite
#
# Copyright 2022 Phoenix Systems
# Authors: Adam Dębek
#

from .common import TestResult


class LsbVsxPosixTestHarness:
    """Class providing harness for parsing output of the LSB_VSX_POSIX test suite"""

    PROMPT = r'(\r+)\x1b\[0J' + r'\(psh\)% '
    RESULT = r"(.+?)(PASS|UNRESOLVED|FAIL)\r+\n"
    FINAL = r"(.+?)TCC End\r+\n"
    MESSAGE = r"(.*?)\r+\n"

    @staticmethod
    def harness(proc):
        test_results = []
        test = None
        msg = ""
        i = 1

        proc.expect("Config End\r+\n")

        while True:
            idx = proc.expect([
                LsbVsxPosixTestHarness.RESULT,
                LsbVsxPosixTestHarness.FINAL,
                LsbVsxPosixTestHarness.MESSAGE
            ], timeout=300)
            groups = proc.match.groups()
            
            if idx == 0:
                test = dict(zip(('status', 'name'), (groups[1], 'test_case' + str(i))))
                i += 1
            elif idx == 1:
                break
            elif idx == 2:
                line = groups[0]
                #Get rid of unnecessary messages
                if(line.find("IC") != -1 or line.find("TP") != -1 or line.find("TC") != -1 or line.find("Execute") != -1):
                    continue
                else:
                    line = line[line.find("|") + 1:]
                    line = line[line.find("|") + 1:]
                    msg += '\t\t' + line + '\n'
                #Tests with these problems treat as fails
                if (line.find("can't acquire exclusive lock") != -1) or (line.find("can't exec") != -1):
                    test = dict(zip(('status', 'name'), (TestResult.FAIL, ' ')))
                    test['status'] = TestResult.FAIL
                    test['msg'] = '\t\t' + "compilation error\n"
                    test_results.append(TestResult(**test))

            if idx != 2 and test:
                # We ended processing test result and message
                if msg and test['status'] == TestResult.FAIL:
                    test['msg'] = msg
                    msg = ''
                elif msg and test['status'] == "UNRESOLVED":
                    test['status'] = TestResult.FAIL
                    test['msg'] = msg
                    msg = ''

                test_results.append(TestResult(**test))


        return test_results
