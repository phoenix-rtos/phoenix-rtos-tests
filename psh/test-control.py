# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh control codes test
#
# Copyright 2024 Phoenix Systems
# Author: Damian Modzelewski
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import tools.psh as psh
import trunner.ctx as ctx
import time


def CC_parser(harness, input: str, output: str):
    harness.write(input)
    if not output:
        harness.expect(psh.PROMPT)
    else:
        # For targets with a slow transfer rate we need to suspend next command util previous will terminate
        # Issue : https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1446
        if "armv7m4-stm32l4x6-nucleo" in ctx.target.name:
            time.sleep(0.5)

        harness.expect(f"psh: {output} not found", timeout=2)


"""
CONTROL CODE CHEAT-SHEET

(\177 or \033[3~) -> Delete the last character
\001 -> Move to the first position in the text
\003 -> Cancel command
(\004 or \033[3~) -> Delete the next character after the cursor
\005 -> Move to the last possible position
(\002 or \033[D) -> Move the cursor to the left
(\006 or \033[C) -> Move the cursor to the right
\033[A -> In system declaration of Up arrow or 8 on numeric keypad
\033[B -> In system declaration of Down arrow or 2 on numeric keypad
\013 -> Cut text between cursor and end of line
(\031 or \033[2~) -> Paste what is in the clipboard. The second one uses 0 on the numeric keypad.
\027 -> Cut the last phrase before the cursor
\025 -> Cut text between the beginning of the line and the cursor
\033[1;5D -> Combination of keys CTRL + 4 on numeric keypad or left arrow to move the cursor to the left over phrases
\033[1;5C -> Combination of keys CTRL + 6 on numeric keypad or right arrow to move the cursor to the left over phrases
\t -> autocomplete path

"""


@psh.run
def harness(p):
    # Use the up arrow to redo the previous command and back to the command passed at the beginning of the string
    CC_parser(p, "loremipsum\033[A\033[B\r", "loremipsum")
    # Move the cursor backward three paces and put ABC after it move one forward and put ABC
    CC_parser(p, "loremipsum\002\002\002\002ABC\006ABC\r", "loremiABCpABCsum")

    # Following control code terminates QEMU upon use and causes malfunctions
    # that's why skipped in the case of emulators.
    if "emu" not in ctx.target.name:
        # Cancel command
        CC_parser(p, "loremipsum\003", "")

    # Trim the first three characters and put ABC after it move to start and put ABC
    CC_parser(p, "loremipsum\177\177\177\177\001ABC\005ABC\r", "ABCloremiABC")
    # Use the up arrow to redo the previous command
    CC_parser(p, "\033[A\r", "ABCloremiABC")
    # Move the cursor to the start of the text and after that jump over the phrase,
    # remove whitespace and again jump. At the end copy all from the beginning to the cursor point
    CC_parser(p, "lorem ipsum dolor\001\033[1;5C\004\033[1;5C\025\r", "dolor")
    # Paste the copied text
    CC_parser(p, "\031\r", "loremipsum")
    # Cut the previous phrase before
    CC_parser(p, "lorem \027ipsum\r", "ipsum")
    # Paste the copied text
    CC_parser(p, "\031\r", "lorem")
    # Use the up arrow to redo the previous command
    CC_parser(p, "\033[A\r", "lorem")
    # Cut everything after the cursor
    CC_parser(p, "lorem\025ipsum\r", "ipsum")
    # Paste the copied text
    CC_parser(p, "\031\r", "lorem")
    # Move the cursor 5 times to the left and copy all after to the clipboard
    CC_parser(p, "loremipsum\033[D\033[D\033[D\033[D\033[D\013\r", "lorem")
    # Paste the copied text
    CC_parser(p, "\031\r", "ipsum")
    # Move the cursor 2 phrases before and cut everything before the cursor
    CC_parser(p, "lorem ipsum dolor\033[1;5D\033[1;5D\025\r", "ipsum")
    # The cursor moves three spaces to the left and removes 3 characters after it
    CC_parser(p, "loremipsum\033[D\033[D\033[D\033[D\033[D\004\004\004\r", "loremum")
    # Move the cursor 3 characters to the left and cut everything from the beginning to it point
    CC_parser(p, "loremips u m\033[D\033[D\033[D\033[D\033[D\025\r", "s")
    # Paste the copied text
    CC_parser(p, "\031\r\n", "loremip")

    # Create a directory to check auto-completion
    psh.assert_cmd(
        p,
        "mkdir contorlcodetestdir ",
        result="success",
        msg="Failed to create dir",
    )
    # Trying to execute the previous folder with auto-completion
    p.sendline("/con\t")
    p.expect("psh: /contorlcodetestdir/ is not an executable")
    # Removing unnecessary directory
    psh.assert_cmd(p, "rm -d contorlcodetestdir", result="success", msg="Failed to delete dir")
