#!/usr/bin/env python3
#
# Cargo runner for Phoenix-RTOS QEMU targets: `cargo-runner.py <binary> [args...]`
# runs the binary on the target with trunner and exits with its exit status.
# The binary goes into a scratch copy of the project image, or the syspage.
#
# Environment: TARGET (required), RUST_RUNNER_DIR (scratch directory),
# RUST_RUNNER_BOOT_TIMEOUT, RUST_RUNNER_RUN_TIMEOUT (default 300s),
# RUST_RUNNER_STRIP (the build's STRIP), RUST_RUNNER_DOCKER (0/1, overrides
# IMAGES' "docker"), RUST_RUNNER_DOCKER_IMAGE, RUST_RUNNER_ENV ("NAME=value
# ..." exported in the target's shell before the binary runs, e.g.
# RUST_MIN_STACK)
#
# Copyright 2026 Phoenix Systems
#
# SPDX-License-Identifier: BSD-3-Clause
#

import os
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

import pexpect

from runner import resolve_targets_and_hosts
from trunner.ctx import TestContext
from trunner.harness import HarnessError
from trunner.text import remove_ansi_sequences
from trunner.types import AppOptions, BootloaderOptions, ShellOptions, Status, TestOptions, TestResult

PROJECT = Path(__file__).resolve().parent.parent

# fs: where the binary goes - the rootfs ("ext2", "jffs2"), trunner's syspage
# loader ("loader") or a rebuilt boot image ("syspage")
# docker: the QEMU machine is only in the devel image (Xilinx fork)
IMAGES = {
    "riscv64-generic-qemu": {"fs": "ext2", "device": "vblk0", "page_size": 0x1000},
    "ia32-generic-qemu": {"fs": "ext2", "device": "hd0", "page_size": 0x1000},
    "aarch64a53-zynqmp-qemu": {
        "fs": "jffs2", "device": "flash0", "erase_size": 0x10000, "page_size": 0x1000, "docker": True,
    },
    "armv7a9-zynq7000-qemu": {
        "fs": "jffs2", "device": "flash0", "erase_size": 0x10000, "page_size": 0x1000, "docker": True,
    },
    "armv7r5f-zynqmp-qemu": {"fs": "loader", "docker": True},
    "sparcv8leon-generic-qemu": {
        # as exported by the target's build.project
        "fs": "syspage",
        "device": "flash0",
        "image": "phoenix.disk",
        "page_size": 0x1000,
        "boot_device": "ram0",
        "magic_user_script": "dabaabad",
        "syspage_maps": ("sram", "sram"),
        "disk_parts": ("plo=@plo-sparcv8leon-generic.img",),
    },
}

DOCKER_IMAGE = os.environ.get("RUST_RUNNER_DOCKER_IMAGE", "phoenixrtos/devel")
# as in docker-devel.sh
DOCKER_PROJECT = "/project"


def die(msg):
    print(f"cargo-runner: {msg}", file=sys.stderr)
    sys.exit(1)


def target_file(target, name):
    """The project's file, else the target's (as b_set_default_path())."""
    project = PROJECT / "_projects" / target / name
    if project.is_file():
        return project
    family, _, rest = target.partition("-")
    subfamily = rest.partition("-")[0]
    return PROJECT / "_targets" / family / subfamily / name


def image_builder(spec, target, prefix_boot, prefix_prog, args):
    """Run image_builder.py with the paths and variables the build would set."""
    builder = PROJECT / "phoenix-rtos-build" / "scripts" / "image_builder.py"
    build = PROJECT / "_build" / target
    env = dict(os.environ)
    env["SIZE_PAGE"] = str(spec["page_size"])
    if "boot_device" in spec:
        env["BOOT_DEVICE"] = spec["boot_device"]
    if "magic_user_script" in spec:
        env["MAGIC_USER_SCRIPT"] = spec["magic_user_script"]
    result = subprocess.run(
        [sys.executable, str(builder),
         "--size-page", str(spec["page_size"]),
         "--prefix-boot", str(prefix_boot),
         "--prefix-rootfs", str(PROJECT / "_fs" / target / "root"),
         "--prefix-prog-stripped", str(prefix_prog),
         "--plo-script-dir", str(build / "plo-scripts")] + args,
        capture_output=True, text=True, env=env)
    if result.returncode != 0:
        die(f"image_builder.py {args[0]} failed for {target}: "
            f"{result.stderr.strip() or result.stdout.strip()}")
    return result.stdout


