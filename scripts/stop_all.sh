#!/usr/bin/env bash

echo "[INFO] Stopping all SMART AEGIS services..."

pkill -SIGINT -f "coordinator" 2>/dev/null || true
pkill -SIGINT -f "aegis_coordinator" 2>/dev/null || true
pkill -SIGINT -f "intersection" 2>/dev/null || true
pkill -SIGINT -f "aegis_intersection_node" 2>/dev/null || true
pkill -SIGINT -f "simulation" 2>/dev/null || true

sleep 1

# Force kill any stubborn processes
pkill -9 -f "coordinator" 2>/dev/null || true
pkill -9 -f "aegis_coordinator" 2>/dev/null || true
pkill -9 -f "intersection" 2>/dev/null || true
pkill -9 -f "aegis_intersection_node" 2>/dev/null || true
pkill -9 -f "simulation" 2>/dev/null || true

echo "[INFO] All SMART AEGIS processes terminated."
