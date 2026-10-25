#!/usr/bin/env python3
"""
GPS Simulator
Simulates a physical GNSS receiver streaming NMEA-0183 sentences ($GPRMC, $GPGGA) at 10 Hz.
"""

import sys
import time
import math
import argparse
from datetime import datetime, timezone

def calculate_checksum(sentence: str) -> str:
    """Calculates 2-digit hex NMEA XOR checksum between $ and *."""
    csum = 0
    content = sentence
    if content.startswith('$') or content.startswith('!'):
        content = content[1:]
    if '*' in content:
        content = content.split('*')[0]
    for char in content:
        csum ^= ord(char)
    return f"{csum:02X}"

def decimal_to_nmea_lat(lat: float) -> tuple[str, str]:
    direction = 'N' if lat >= 0 else 'S'
    abs_lat = abs(lat)
    degrees = int(abs_lat)
    minutes = (abs_lat - degrees) * 60.0
    return f"{degrees:02d}{minutes:07.4f}", direction

def decimal_to_nmea_lon(lon: float) -> tuple[str, str]:
    direction = 'E' if lon >= 0 else 'W'
    abs_lon = abs(lon)
    degrees = int(abs_lon)
    minutes = (abs_lon - degrees) * 60.0
    return f"{degrees:03d}{minutes:07.4f}", direction

def format_gprmc(lat: float, lon: float, speed_kmh: float, heading_deg: float) -> str:
    now = datetime.now(timezone.utc)
    time_str = now.strftime("%H%M%S")
    date_str = now.strftime("%d%m%y")
    lat_str, lat_dir = decimal_to_nmea_lat(lat)
    lon_str, lon_dir = decimal_to_nmea_lon(lon)
    speed_knots = speed_kmh / 1.852
    
    body = f"GPRMC,{time_str},A,{lat_str},{lat_dir},{lon_str},{lon_dir},{speed_knots:.1f},{heading_deg:.1f},{date_str},,,A"
    return f"${body}*{calculate_checksum(body)}"

def format_gpgga(lat: float, lon: float, altitude_m: float = 920.0, hdop: float = 0.9) -> str:
    now = datetime.now(timezone.utc)
    time_str = now.strftime("%H%M%S")
    lat_str, lat_dir = decimal_to_nmea_lat(lat)
    lon_str, lon_dir = decimal_to_nmea_lon(lon)
    
    body = f"GPGGA,{time_str},{lat_str},{lat_dir},{lon_str},{lon_dir},1,08,{hdop:.1f},{altitude_m:.1f},M,0.0,M,,"
    return f"${body}*{calculate_checksum(body)}"

def main():
    parser = argparse.ArgumentParser(description="SMART AEGIS NMEA GNSS Simulator")
    parser.add_argument("--lat", type=float, default=12.971598, help="Initial latitude")
    parser.add_argument("--lon", type=float, default=77.594562, help="Initial longitude")
    parser.add_argument("--speed", type=float, default=65.0, help="Vehicle speed in km/h")
    parser.add_argument("--heading", type=float, default=88.5, help="Heading in degrees [0-360)")
    parser.add_argument("--count", type=int, default=5, help="Number of sentences to emit")
    parser.add_argument("--continuous", action="store_true", help="Stream continuously at 10 Hz")
    args = parser.parse_args()

    print(f"[GPS SIMULATOR] Initialized at Lat: {args.lat:.6f}, Lon: {args.lon:.6f}, Speed: {args.speed:.1f} km/h")

    lat = args.lat
    lon = args.lon
    speed = args.speed
    heading = args.heading

    emitted = 0
    try:
        while args.continuous or emitted < args.count:
            # Advance position slightly (100 ms interval)
            dt = 0.1
            travel_m = (speed / 3.6) * dt
            rad = math.radians(heading)
            dlat = (travel_m * math.cos(rad)) / 6371000.0 * (180.0 / math.pi)
            dlon = (travel_m * math.sin(rad)) / (6371000.0 * math.cos(math.radians(lat))) * (180.0 / math.pi)
            lat += dlat
            lon += dlon

            rmc = format_gprmc(lat, lon, speed, heading)
            gga = format_gpgga(lat, lon)
            print(rmc)
            print(gga)
            sys.stdout.flush()

            emitted += 1
            if args.continuous or emitted < args.count:
                time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n[GPS SIMULATOR] Terminated by user.")

if __name__ == "__main__":
    main()