def nvm_config(target):
    nvm = target_file(target, "nvm.yaml")
    if not nvm.is_file():
        die(f"no nvm.yaml for {target} (looked for {nvm})")
    return nvm


def rootfs_geometry(spec, target):
    """(byte offset, byte size) of the rootfs partition in the device's disk image."""
    device = spec["device"]
    offs, size = image_builder(
        spec, target, PROJECT / "_boot" / target, PROJECT / "_build" / target / "prog.stripped",
        ["query", "--nvm", str(nvm_config(target)),
         "{{ nvm.%s.rootfs.offs }} {{ nvm.%s.rootfs.size }}" % (device, device)]).split()
    return int(offs), int(size)


def link(src, dst):
    """Relative, so that it also resolves in the devel container."""
    dst.symlink_to(os.path.relpath(src.resolve(), dst.parent))


def scratch_boot_dir(target, work, keep):
    """A boot directory linking to the project's, except for `keep`, which is copied."""
    boot = PROJECT / "_boot" / target
    if not boot.is_dir():
        die(f"no {boot}; build the project first")

    scratch = work / "boot"
    if scratch.exists():
        shutil.rmtree(scratch)
    scratch.mkdir(parents=True)

    for entry in boot.iterdir():
        if entry.name == keep:
            shutil.copyfile(entry, scratch / entry.name)
        else:
            link(entry, scratch / entry.name)

    if not (scratch / keep).is_file():
        die(f"no {keep} in {boot}; build the project first")
    return scratch


def splice(src, disk, offset):
    with open(src, "rb") as fsrc, open(disk, "r+b") as fdisk:
        fdisk.seek(offset)
        shutil.copyfileobj(fsrc, fdisk)


