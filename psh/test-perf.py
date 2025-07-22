import psh.tools.psh as psh


@psh.run
def harness(p):
    psh.assert_prompt_after_cmd(p, 'perf -m trace -e /dev/null -o /dev/null -t 2', result='success')
