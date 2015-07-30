#!/bin/sh

QEMU_OPTIONS="-net vde,group=users -net nic,model=ne2k_pci,macaddr=52:54:00:00:00:01" \
make qemu E=dom0_server_only &

sleep 3

QEMU_OPTIONS="-net vde,group=users -net nic,model=ne2k_pci,macaddr=52:54:00:00:00:02" \
make qemu E=dom0_server_and_client &

sleep 3

QEMU_OPTIONS="-net vde,group=users -net nic,model=ne2k_pci,macaddr=52:54:00:00:00:03" \
make qemu E=dom0_server_and_client &

sleep 3

QEMU_OPTIONS="-net vde,group=users -net nic,model=ne2k_pci,macaddr=52:54:00:00:00:04" \
make qemu E=dom0_server_and_client &


