#!/bin/bash
# Opakované uspání v testovacím initramfs: telefon usne, uživatel ho probudí
# krátkým stiskem tlačítka, skript počká na USB síť a zkontroluje stav jádra.
#   tools/s2idle-cycles.sh [počet]    (výchozí 20)
root="$(cd "$(dirname "$0")/.." && pwd)"
n="${1:-20}"
ok=0
for i in $(seq 1 "$n"); do
	"$root/tools/phone-sh.py" "sh -c 'sleep 3; echo freeze > /sys/power/state' & true" >/dev/null 2>&1
	# počkej, až USB síť zmizí (telefon usnul)
	for _ in $(seq 1 30); do ping -c1 -W1 172.16.42.1 >/dev/null 2>&1 || break; sleep 1; done
	echo "cyklus $i: telefon spí, stiskni krátce tlačítko"
	# počkej na probuzení (max 120 s)
	up=0
	for _ in $(seq 1 120); do ping -c1 -W1 172.16.42.1 >/dev/null 2>&1 && { up=1; break; }; sleep 1; done
	if [ "$up" != 1 ]; then echo "cyklus $i: SELHÁNÍ, telefon se neprobudil"; break; fi
	sleep 3
	st="$("$root/tools/phone-sh.py" 'echo bad=$(dmesg | grep -ciE "Unable to handle|BUG:|Oops"); dd if=/dev/sda of=/dev/null bs=1M count=1 2>&1 | tail -1' 2>&1 | tr '\r\n' ' ')"
	echo "cyklus $i: probuzeno; $st"
	ok=$i
done
echo "hotovo: $ok z $n cyklů prošlo"
