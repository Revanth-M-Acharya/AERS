# SMART AEGIS Source Tree
Contains implementations of all software modules organized according to single-responsibility boundaries:
* `common/`: Shared utility libraries, mathematical routines, and geometric algorithms.
* `config/`: Configuration file loader and schema validator.
* `logger/`: Asynchronous structured logging service (POSIX / QNX slogger2).
* `safety/`: Hardware watchdog manager and heartbeat supervisor.
* `network/`: MQTT client wrapper, POSIX socket adapters, and message serialization.
* `coordinator/`: Central dispatch intelligence, call intake, and fleet asset locks.
* `priority_scheduler/`: Multi-criteria triage arbiter and preemption ranking.
* `route_planner/`: A* graph path calculation and intersection passage vectors.
* `ambulance/`: Vehicle On-Board Unit (OBU), GPS NMEA parser, and vitals streaming.
* `traffic_controller/`: Roadside Unit (RSU), signal phase FSM, and relay actuation.
* `hospital_dashboard/`: Web / Qt emergency room telemetry intake terminal.
* `traffic_dashboard/`: Web / Qt municipal traffic console and manual override desk.
