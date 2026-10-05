import subprocess

HOSTS = {
    "host1": {
        "vxlan": "vxlan100",
        "mtu": 1450,
        "fdb_dst": "10.0.0.2"
    },
    "host2": {
        "vxlan": "vxlan100",
        "mtu": 1450,
        "fdb_dst": "10.0.0.1"
    }
}

FDB_MAC = "00:00:00:00:00:00"


def run(ns, *cmd):
    return subprocess.run(
        ["sudo", "ip", "netns", "exec", ns, *cmd],
        capture_output=True,
        text=True
    )


def reconcile(host, cfg):

    vxlan = cfg["vxlan"]
    mtu = cfg["mtu"]
    dst = cfg["fdb_dst"]

    
    result = run(host, "ip", "link", "show", vxlan)

    if result.returncode != 0:
        print(f"[ERROR] {host}: {vxlan} missing")
        return

    print(f"[OK] {host}: VXLAN exists")

    
    if f"mtu {mtu}" in result.stdout:
        print(f"[OK] {host}: MTU {mtu}")
    else:
        print(f"[DRIFT] {host}: incorrect MTU")
        run(host, "ip", "link", "set", vxlan, "mtu", str(mtu))
        print(f"[REPAIR] MTU restored")

    
    result = run(host, "bridge", "fdb", "show")

    expected = f"{FDB_MAC} dev {vxlan} dst {dst}"

    if expected in result.stdout:
        print(f"[OK] {host}: FDB exists")
    else:
        print(f"[DRIFT] {host}: FDB missing")

        run(
            host,
            "bridge", "fdb", "append",
            FDB_MAC,
            "dev", vxlan,
            "dst", dst
        )

        print(f"[REPAIR] FDB restored")


for host, config in HOSTS.items():
    print(f"\nChecking {host}")
    reconcile(host, config)
