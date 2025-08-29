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
DEFAULT_ROUTE_ARGS_EN1 = (
    "default",
    "0.0.0.0",
    "0.0.0.0",
    "U",
    0,
    0,
    0,
    "en1",
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


def format_expected(
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


@psh.run
def harness(p):
    expected = [
        ROUTE_ROW0,
        format_expected(*DEFAULT_ROUTE_ARGS_EN1),
        format_expected(*DEFAULT_ROUTE_ARGS_LO0),
    ]
    psh.assert_cmd(p, "route", expected=expected)

    # lo0 should be always available
    psh.assert_cmd(
        p, "route add 123.123.123.123 " "netmask 255.255.255.255 dev lo0"
    )
    expected.insert(
        1,
        format_expected(
            "123.123.123.123",
            "127.0.0.1",
            "255.255.255.255",
            "U",
            100,
            0,
            0,
            "lo0",
        ),
    )
    psh.assert_cmd(p, "route", expected=expected)

    psh.assert_cmd(
        p, "route del 123.123.123.123 " "netmask 255.255.255.255 dev lo0"
    )
    expected.pop(1)
    psh.assert_cmd(p, "route", expected=expected)
