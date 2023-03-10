#
# Phoenix-RTOS test runner
#
# The harness for the LSB_VSX-2.0-1 (POSIX) Test Suite
#
# Copyright 2022 Phoenix Systems
# Authors: Adam Dębek
#

from trunner.dut import Dut
from trunner.types import TestResult, Result, Status


def harness(dut: Dut):
    test_results = []
    result = None
    msg = []
    test_name = []
    test_status = Status.OK

    NAME = r'(.+?)Execute(.+?)T\.(?P<name>.+?)\r+\n'
    PROMPT = r'(\r+)\x1b\[0J' + r'\(psh\)% '
    #RESULT = r"(.+?)(?P<status>PASS|UNRESOLVED|FAIL)\r+\n"  no status unresolved for now
    RESULT = r"(.+?)(?P<status>PASS|FAIL)\r+\n"
    FINAL = r"(.+?)TCC End\r+\n"
    MESSAGE = r"(?P<line>.*?)\r+\n"

    dut.expect("Config End\r+\n")

    while True:
        idx = dut.expect([
            NAME,
            RESULT,
            FINAL,
            MESSAGE
        ], timeout=300)
        parsed = dut.match.groupdict()
        
        if idx == 0:
            test_name = parsed["name"]
        elif idx == 1:
            result = Result(name=test_name, status=Status.from_str(parsed["status"]))
        elif idx == 2:
            break
        elif idx == 3:
            line = parsed["line"]
            #Get rid of unnecessary messages
            if(line.find("IC") != -1 or line.find("TP") != -1 or line.find("TC") != -1 or line.find("Execute") != -1):
                continue
            else:
                line = line[line.find("|") + 1:]
                line = line[line.find("|") + 1:]
                msg.append('\t\t' + line + '\n')
            #Tests with these problems treat as fails
            if (line.find("can't acquire exclusive lock") != -1) or (line.find("can't exec") != -1):
                test_status = Status.FAIL
                result = Result(name=test_name, status=Status.FAIL)
                result.status = Status.FAIL

                if (line.find("can't acquire exclusive lock") != -1): 
                    result.msg = '\t\t' + "can't acquire exclusive lock\n"
                elif(line.find("can't exec") != -1):
                    result.msg = '\t\t' + "compilation error\n"

                test_results.append(result)
                result = None

        if idx != 3 and result:
            # We ended processing test result and message
            if msg and result.status == Status.FAIL:
                test_status = Status.FAIL
                result.msg = "\n".join(msg)
                msg = []
            #UNRESOLVED status not added yet
            #elif msg and result.status == Status.UNRESOLVED:
            #    result.status = Status.UNRESOLVED
            
            test_results.append(result)
            result = None


    return TestResult(msg=Result.format_output(test_results), status=test_status)
