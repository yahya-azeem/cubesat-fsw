module Cubesat {

    @ Consumer for the PA enable gate. On flight this owns the S-band
    @ transmitter keying line and records the actual RF state.
    passive component RadioDriver {

        sync input port paEnableIn: PaEnable

        @ 1 when the PA is keyed
        telemetry PaState: U8
        @ Number of key-up transitions
        telemetry KeyCount: U32

        event PaKeyed() severity activity high id 0 format "S-band PA keyed"

        event PaInhibited() severity activity low id 1 format "S-band PA inhibited"

        @ Standard AC ports
        time get port timeCaller
        import Fw.Event
        import Fw.Channel
    }
}
