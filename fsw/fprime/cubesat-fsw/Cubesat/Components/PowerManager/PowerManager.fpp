module Cubesat {

    @ Fault identifier carried by faultIn
    port FaultId(
        value: U32
    )

    @ PA enable gate: 1 = key the S-band PA, 0 = inhibit
    port PaEnable(
        enabled: U8
    )

    @ Battery, mode FSM and CCSDS TM policy.
    @ Passive by design: the rate group drives it, so there is no task stack
    @ and no context switch per tick.
    passive component PowerManager {

        @ 1 Hz housekeeping tick from the rate group
        sync input port schedIn: Svc.Sched

        @ Pass lifecycle triggers (ground scheduler / ephemeris)
        sync input port passScheduled: Svc.Sched
        sync input port passAcquired: Svc.Sched
        sync input port downlinkComplete: Svc.Sched

        @ Latching fault input: battery guard, thermal, watchdog
        sync input port faultIn: FaultId

        @ Ground/POST acknowledgement: Safe -> Charge
        sync input port clearFault: FaultId

        @ Physically gates the S-band PA. ON only while Downlink is active.
        output port paEnable: PaEnable

        @ Mode: 0 Safe, 1 Charge, 2 AwaitPass, 3 Downlink
        telemetry Mode: U8
        telemetry BatteryMwh: I32
        telemetry SocPermille: U16

        event DownlinkEntered() severity activity high id 0 format "S-band downlink window open"

        event FaultLatched(faultId: U32) severity warning high id 1 format "Fault latched: {}"

        @ Standard AC ports
        time get port timeCaller
        import Fw.Event
        import Fw.Channel
        import Fw.Command

        @ Ground commands (demo / bench use)
        sync command CLEAR_FAULT()
        sync command PASS_SCHEDULED()
        sync command PASS_ACQUIRED()
        sync command DOWNLINK_COMPLETE()
    }
}
