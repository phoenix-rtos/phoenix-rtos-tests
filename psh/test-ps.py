import psh.tools.psh as psh

"""
(psh)% ps
  PID  PPID PR STATE  %CPU  WAIT    TIME   VMEM THR CMD
    0     0  7 ready  97.1  2.9s   31:25   2.6M   1 [idle]
    5     1  1 sleep   1.7 295ms    0:37   124K   3 uart16550
    4     1  4 ready   0.6   7ms    0:12   140K   1 psh
    2     1  4 sleep   0.1  52ms    0:03   308K   1 dummyfs
    1     0  4 sleep   0.0   3ms    0:00      0   1 init
"""


@psh.run
def harness(p):
    header_seen = False
    expected_tasks = ['[idle]', 'init', 'psh']
    ps_header = 'PID PPID PR STATE %CPU WAIT TIME VMEM THR CMD'.split()

    p.sendline('ps')
    p.expect(r'ps(\r+)\n')

    while True:
        # Get prompt or new line
        idx = p.expect([r'\(psh\)\% ', r'(.*?)(\r+)\n'])
        if idx == 0:
            break

        line = p.match.group(0)

        if line.split() == ps_header and not header_seen:
            header_seen = True
        else:
            try:
                pid, ppid, pr, state, cpu, wait, time, vmem, thr, task = line.split()
            except ValueError:
                try:
                    pid, ppid, pr, state, cpu, wait, time, vmem, thr, task, arguments = line.split()
                except ValueError:
                    assert False, f'wrong ps output: {line}'
            # handle for example /bin/psh
            if task.endswith('psh'):
                task = 'psh'
            if task in expected_tasks:
                expected_tasks.remove(task)

    assert header_seen, 'ps header not seen'
    assert not expected_tasks, f'not seen expected task: {", ".join(expected_tasks)}'
