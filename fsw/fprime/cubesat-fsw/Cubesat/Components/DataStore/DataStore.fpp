module Cubesat {

    @ Appends housekeeping records to LittleFS. Passive: the rate group runs
    @ it, so there is no task and no queue.
    passive component DataStore {

        @ 1 Hz housekeeping tick
        sync input port schedIn: Svc.Sched

        @ Records successfully appended since boot
        telemetry Records: U32
        @ Bytes written on the last append
        telemetry LastBytes: U32
        @ 1 when LittleFS is mounted
        telemetry Mounted: U8

        event StorageReady() severity activity high id 0 format "LittleFS mounted"

        event WriteFailed(code: I32) severity warning high id 1 format "LittleFS write failed: {}"

        @ Standard AC ports
        time get port timeCaller
        import Fw.Event
        import Fw.Channel
    }
}