def inject_ext2(spec, target, work, binary, path):
    """Add the binary to the ext2 rootfs of a scratch disk image."""
    if not shutil.which("debugfs"):
        die("debugfs (e2fsprogs) not found")

    offset, _ = rootfs_geometry(spec, target)
    image = f"{spec['device']}.disk"
    boot = scratch_boot_dir(target, work, image)

    rootfs = work / "rootfs.img"
    with open(boot / image, "rb") as fsrc, open(rootfs, "wb") as fdst:
        fsrc.seek(offset)
        shutil.copyfileobj(fsrc, fdst)

    # rm fails if there is nothing to replace; debugfs carries on
    requests = work / "debugfs.cmd"
    requests.write_text(
        f"cd {path.parent}\n"
        f"rm {path.name}\n"
        f"write {binary} {path.name}\n"
        f"sif {path.name} mode 0100755\n")
    result = subprocess.run(["debugfs", "-w", "-f", str(requests), str(rootfs)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        die(f"debugfs failed: {result.stderr.strip()}")

    splice(rootfs, boot / image, offset)
    return boot


def inject_jffs2(spec, target, work, binary, path):
    """Regenerate the jffs2 rootfs with the binary, as b_image_target() in build.project does."""
    if not shutil.which("mkfs.jffs2"):
        die("mkfs.jffs2 (mtd-utils) not found")
    fs_root = PROJECT / "_fs" / target / "root"
    if not fs_root.is_dir():
        die(f"no rootfs staging tree at {fs_root}; build the project first")

    offset, part_size = rootfs_geometry(spec, target)

    staging = work / "root"
    if staging.exists():
        shutil.rmtree(staging)
    shutil.copytree(fs_root, staging, copy_function=os.link, symlinks=True)

    staged = staging / path.relative_to("/")
    staged.parent.mkdir(parents=True, exist_ok=True)
    if staged.exists():
        staged.unlink()
    shutil.copyfile(binary, staged)
    staged.chmod(0o755)

    rootfs = work / "rootfs.jffs2"
    result = subprocess.run(
        ["mkfs.jffs2", "-U", "-c", "12", "-m", "none",
         "-e", str(spec["erase_size"]), "-s", str(spec["page_size"]),
         "-n", "-r", str(staging), "-o", str(rootfs), "-p"],
        stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        die(f"mkfs.jffs2 failed: {result.stderr.strip()}")
    pad_cleanmarkers(rootfs, spec["erase_size"], part_size)

    boot = scratch_boot_dir(target, work, f"{spec['device']}.disk")
    splice(rootfs, boot / f"{spec['device']}.disk", offset)
    return boot


def pad_cleanmarkers(rootfs, erase_size, part_size):
    """Fill the rest of the partition with erase blocks starting with a cleanmarker."""
    size = rootfs.stat().st_size
    if size % erase_size:
        die(f"jffs2 image is {size} bytes, not a multiple of the {erase_size}-byte erase block")
    if size > part_size:
        die(f"rootfs is {size} bytes but the partition is only {part_size}")

    # magic 0x1985, node type 0x2003, total length 0x10, then its crc
    cleanmarker = bytes([0x85, 0x19, 0x03, 0x20,
                         0x10, 0x00, 0x00, 0x00,
                         0x96, 0x58, 0xD1, 0xFE])
    block = bytearray(b"\xff" * erase_size)
    block[: len(cleanmarker)] = cleanmarker
    with open(rootfs, "ab") as fh:
        for _ in range((part_size - size) // erase_size):
            fh.write(block)


def strip(binary):
    """Strip as the build does - unstripped binaries do not load into the syspage."""
    command = shlex.split(os.environ.get("RUST_RUNNER_STRIP", ""))
    if not command:
        die("RUST_RUNNER_STRIP is not set; a syspage binary has to be stripped as the build does it")
    # strip.py only takes `-o out in`
    unstripped = binary.with_name(binary.name + ".unstripped")
    binary.replace(unstripped)
    result = subprocess.run(command + ["-o", str(binary), str(unstripped)], stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        die(f"{shlex.join(command)} failed: {result.stderr.strip()}")


def inject_syspage(spec, target, work, binary, name):
    """Rebuild the boot image with the binary as a syspage app (not started, no -x)."""
    script = target_file(target, "user.plo.yaml")
    if not script.is_file():
        die(f"no user.plo.yaml for {target} (looked for {script})")

    # plo looks up apps by name in prog.stripped
    real_prog = PROJECT / "_build" / target / "prog.stripped"
    if not real_prog.is_dir():
        die(f"no {real_prog}; build the project first")
    prog = work / "prog.stripped"
    if prog.exists():
        shutil.rmtree(prog)
    prog.mkdir(parents=True)
    for entry in real_prog.iterdir():
        link(entry, prog / entry.name)
    staged = prog / name
    shutil.copyfile(binary, staged)
    staged.chmod(0o755)
    strip(staged)

    codemap, datamap = spec["syspage_maps"]
    app_line = f"  - app {{{{ env.BOOT_DEVICE }}}} {name} {codemap} {datamap}\n"
    lines = script.read_text().splitlines(keepends=True)
    for i, line in enumerate(lines):
        if line.lstrip().startswith(("- wait", "- go!")):
            lines.insert(i, app_line)
            break
    else:
        die(f"could not find where to add an app in {script}")
    user_script = work / "user.plo.yaml"
    user_script.write_text("".join(lines))

    image = spec["image"]
    boot = scratch_boot_dir(target, work, image)
    (boot / "part_kernel.img").unlink(missing_ok=True)

    nvm = str(nvm_config(target))
    image_builder(spec, target, boot, prog,
                  ["partition", "--nvm", nvm, "--name", "kernel", "--script", str(user_script)])

    disk_args = ["disk", "--nvm", nvm, "--name", spec["device"]]
    for part in spec.get("disk_parts", ()):
        # plo=@<name>: a file in prog.stripped
        if part.startswith("plo=@"):
            part = "plo=" + str(prog / part[len("plo=@"):])
        disk_args += ["--part", part]
    (boot / image).unlink()
    image_builder(spec, target, boot, prog, disk_args + ["--out", image])
    return boot


# a kernel exception report, or the target booting again
CRASH = re.compile(r"Exception: \d+ #\w+|Phoenix-RTOS loader v\.|Phoenix-RTOS microkernel v")
# a failed kernel assertion (lib_assertPanic()); the target halts, so the prompt never returns
KERNEL_PANIC = r"kernel \(\w+:\d+\): [^\r\n]*"


class Relay:
    """Passes the program's output on to stdout, holding back what may be the returning prompt."""

    HOLD = 64

    def __init__(self, prompt):
        self.prompt = prompt
        self.pending = ""
        self.seen = ""

    def _emit(self, text):
        sys.stdout.write(remove_ansi_sequences(text).replace("\r", ""))
        sys.stdout.flush()

    def write(self, text):
        self.pending += text
        self.seen += text
        if len(self.pending) > self.HOLD:
            self._emit(self.pending[: -self.HOLD])
            self.pending = self.pending[-self.HOLD:]

    def flush(self):
        pass

    def finish(self):
        end = self.pending.rfind(self.prompt)
        self._emit(self.pending[: end if end >= 0 else len(self.pending)])
        self.pending = ""


def program_harness(dut, prompt, timeout, exit_status, exports=(), command=None):
    """Relays the program's output until the prompt returns, then asks the shell for its exit status.

    With `command`, exports the `exports` ("NAME=value") and runs it first, the shell not having done so.
    """

    def harness(result: TestResult) -> TestResult:
        if command is not None:
            for export in exports:
                dut.sendline(f"export {export}")
                dut.expect_exact(prompt, timeout=10)
            dut.sendline(command)
            dut.expect(f"{re.escape(command)}(\r+)\n", timeout=10)

        relay = Relay(prompt)
        # read past the echoed command already
        relay.write(dut.buffer)
        # Dut would keep the attribute itself
        dut.pexpect_proc.logfile_read = relay
        try:
            panicked = dut.expect([re.escape(prompt), KERNEL_PANIC], timeout=timeout) == 1
        except pexpect.TIMEOUT:
            return TestResult(msg=f"the program did not finish within {timeout:g}s", status=Status.FAIL)
        finally:
            dut.pexpect_proc.logfile_read = None
            relay.finish()

        if panicked:
            return TestResult(msg=f"the program crashed the target: {dut.match.group(0).strip()}", status=Status.FAIL)

        # a crash can reset the target, whose fresh shell would then report 0
        crash = CRASH.search(remove_ansi_sequences(relay.seen))
        if crash:
            return TestResult(msg=f"the program crashed the target: {crash.group(0).strip()}", status=Status.FAIL)

        dut.sendline("echo $?")
        try:
            dut.expect(r"echo \$\?\r*\n(\d+)\r*\n", timeout=10)
        except pexpect.TIMEOUT:
            return TestResult(msg="the shell did not report the exit status", status=Status.FAIL)
        exit_status.append(int(dut.match.group(1)))
        return TestResult(status=Status.OK)

    return harness


def use_docker(spec):
    forced = os.environ.get("RUST_RUNNER_DOCKER")
    if forced is not None:
        return forced not in ("", "0", "no", "false")
    return bool(spec.get("docker"))


def run_in_docker(argv, work, boot):
    """Boot and run in the devel container; the image is prepared on the host."""
    if not shutil.which("docker"):
        die("this target's QEMU lives in the devel container, but docker was not found; see docker-devel.sh")
    for path in [Path(argv[1]), work]:
        if not path.is_relative_to(PROJECT):
            die(f"{path} is outside of {PROJECT}, which is all the devel container sees")

    def translate(text):
        return text.replace(str(PROJECT), DOCKER_PROJECT)

    # no root-owned bytecode caches in the source tree
    env = ["-e", "PYTHONDONTWRITEBYTECODE=1",
           "-e", "RUST_RUNNER_PREPARED=1", "-e", f"RUST_RUNNER_DIR={translate(str(work))}"]
    if boot is not None:
        env += ["-e", f"PHOENIX_BOOT_DIR={translate(str(boot))}"]
    for var in ("TARGET", "RUST_RUNNER_BOOT_TIMEOUT", "RUST_RUNNER_RUN_TIMEOUT", "RUST_RUNNER_ENV"):
        if var in os.environ:
            env += ["-e", f"{var}={translate(os.environ[var])}"]

    command = [
        "docker", "run", "--rm", "-i",
        "-v", f"{PROJECT}:{DOCKER_PROJECT}",
        "-w", DOCKER_PROJECT,
        *env,
        "--entrypoint", "python3",
        DOCKER_IMAGE,
    ] + [translate(arg) for arg in [str(Path(__file__).resolve())] + argv[1:]]
    return subprocess.run(command).returncode


def main(argv):
    if len(argv) < 2:
        die("usage: cargo-runner.py <binary> [args...]")

    binary = Path(argv[1]).resolve()
    args = argv[2:]
    if not binary.is_file():
        die(f"no such binary: {binary}")

    target_name = os.environ.get("TARGET")
    if not target_name:
        die("TARGET is not set")
    spec = IMAGES.get(target_name)
    if spec is None:
        die(f"TARGET={target_name} is not supported; known targets: " + ", ".join(sorted(IMAGES)))

    targets, _ = resolve_targets_and_hosts()
    ctx = TestContext(
        port=None,
        baudrate=115200,
        project_path=PROJECT,
        nightly=False,
        logdir=None,
        should_flash=False,
        should_test=True,
        verbosity=0,
        stream_output=False,
        output=None,
    )
    target = targets[target_name].from_context(ctx)
    if "RUST_RUNNER_BOOT_TIMEOUT" in os.environ:
        target.prompt_timeout = float(os.environ["RUST_RUNNER_BOOT_TIMEOUT"])
    run_timeout = float(os.environ.get("RUST_RUNNER_RUN_TIMEOUT", 300))

    work = Path(os.environ.get("RUST_RUNNER_DIR") or PROJECT / "_build" / target_name / "cargo-runner")
    work.mkdir(parents=True, exist_ok=True)

    # <crate>-<hash>, unique per build; plo keeps 33 characters of an app's name (phfs_file_t.alias)
    name = f"rust-{binary.name}"[:33]
    path = target.exec_dir() / name
    # trunner's syspage loader takes the binary from <root_dir>/<shell.path>
    root = work / "root"

    # set by run_in_docker()
    if os.environ.get("RUST_RUNNER_PREPARED") != "1":
        if spec["fs"] in ("ext2", "jffs2"):
            inject = inject_ext2 if spec["fs"] == "ext2" else inject_jffs2
            boot = inject(spec, target_name, work, binary, path)
        elif spec["fs"] == "loader":
            if root.exists():
                shutil.rmtree(root)
            staged = root / path.relative_to("/")
            staged.parent.mkdir(parents=True)
            shutil.copyfile(binary, staged)
            strip(staged)
            boot = None
        else:
            boot = inject_syspage(spec, target_name, work, binary, name)

        if use_docker(spec):
            return run_in_docker([argv[0], str(binary)] + args, work, boot)
        if boot is not None:
            os.environ["PHOENIX_BOOT_DIR"] = str(boot)

    test = TestOptions(name=name, should_reboot=True)
    if spec["fs"] in ("ext2", "jffs2"):
        test.shell = ShellOptions(binary=name, cmd=[str(path)] + args)
    elif spec["fs"] == "loader":
        target.root_dir = lambda: root
        test.shell = ShellOptions(binary=name, cmd=["sysexec", name] + args, path=path.parent.relative_to("/"))
        test.bootloader = BootloaderOptions(apps=[AppOptions(file=name, exec=False)])
    else:
        test.shell = ShellOptions(binary=name, cmd=["sysexec", name] + args)

    exit_status = []
    exports = shlex.split(os.environ.get("RUST_RUNNER_ENV", ""))
    command = None
    if exports:
        # the shell harness would run the binary before the exports; the program harness does both
        command = " ".join(map(shlex.quote, test.shell.cmd))
        test.shell.cmd = None
    test.harness = program_harness(target.dut, target.shell_prompt.strip(), run_timeout, exit_status, exports, command)

    try:
        result = target.build_test(test)(TestResult(name))
    except HarnessError as e:
        print(str(e), file=sys.stderr)
        return 1
    finally:
        target.dut.close()

    if result.status is not Status.OK:
        die(f"{name}: {result.msg}")
    return exit_status[0]


if __name__ == "__main__":
    sys.exit(main(sys.argv))
