import psh.tools.psh as psh


ROUTE_ROW0 = "%-15s %-15s %-15s %-5s %-6s %-6s %3s %s" % (
    "Destination",
    "Gateway",
    "Genmask",
    "Flags",
    "Metric",
    "Ref",
    "Use",
    "Iface",
)
DEFAULT_ROUTE_ARGS_LO0 = (
    "127.0.0.0",
    "127.0.0.1",
    "255.0.0.0",
    "UG",
    0,
    0,
    0,
    "lo0",
)


def format_route_row(
    destination_addr, gateway_addr, mask, sFlags, metric, refcnt, use, iface
):
    return "%-15s %-15s %-15s %-5s %-6d %-6d %3d %s" % (
        destination_addr,
        gateway_addr,
        mask,
        sFlags,
        metric,
        refcnt,
        use,
        iface,
    )


def match_literal_line(line):
    return rf"{line}[\r\n]+"


def match_anything_until(until):
    return rf"[\s\S]*?({until}){{1}}"


def route_assert_matches(pexpect_proc, expected_lines):
    pexpect_proc.sendline("route")
    pexpect_proc.expect(r"".join(expected_lines))
    pexpect_proc.expect_exact("(psh)%")


@psh.run
def harness(p):
    expected_lines = [
        match_literal_line("route"),
        match_literal_line(ROUTE_ROW0),
        match_anything_until(format_route_row(*DEFAULT_ROUTE_ARGS_LO0)),
    ]
    route_assert_matches(p, expected_lines)

    # lo0 should be always available
    psh.assert_cmd(
        p, "route add 123.123.123.123 netmask 255.255.255.255 dev lo0"
    )
    expected_lines.insert(
        2,
        match_literal_line(
            format_route_row(
                "123.123.123.123",
                "127.0.0.1",
                "255.255.255.255",
                "U",
                100,
                0,
                0,
                "lo0",
            )
        ),
    )
    route_assert_matches(p, expected_lines)

    psh.assert_cmd(
        p, "route del 123.123.123.123 netmask 255.255.255.255 dev lo0"
    )
    expected_lines.pop(2)
    route_assert_matches(p, expected_lines)
