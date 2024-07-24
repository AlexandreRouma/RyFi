#!/bin/bash
sudo ip addr add 10.0.0.1/24 dev ryfi0
sudo ip link set ryfi0 up
