"""
(psh)% ps
  PID  PPID PR STATE  %CPU  WAIT    TIME   VMEM THR CMD
    0     0  7 ready  97.1  2.9s   31:25   2.6M   1 [idle]
    5     1  1 sleep   1.7 295ms    0:37   124K   3 uart16550
    4     1  4 ready   0.6   7ms    0:12   140K   1 psh
    2     1  4 sleep   0.1  52ms    0:03   308K   1 dummyfs
    1     0  4 sleep   0.0   3ms    0:00      0   1 init
"""

def harness(p):
    header_seen = False
    expected_tasks = ['[idle]', 'init', 'psh']
    ps_header = 'PID PPID PR STATE %CPU WAIT TIME VMEM THR CMD'.split()
    prompt = '\r\x1b[0J' + '(psh)% '

    got = p.read(len(prompt))
    if got != prompt:
        print(f'Expected:\n\t{prompt}\nGot:\n\t{got}')
        return False

    p.sendline('ps')
    p.expect('ps(\r+)\n')

    while True:
        # Get prompt or new line
        idx = p.expect(['\(psh\)\% ', '(.*)(\r+)\n'])
        if idx == 0:
            break

        line = p.match.group(0)

        if line.split() == ps_header and not header_seen:
            header_seen = True
        else:
            try:
                pid, ppid, pr, state, cpu, wait, time, vmem, thr, task = line.split()
            except ValueError:
                print(f'psh ps: wrong ps output')
                return False

            if task in expected_tasks:
                expected_tasks.remove(task)

    if not header_seen:
        print(f'psh ps: wrong header')

    if expected_tasks:
        print(f'psh ps: not seen expected task: {", ".join(expected_tasks)}')
        return False

    return True
