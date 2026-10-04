#include <linux/bpf.h>
#include <linux/pkt_cls.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <bpf/bpf_helpers.h>

struct stats {
    __u64 packets;
    __u64 bytes;
};

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 3);
    __type(key, __u32);
    __type(value, struct stats);
} counters SEC(".maps");

SEC("tc")
int firewall(struct __sk_buff *skb)
{
    void *data = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;

    __u32 key;
    struct stats *s;

    key = 0;
    s = bpf_map_lookup_elem(&counters, &key);

    if (s) {
        __sync_fetch_and_add(&s->packets, 1);
        __sync_fetch_and_add(&s->bytes, skb->len);
    }


    struct ethhdr *eth = data;

    if ((void *)(eth + 1) > data_end)
        return TC_ACT_OK;

    if (eth->h_proto != __constant_htons(ETH_P_IP))
        goto allow;

    struct iphdr *ip = (void *)(eth + 1);

    if ((void *)(ip + 1) > data_end)
        return TC_ACT_OK;


    if (ip->protocol != 6)
        goto allow;

    struct tcphdr *tcp =
        (void *)ip + (ip->ihl * 4);

    if ((void *)(tcp + 1) > data_end)
        return TC_ACT_OK;

    if (tcp->dest == __constant_htons(80))
    {
        key = 2;

        s = bpf_map_lookup_elem(&counters, &key);

        if (s) {
            __sync_fetch_and_add(&s->packets, 1);
            __sync_fetch_and_add(&s->bytes, skb->len);
        }

        return TC_ACT_SHOT;
    }

allow:
    key = 1;

    s = bpf_map_lookup_elem(&counters, &key);

    if (s) {
        __sync_fetch_and_add(&s->packets, 1);
        __sync_fetch_and_add(&s->bytes, skb->len);
    }

    return TC_ACT_OK;
}


