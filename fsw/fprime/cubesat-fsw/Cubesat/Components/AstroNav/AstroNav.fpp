module Cubesat {

    @ Ephemeris and attitude-support telemetry from CSPICE. Passive: driven by
    @ the 1 Hz rate group. Kernels are loaded on the first tick.
    passive component AstroNav {

        sync input port schedIn: Svc.Sched

        @ Geometric altitude above the WGS-84 equatorial radius, km
        telemetry AltKm: F64
        @ Inertial speed, km/s
        telemetry SpeedKmS: F64
        @ Position in J2000, km (Earth-centred)
        telemetry PosX: F64
        telemetry PosY: F64
        telemetry PosZ: F64
        @ Velocity in J2000, km/s
        telemetry VelX: F64
        telemetry VelY: F64
        telemetry VelZ: F64
        @ Unit vector Earth -> Sun in J2000
        telemetry SunX: F64
        telemetry SunY: F64
        telemetry SunZ: F64
        @ Sub-satellite point, degrees (IAU_EARTH, spherical)
        telemetry LatDeg: F64
        telemetry LonDeg: F64
        @ Ephemeris time, seconds past J2000
        telemetry Et: F64

        event KernelsReady() severity activity high id 0 format "CSPICE kernels loaded"
        event KernelError(errCode: I32) severity warning high id 1 format "CSPICE kernel error: {}"
        event EphemerisError(errCode: I32) severity warning low id 2 format "Ephemeris lookup failed: {}"

        @ Standard AC ports
        time get port timeCaller
        import Fw.Event
        import Fw.Channel
    }
}
