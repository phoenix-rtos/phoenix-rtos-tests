import psh.tools.psh as psh
import time

@psh.run
def harness(p):
    print('start')
    time.sleep(30)

    while True:
        p.sendline('date')
        p.expect_exact('(psh)% ')
        if '01:00' in p.before:
            p.sendline('touch /usr/bin/hello')
            p.sendline('ls -la /usr/bin/hello')

            time.sleep(1)
            p.sendline('date')
            p.expect_exact('(psh)% ')
            p.sendline('touch /usr/bin/hello')
            p.sendline('ls -la /usr/bin/hello')
            break
